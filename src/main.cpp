#include <fstream>
#include <iostream>
#include <stdexcept>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ogu/shader.h>
#include <ogu/vertex_array.h>

#include <vvm/vvm.hpp>
#include <vvm/matrix_tfm.hpp>
#include <vvm/string.hpp>

#include <mesh.hpp>


struct glfw_context {
public:
    GLFWwindow* window;

    glfw_context() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        window = glfwCreateWindow(640, 480, "GLFW Window", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window.");
        }

        glfwMakeContextCurrent(window);
    }

    ~glfw_context() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    glfw_context(const glfw_context&) = delete;
    glfw_context(glfw_context&&) = delete;

    glfw_context& operator=(const glfw_context&) = delete;
    glfw_context& operator=(glfw_context&&) = delete;
};

std::string file_as_string(const std::string& filename) {
    std::ifstream f(filename);
    if (!f) {
        throw std::runtime_error("Failed to load file: \"" + filename + "\".");
    }

    return std::string {
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    };
}

void printMeshPositions(const Mesh& mesh) {
    for (const auto& pos : mesh.getAttributeBuffer<vec3>(MeshAttribute::POSITION)) {
        std::cout << vvm::to_string(pos) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    glfw_context context;

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW.");
    }

    ogu::shader_program program({
        ogu::shader({file_as_string("shaders/vertex.glsl")},
                    ogu::shader::type::VERTEX),
        ogu::shader({file_as_string("shaders/fragment.glsl")},
                    ogu::shader::type::FRAGMENT)
    });
    program.use();

    program.addUniformBuffer("matrices");
    program.addUniformBuffer("material");
    program.addUniformBuffer("light");
    program.addUniform("diffuse_texture");

    int width, height;
    glfwGetWindowSize(context.window, &width, &height);

    struct matrices {
        vvm::m4f projection;
        vvm::m4f model_view;
        vvm::m4f model_view_normals;
    };

    struct material {
        vvm::v3f diffuse;
        float pad0;
        vvm::v3f specular;
        float specular_power;
        int use_texture;
        int use_vertex_color;
    };

    struct light {
        vvm::v3f color;
        float pad0;
        vvm::v3f direction;
        float pad1;
    };

    ogu::buffer matrices_ubo(sizeof(matrices));
    matrices_ubo.write(0, 0, [&] (void* buffer_data) {
        matrices* m = (matrices*) buffer_data;
        m->projection = vvm::ortho(1.0f, (float) width / (float) height);
        m->model_view = vvm::identity<vvm::m4f>();
        m->model_view_normals = vvm::identity<vvm::m4f>();
    });

    ogu::buffer material_ubo(sizeof(material));
    material_ubo.write(0, 0, [] (void* buffer_data) {
        material* m = (material*) buffer_data;
        m->diffuse = vvm::v3f(0.5f);
        m->specular = vvm::v3f(0.8f);
        m->specular_power = 30.0f;
        m->use_texture = 0;
        m->use_vertex_color = 0;
    });

    ogu::buffer light_ubo(sizeof(light));
    light_ubo.write(0, 0, [] (void* buffer_data) {
        light* l = (light*) buffer_data;
        l->color = vvm::v3f(1.0f);
        l->direction = vvm::v3f(0, 0, -1);
    });

    program.bindUniformBuffer("matrices", matrices_ubo);
    program.bindUniformBuffer("material", material_ubo);
    program.bindUniformBuffer("light", light_ubo);

    ogu::buffer vbo(4 * (sizeof(vvm::v3f) + sizeof(vvm::v3f)));
    vbo.write(0, 0, [] (void* buffer_data) {
        vvm::v3f* vec3_ptr = (vvm::v3f*) buffer_data;
        *vec3_ptr++ = {-0.5, -0.5,  0.5};
        *vec3_ptr++ = { 0.0,  0.0,  1.0};
        *vec3_ptr++ = { 0.5, -0.5,  0.5};
        *vec3_ptr++ = { 0.0,  0.0,  1.0};
        *vec3_ptr++ = { 0.0, -0.5, -0.5};
        *vec3_ptr++ = { 0.0,  0.0,  1.0};
        *vec3_ptr++ = { 0.0,  0.5,  0.0};
        *vec3_ptr++ = { 0.0,  0.0,  1.0};
    });

    ogu::vertex_array vao({
        ogu::vertex_buffer_binding(vbo, {
            ogu::vertex_attrib_description(0, 3, GL_FLOAT, 0),
            ogu::vertex_attrib_description(1, 3, GL_FLOAT, sizeof(vvm::v3f))
        }, 2 * sizeof(vvm::v3f))
    });

    vao.bind();

    vvm::v3f camera_position = {0, 0, 3};

    Mesh mesh;

    mesh.resize(36);

    mesh.createAttributeBuffer<vec3>(MeshAttribute::POSITION);
    for (auto& pos : mesh.getAttributeBuffer<vec3>(MeshAttribute::POSITION)) {
        pos = vec3(1, 1, 1);
    }

    printMeshPositions(mesh);


    while (!glfwWindowShouldClose(context.window)) {
        glfwPollEvents();

        glfwGetWindowSize(context.window, &width, &height);

        if (glfwGetKey(context.window, GLFW_KEY_W))
            camera_position.z -= 0.001;
            
        if (glfwGetKey(context.window, GLFW_KEY_S))
            camera_position.z += 0.001;

        matrices_ubo.write(0, 0, [&] (void* buffer_data) {
            matrices* m = (matrices*) buffer_data;
            m->projection = vvm::perspective((float) M_PI / 2.0f, (float) width / (float) height, 0.1f, 100.0f);
            m->model_view = vvm::translate(-camera_position) * vvm::m4f(vvm::rotateX((float) glfwGetTime()));
            m->model_view_normals = vvm::m4f(vvm::m3f(m->model_view));
        });
        program.bindUniformBuffer("matrices", matrices_ubo);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(context.window);
    }

    return 0;
}