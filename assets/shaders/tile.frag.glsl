#version 410 core
flat in int vertType;
out vec4 FragColor;

vec3 getTileColor(int type) {
    switch(type) {
        case 0: return vec3(0.0, 0.0, 1.0); // WATER
        case 1: return vec3(0.0, 0.5, 0.0); // FOREST
        case 2: return vec3(0.0, 1.0, 0.0); // GRASS
        case 3: return vec3(0.75, 0.75, 0.75); // MOUNTAIN
        case 4: return vec3(0.5, 1.0, 0.0); // FIELD
        case 5: return vec3(0.5, 0.5, 0.5); // CLAY
        case 6: return vec3(0.75, 0.75, 1.0); // ICE
        default: return vec3(0.0, 0.0, 0.0); // EMPTY
    }
}

void main() {
    vec3 color = getTileColor(vertType);
    FragColor = vec4(color, 1.0);
}
