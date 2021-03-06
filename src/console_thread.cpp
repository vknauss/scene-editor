#include "console_thread.hpp"

#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define MAKE_TOKEN_STRING_CASE(X) case Token::X: return std::string(#X)


struct IOStreamWrapper {
    std::istream& in;
    std::ostream& out;
};

enum class Token {
    EXIT,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    OPARENS,
    CPARENS,
    OCURLYB,
    CCURLYB,
    OSQUAREB,
    CSQUAREB,
    VALUE_STRING,
    VALUE_FLOAT,
    VALUE_INT
};

std::string tokenString(Token token) {
    switch (token) {
    MAKE_TOKEN_STRING_CASE(EXIT);
    MAKE_TOKEN_STRING_CASE(PLUS);
    MAKE_TOKEN_STRING_CASE(MINUS);
    MAKE_TOKEN_STRING_CASE(STAR);
    MAKE_TOKEN_STRING_CASE(SLASH);
    MAKE_TOKEN_STRING_CASE(OPARENS);
    MAKE_TOKEN_STRING_CASE(CPARENS);
    MAKE_TOKEN_STRING_CASE(OCURLYB);
    MAKE_TOKEN_STRING_CASE(CCURLYB);
    MAKE_TOKEN_STRING_CASE(OSQUAREB);
    MAKE_TOKEN_STRING_CASE(CSQUAREB);
    MAKE_TOKEN_STRING_CASE(VALUE_STRING);
    MAKE_TOKEN_STRING_CASE(VALUE_FLOAT);
    MAKE_TOKEN_STRING_CASE(VALUE_INT);
    default: return "Unknown token";
    }
}

class BaseToken {
public:

    virtual ~BaseToken() {}

    bool match(Token token) const;

    template<typename T>
    bool match(Token token, T& value) const;

    Token token() const { return _token; }

protected:

    Token _token;

    BaseToken(Token token) : _token(token) {}
    
};

class SimpleToken : public BaseToken {
public:

    SimpleToken(Token token) : BaseToken(token) {}

};

template<typename T>
class ValueToken : public BaseToken {
public:
    friend class BaseToken;

    ValueToken(Token token, const T& value) : BaseToken(token), _value(value) { }

    const T& value() const { return _value; }

private:

    const T _value;

};

bool BaseToken::match(Token token) const {
    if (dynamic_cast<const SimpleToken*>(this)) {
        return _token == token;
    }
    return false;
}

template<typename T>
bool BaseToken::match(Token token, T& value) const {
    if (auto vt = dynamic_cast<const ValueToken<T>*>(this)) {
        if (_token == token) {
            value = vt->_value;
            return true;
        }
    }
    return false;
}

class BaseExpression {

public:

    virtual std::unique_ptr<BaseExpression> evaluate() const = 0;

    virtual ~BaseExpression() { }

    template<typename T>
    bool match(T& value) const;

};

template<typename T>
class ValueExpression : public BaseExpression {
public:

    ValueExpression(const T& value) : _value(value) { }

    const T& value() const {
        return _value;
    }

    std::unique_ptr<BaseExpression> evaluate() const override {
        return std::make_unique<ValueExpression<T>>(*this);
    }

private:

    T _value;
};

template<typename Op>
class UnaryExpression : public BaseExpression {
public:

    UnaryExpression(const Op& op, std::unique_ptr<BaseExpression>&& expr) :
        _op(op), _expr(std::move(expr)) { }

    std::unique_ptr<BaseExpression> evaluate() const override {
        return std::unique_ptr<BaseExpression>(_op(*_expr->evaluate()));
    }

private:

    Op _op;

    std::unique_ptr<BaseExpression> _expr;

};

template<typename Op>
class BinaryExpression : public BaseExpression {
public:

    BinaryExpression(const Op& op, std::unique_ptr<BaseExpression>&& expr1,
        std::unique_ptr<BaseExpression>&& expr2) :
            _op(op), _expr1(std::move(expr1)), _expr2(std::move(expr2)) { }

