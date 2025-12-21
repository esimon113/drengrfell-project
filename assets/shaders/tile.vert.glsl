#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vertexUv;
layout(location = 2) in vec2 instancePosition;
layout(location = 3) in int type;
layout(location = 4) in int explored;
layout(location = 5) in uint tileIndex;

out vec2 uv;
flat out int vertType;
flat out int vertExplored;
flat out vec4 indicatorColor;

uniform mat4 model;
uniform mat4 projection;
uniform int selectedTile;

void main() {
    vertType = type;
    vertExplored = explored;
    indicatorColor = (tileIndex == uint(selectedTile))
                   ? vec4(1.0, 0.0, 0.0, 1.0)
                   : vec4(1.0, 1.0, 1.0, 1.0);

    vec2 worldPos = position + instancePosition;
    gl_Position = projection * model * vec4(worldPos, 0.0, 1.0);
    uv = vertexUv;
}
