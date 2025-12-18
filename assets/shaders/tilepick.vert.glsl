#version 410 core
// Same as in tile.vert.glsl
// See https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/ for idea

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vertexUv;
layout(location = 2) in vec2 instancePosition;
layout(location = 3) in int type;
layout(location = 4) in int explored;
layout(location = 5) in uint tileIndex;

flat out vec4 pickingColor;

uniform mat4 model;
uniform mat4 projection;

vec3 integerToColor(uint i) {
    return vec3(
        float((i & 0x000000FF) >> 0),
        float((i & 0x0000FF00) >> 8),
        float((i & 0x00FF0000) >> 16)
    ) / 255.0;
}

void main() {
    vec2 worldPos = position + instancePosition;
    gl_Position = projection * model * vec4(worldPos, 0.0, 1.0);
    pickingColor = vec4(integerToColor(tileIndex), 1.0);
}