    std::unique_ptr<BaseExpression> evaluate() const override {
        return std::unique_ptr<BaseExpression>(_op(*_expr1->evaluate(), *_expr2->evaluate()));
    }

private:

    Op _op;

    std::unique_ptr<BaseExpression> _expr1, _expr2;

};

enum class ControlFlowSymbol {
    EXIT
};

template<typename T>
bool BaseExpression::match(T& value) const {
    if (auto ve = dynamic_cast<const ValueExpression<T>*>(this)) {
        value = ve->value();
        return true;
    }
    return false;
}

static bool matchStrings(const std::string& compared,
        const std::initializer_list<std::string>& comparing, size_t start, size_t end) {
    for (const std::string& s : comparing) {
        if (compared.compare(start, end, s) == 0) {
            return true;
        }
    }
    return false;
}

static std::tuple<std::unique_ptr<BaseToken>, size_t>
getToken(const std::string& line, size_t position) {
    // skip beginning whitespace
    while (position < line.length() && isspace(line[position])) ++position;
    if (position == line.length()) {
        return std::make_tuple(nullptr, position);
    }

    auto make_ret = [] (Token token, size_t position) {
        return std::make_tuple(std::make_unique<SimpleToken>(token), position);
    };

    auto make_value_ret = [] (Token token, auto value, size_t position) {
        return std::make_tuple(std::make_unique<ValueToken<decltype(value)>>(token, value), position);
    };

    // check single-character tokens
    switch (line[position]) {
    case '+':
        return make_ret(Token::PLUS, position+1);
    case '-':
        return make_ret(Token::MINUS, position+1);
    case '*':
        return make_ret(Token::STAR, position+1);
    case '/':
        return make_ret(Token::SLASH, position+1);
    case '(':
        return make_ret(Token::OPARENS, position+1);
    case ')':
        return make_ret(Token::CPARENS, position+1);
    case '{':
        return make_ret(Token::OCURLYB, position+1);
    case '}':
        return make_ret(Token::CCURLYB, position+1);
    case '[':
        return make_ret(Token::OSQUAREB, position+1);
    case ']':
        return make_ret(Token::CSQUAREB, position+1);
    default:
        break;
    }
    
    // check quotes
    size_t endpos;
    if (line[position] == '\"' || line[position] == '\'') {
        endpos = line.find_first_of(&line[position], position + 1, 1);
        if (endpos == std::string::npos) {
            throw std::runtime_error("Unmatched quote at position: " + std::to_string(position));
        }
        return make_value_ret(Token::VALUE_STRING, line.substr(position+1, endpos-position-1), endpos+1);
    }
    
    // delimit by whitespace
    for (endpos = position + 1; endpos < line.length() && !isspace(line[endpos]); ++endpos);
    
    // check keywords
    if (matchStrings(line, {"quit", "exit"}, position, endpos)) {
        return make_ret(Token::EXIT, endpos);
    }

    // value types, most to least specific
    if (isdigit(line[position])) {
        if (line[position] == '0' && endpos > position + 2 && (line[position + 1] == 'x' || line[position + 1] == 'X')) {
            // try to parse hex
            bool hasDot = false, hasExponent = false;
            bool isHex = true;
            for (size_t i = position + 2; i < endpos; ++i) {
                if (!isxdigit(line[i])) {
                    if (!hasDot && !hasExponent && line[i] == '.') {
                        hasDot = true;
                    } else if (!hasExponent && i + 1 < endpos && (line[i] == 'p' || line[i] == 'P')) {
                        if ((line[i + 1] == '-' || line[i + 1] == '-') && i + 2 < endpos) ++i;
                        hasExponent = true;
                    } else {
                        isHex = false;
                        break;
                    }
                }
            }
            if (isHex) {
                if (hasDot || hasExponent) {
                    double d = strtod(&line[position], nullptr);
                    return make_value_ret(Token::VALUE_FLOAT, (float) d, endpos);
                } else {
                    long l = strtol(&line[position], nullptr, 0);
                    return make_value_ret(Token::VALUE_INT, (int) l, endpos);
                }
            }
        }
        bool hasDot = false, hasExponent = false;
        bool isNumber = true;
        for (size_t i = position; i < endpos; ++i) {
            if (!isdigit(line[i])) {
                if (!hasDot && !hasExponent && line[i] == '.') {
                    hasDot = true;
                } else if (!hasExponent && i + 1 < endpos && (line[i] == 'e' || line[i] == 'E')) {
                    if ((line[i + 1] == '-' || line[i + 1] == '-') && i + 2 < endpos) ++i;
                    hasExponent = true;
                } else {
                    isNumber = false;
                    break;
                }
            }
        }
        if (isNumber) {
            if (hasDot || hasExponent) {
                double d = strtod(&line[position], nullptr);
                return make_value_ret(Token::VALUE_FLOAT, (float) d, endpos);
            } else {
                long l = strtol(&line[position], nullptr, 0);
                return make_value_ret(Token::VALUE_INT, (int) l, endpos);
            }
        }
    }

    return make_value_ret(Token::VALUE_STRING, line.substr(position, endpos-position), endpos);
}

