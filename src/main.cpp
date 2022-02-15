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
#include <mesh_renderer.hpp>
#include <mesh_vertex_buffer_writer.hpp>
#include <mesh_io.hpp>


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

void printMeshVertices(const Mesh& mesh) {
    std::cout << "iterating vertices" << std::endl;
    for (auto&& [pos, norm, uv] : mesh.view<vec3, vec3, vec2>(MeshAttribute::POSITION, MeshAttribute::NORMAL, MeshAttribute::TEXCOORD)) {
        std::cout << "Vertex:\n\t" <<
            "Position: " << vvm::to_string(pos) << "\n\t" <<
            "Normal: " << vvm::to_string(norm) << "\n" <<
            "UV: " << vvm::to_string(uv) << "\n";
    }
    std::cout << std::flush;
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

    Mesh testMesh;
    {
        using ma = MeshAttribute;
        testMesh.setNumVertices(4);

        testMesh.createAttributeBuffer<vec3>(ma::POSITION).assign({
                {-1, -1,  1},
                { 1, -1,  1},
                { 0, -1, -1},
                { 0,  1,  1}
            });
        
        testMesh.createAttributeBuffer<vec3>(ma::NORMAL).assign({
                vvm::normalize(vec3(-0.5, -0.5,  0.5)),
                vvm::normalize(vec3( 0.5, -0.5,  0.5)),
                vvm::normalize(vec3( 0.0, -0.5, -0.5)),
                vvm::normalize(vec3( 0.0,  0.5,  0.5))
            });

        testMesh.createAttributeBuffer<vec2>(ma::TEXCOORD).assign(0, 0);
    }


    
    MeshWriter("test_mesh.mbin").writeMesh(testMesh);

    // testMesh = MeshReader("C:\\Users\\vbk73\\Desktop\\test_export.mbin").readMesh();

    printMeshVertices(testMesh);
    for (auto ind : testMesh.indices()) std::cout << ind << ",";
    std::cout << std::endl;    

    // todo: auto generate
    // easy to get the elements from the mesh, but the order matters
    // and depends on shader access.
    // eventually, shaders should be auto-generated too so no problem there i guess
    RenderMeshMapping renderMeshMapping;
    renderMeshMapping.attributeMappings = {
        RenderMeshMapping::AttributeMapping {
            .attribute = MeshAttribute::POSITION,
            .componentType = MeshAttributeComponentType::FLOAT,
            .numComponents = 3
        },
        RenderMeshMapping::AttributeMapping {
            .attribute = MeshAttribute::NORMAL,
            .componentType = MeshAttributeComponentType::FLOAT,
            .numComponents = 3
        }};
    MeshRenderer meshRenderer(renderMeshMapping, 1000, 1000);
    
    MeshVertexBufferWriter(testMesh).write(meshRenderer);


    meshRenderer.getVertexArray().bind();
    meshRenderer.getIndexBuffer().bind(GL_ELEMENT_ARRAY_BUFFER);

    vvm::v3f camera_position = {0, 0, 3};

    glEnable(GL_DEPTH_TEST);


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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

        glfwSwapBuffers(context.window);
    }

    return 0;
}