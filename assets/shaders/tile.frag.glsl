#version 410 core
in vec2 uv;
flat in int vertType;
flat in int vertExplored;

out vec4 color;

uniform float time;
uniform int frames;
uniform sampler2DArray tileAtlas;

void main() {
    int sprite = (vertExplored * (vertType * frames)) + int(time * frames);
    color = texture(tileAtlas, vec3(uv.x, uv.y, sprite));
}