static std::vector<std::unique_ptr<BaseToken>>
getLineTokens(const std::string& line) {
    std::vector<std::unique_ptr<BaseToken>> tokens;

    size_t position = 0;
    while (position < line.length()) {
        auto [token, nextpos] = getToken(line, position);
        if (!token) break;
        tokens.push_back(std::move(token));
        position = nextpos;
    }

    return tokens;
}

template<typename T>
static std::string as_string(const T& t) {
    return std::to_string(t);
}

template<>
std::string as_string(const std::string& s) {
    return s;
}

static std::tuple<std::unique_ptr<BaseExpression>, size_t>
buildExpression(std::unique_ptr<BaseExpression>&& left, const std::vector<std::unique_ptr<BaseToken>>& tokens, size_t position) {
    auto unexpected_token_type = [] (Token token, auto value) {
        return std::runtime_error("Unexpected token " + tokenString(token) + ": " + as_string(value));
    };

    auto unexpected_token = [] (Token token) {
        return std::runtime_error("Unexpected token " + tokenString(token));
    };

    auto make_value_expr = [] (auto value) {
        return std::make_unique<ValueExpression<decltype(value)>>(value);
    };

    auto make_binary_expr = [] (auto op, auto&& left, auto&& right) {
        return std::make_unique<BinaryExpression<decltype(op)>>(op, std::move(left), std::move(right));
    };

    auto make_unary_expr = [] (auto op, auto&& e) {
        return std::make_unique<UnaryExpression<decltype(op)>>(op, std::move(e));
    };

    auto make_base_expr = [] (auto&& expr) {
        return std::unique_ptr<BaseExpression>(std::move(expr));
    };

    auto make_ret = [] (auto&& expr, size_t position) {
        return std::make_tuple(std::move(expr), position);
    };

    auto handle_value_case = [&] (const auto& token, auto& value) {
        assert(token->match(token->token(), value));
        if (left) {
            throw unexpected_token_type(token->token(), value);
        }
        return make_ret(make_value_expr(value), position + 1);
    };

    auto make_type_op = [&] (auto op, auto lval, const BaseExpression& rexp) {
        if (decltype(lval) rval; rexp.match(rval)) {
            return make_base_expr(make_value_expr(op(lval, rval)));
        } else {
            throw std::runtime_error("Types for binary operation must match");
        }
    };

    auto handle_binary_case = [&] (const auto& token, auto op) {
        if (!left) {
            throw unexpected_token(token->token());
        }
        auto [right, nextpos] = buildExpression(nullptr, tokens, position+1);
        if (!right) {
            throw std::runtime_error("Expected expression for right hand side of token " + tokenString(token->token()));
        }
        return make_ret(make_binary_expr(op, std::move(left), std::move(right)), nextpos);
    };
    
    auto handle_unary_case = [&] (const auto& token, auto op) {
        if (left) {
            throw unexpected_token(token->token());
        }
        auto [right, nextpos] = buildExpression(nullptr, tokens, position+1);
        if (!right) {
            throw std::runtime_error("Expected expression for right hand side of token " + tokenString(token->token()));
        }
        return make_ret(make_unary_expr(op, std::move(right)), nextpos);
    };

    if (position == tokens.size()) {
        return make_ret(nullptr, position);
    }

    const auto& ptok = tokens[position];
    switch (ptok->token()) {
        case Token::VALUE_FLOAT: {
            float value;
            return handle_value_case(ptok, value);
        }
        case Token::VALUE_INT: {
            int value;
            return handle_value_case(ptok, value);
        }
        case Token::VALUE_STRING: {
            std::string value;
            return handle_value_case(ptok, value);
        }
        case Token::EXIT: {
            if (left) {
                throw unexpected_token(ptok->token());
            }
            return make_ret(make_value_expr(ControlFlowSymbol::EXIT), position+1);
        }
        case Token::PLUS: {
            auto op = [] (auto x, auto y) { return x + y; };
            return handle_binary_case(ptok,
                [&] (const BaseExpression& e1, const BaseExpression& e2) {
                    if (int x; e1.match(x)) {
                        return make_type_op(op, x, e2);
                    }
                    if (float x; e1.match(x)) {
                        return make_type_op(op, x, e2);
                    }
                    if (std::string x; e1.match(x)) {
                        return make_type_op(op, x, e2);
                    }
                    throw std::runtime_error("Expected value for left hand side of binary op");
                });
        }
        case Token::MINUS: {
            if (left) {
                auto op = [] (auto x, auto y) { return x - y; };
                return handle_binary_case(ptok,
                    [&] (const BaseExpression& e1, const BaseExpression& e2) {
                        if (int x; e1.match(x)) {
                            return make_type_op(op, x, e2);
                        }
                        if (float x; e1.match(x)) {
                            return make_type_op(op, x, e2);
                        }
                        throw std::runtime_error("Expected value for left hand side of binary op");
                    });
            } else {
                auto op = [] (auto x) { return -x; };
                return handle_unary_case(ptok, [&] (const BaseExpression& e) {
                    if (int x; e.match(x)) {
                        return make_base_expr(make_value_expr(op(x)));
                    }
                    if (float x; e.match(x)) {
                        return make_base_expr(make_value_expr(op(x)));
                    }
                    throw std::runtime_error("Expected value for right hand side of unary op");
                });
            }
        }

        default:
            throw unexpected_token(ptok->token());
    }
}

