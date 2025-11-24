#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 instancePosition;
layout(location = 2) in int type;

flat out int vertType;

uniform mat4 projection;

void main() {
    vertType = type;

    vec2 worldPos = position + instancePosition;
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);
}
