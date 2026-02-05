#version 410 core
in vec2 uv;
flat in int vertType;
flat in int vertExplored;
flat in vec4 indicatorColor;
flat in uint vertTileIndex;
flat in int vertOnPath;

out vec4 color;

uniform float time; // In seconds
uniform int frames; // Number of animation frames (resp. sprites) per tile
uniform sampler2DArray tileAtlas;
uniform int selectedTile;

void main() {
	int frame = int(floor(time * frames)) % frames;
	int renderExplored = vertExplored;
	if( vertType == 1){
		renderExplored = 1;
	}
	int sprite = renderExplored * vertType * frames + frame;

	vec4 baseColor = indicatorColor * texture(tileAtlas, vec3(uv.x, uv.y, sprite));

	// Pulses intensity for highlighted path between 0.5 and 1
	float pulse = 0.75 + 0.25 * sin(time * 3.1415);

	if (vertOnPath == 1 && vertTileIndex != selectedTile) {
		baseColor.a *= pulse;
	}

    //int sprite = (vertExplored * (vertType * frames)) + int(time * frames);
    color = baseColor;
}