static std::unique_ptr<BaseExpression>
parseTokens(const std::vector<std::unique_ptr<BaseToken>>& tokens) {
    std::unique_ptr<BaseExpression> expr = nullptr;

    size_t position = 0;
    while (position < tokens.size()) {
        auto [newexpr, nextpos] = buildExpression(std::move(expr), tokens, position);
        expr = std::move(newexpr);
        position = nextpos;
    }
    
    return expr;
}

static void consoleThreadMain(IOStreamWrapper streams) {
    bool running = true;

    while (running) {
        streams.out << ">> " << std::flush;
        if (std::string line; getline(streams.in, line)) {
            try {
                auto ptoks = getLineTokens(line);
                auto expr = parseTokens(ptoks);
                if (expr) {
                    auto val = expr->evaluate();
                    if (int i; val->match(i)) {
                        streams.out << "Int: " << i << std::endl;
                    }
                    if (float f; val->match(f)) {
                        streams.out << "Float: " << f << std::endl;
                    }
                    if (std::string s; val->match(s)) {
                        streams.out << "String: " << s << std::endl;
                    }
                    if (ControlFlowSymbol s; val->match(s)) {
                        running = false;
                    }
                }
            } catch (std::exception& e) {
                streams.out << "Error: " << e.what() << std::endl;
            }
        } else {
            running = false;
        }
    }
    streams.out << "goodbye" << std::endl;
}

ConsoleThread::ConsoleThread(std::istream& in, std::ostream& out) :
    std::thread(consoleThreadMain, IOStreamWrapper {in, out}) {
}

