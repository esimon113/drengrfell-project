#version 410 core

flat in vec4 pickingColor;

out vec4 color;

void main() {
    color = pickingColor;
}
