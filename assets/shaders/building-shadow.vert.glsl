#version 410 core

// same as sprite.vert.glsl

layout(location = 0) out vec2 frag_uv;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model[15];

vec2 position[4] = vec2[](
        vec2(-1, -1),
        vec2(1, -1),
        vec2(1, 1),
        vec2(-1, 1)
    );

void main() {
    frag_uv = position[gl_VertexID] * 0.5 + 0.5;
    vec4 pos = vec4(position[gl_VertexID], 0, 1);
    gl_Position = projection * view * model[gl_InstanceID] * pos;
}
