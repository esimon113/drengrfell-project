#version 410 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;

uniform mat4 model[1];
uniform mat4 view;
uniform mat4 projection;

layout(location = 0) out vec2 frag_uv;

void main() {
    frag_uv = texcoord;
    gl_Position = projection * view * model[0] * vec4(position, 0.0, 1.0);
}

