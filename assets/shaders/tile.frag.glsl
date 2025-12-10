#version 410 core
in vec2 uv;
flat in int vertType;
out vec4 color;
flat in int fragExplored;

uniform sampler2DArray tileAtlas;

void main() {
    color = texture(tileAtlas, vec3(uv.x, uv.y, fragExplored * vertType));
}
