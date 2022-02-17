#pragma once

#include <istream>
#include <ostream>
#include <thread>


class ConsoleThread : public std::thread {

public:

    ConsoleThread(std::istream& in, std::ostream& out);



};