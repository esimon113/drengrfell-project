#version 410 core
in vec2 uv;
flat in int vertType;
flat in int vertExplored;
flat in vec4 indicatorColor;

out vec4 color;

uniform float time; // In seconds
uniform int frames; // Number of animation frames (resp. sprites) per tile
uniform sampler2DArray tileAtlas;

void main() {
    int sprite = (vertExplored * (vertType * frames)) + int(time * frames);
    color = indicatorColor * texture(tileAtlas, vec3(uv.x, uv.y, sprite));
}
