#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vertexUv;
layout(location = 2) in vec2 instancePosition;
layout(location = 3) in int type;
layout(location = 4) in int explored;
layout(location = 5) in uint tileIndex;
layout(location = 6) in int onPath;

out vec2 uv;
flat out int vertType;
flat out int vertExplored;
flat out vec4 indicatorColor;
flat out uint vertTileIndex;
flat out int vertOnPath;

uniform mat4 model;
uniform mat4 projection;
uniform int selectedTile;

void main() {
    vertType = type;
    vertExplored = explored;
	vertOnPath = onPath;
	vertTileIndex = tileIndex;
	if (tileIndex == uint(selectedTile)) {
		indicatorColor = vec4(0.25, 0.25, 1.0, 1.0);	// Highlight target tile
	} else if (onPath == 1) {
		indicatorColor = vec4(0.65, 0.65, 1.0, 1.0);		// weakly highlight path tiles
	} else {
		indicatorColor = vec4(1.0, 1.0, 1.0, 1.0);
	}

    vec2 worldPos = position + instancePosition;
    gl_Position = projection * model * vec4(worldPos, 0.0, 1.0);
    uv = vertexUv;
}
