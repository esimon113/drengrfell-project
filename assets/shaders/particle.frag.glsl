#version 410 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 particlecolor;

// Ouput data
out vec4 color;

uniform sampler2D myTextureSampler;

void main(){
    // If you want to use a texture (optional):
    // color = texture(myTextureSampler, UV) * particlecolor;
    
    // For now, just use the color and make it circular/soft
    vec2 coord = UV - vec2(0.5);
    float dist = length(coord);
    
    // Make particles circular
    if(dist > 0.5) 
        discard;
    
    // Soft edges
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    
    color = vec4(particlecolor.rgb, particlecolor.a * alpha);
}