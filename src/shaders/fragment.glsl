#version 330

in vec3 v_view;
in vec3 v_normal;
in vec3 v_color;
in vec2 v_texcoord;

layout(std140) uniform material {
    vec3 diffuse;
    vec3 specular;
    float specular_power;
    int use_texture;
    int use_vertex_color;
};

uniform sampler2D diffuse_texture;

layout(std140) uniform light {
    vec3 color;
    vec3 direction;
};

layout(location = 0) out vec4 f_color;

vec3 compute_light(vec3 diffuse, vec3 specular, float specular_power,
                   vec3 light_color, vec3 light_direction) {
    vec3 n = normalize(v_normal);
    vec3 h = normalize(normalize(v_view) - light_direction);
    
    float ndotl = -dot(n, light_direction);
    float ndoth = dot(n, h);

    float f_diffuse = clamp(ndotl, 0.0, 1.0);
    float f_specular = pow(clamp(ndoth, 0.0, 1.0), specular_power);

    return light_color * (f_diffuse * diffuse + f_specular * specular);
}

void main() {
    vec3 diffuse_color = diffuse;
    if (use_vertex_color == 1) {
        diffuse_color *= v_color;
    }
    if (use_texture == 1) {
        diffuse_color *= texture(diffuse_texture, v_texcoord).rgb;
    }

    vec3 light_color = compute_light(diffuse_color, specular, specular_power,
        color, direction);

    f_color = vec4(light_color, 1.0);
}

