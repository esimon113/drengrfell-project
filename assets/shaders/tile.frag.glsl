#version 410 core
in vec2 uv;
flat in int vertType;
out vec4 color;
flat in int fragExplored;

uniform sampler2DArray tileAtlas;

vec4 getTileColor(int type) {
    switch(type) {
        case 1: return vec4(0.0, 0.0, 1.0, 1); // WATER
        case 2: return vec4(0.0, 0.5, 0.0, 1); // FOREST
        case 3: return vec4(0.0, 1.0, 0.0, 1); // GRASS
        case 4: return vec4(0.75, 0.75, 0.75, 1); // MOUNTAIN
        case 5: return vec4(0.5, 1.0, 0.0, 1); // FIELD
        case 6: return vec4(0.5, 0.5, 0.5, 1); // CLAY
        case 7: return vec4(0.75, 0.75, 1.0, 1); // ICE
        default: return vec4(0.0, 0.0, 0.0, 1); // EMPTY
    }
}

void main() {
    if(fragExplored == int(0)){
        color = texture(tileAtlas, vec3(uv.x, uv.y, 7 ));
    } else {
        color = texture(tileAtlas, vec3(uv.x, uv.y, 7 - vertType));
    }
    

    //color = getTileColor(vertType);
}
