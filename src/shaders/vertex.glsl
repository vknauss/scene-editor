#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 texcoord;

layout(std140) uniform matrices {
    mat4 projection;
    mat4 model_view;
    mat4 model_view_normals;
};

out vec3 v_view;
out vec3 v_normal;
out vec3 v_color;
out vec2 v_texcoord; 

void main() {
    vec4 mv_position = model_view * vec4(position, 1.0);
    v_view = -vec3(mv_position);
    gl_Position = projection * mv_position;
    v_normal = (model_view_normals * vec4(normal, 0.0)).xyz;
    v_color = color;
    v_texcoord = texcoord;
}
