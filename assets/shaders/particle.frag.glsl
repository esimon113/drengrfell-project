#version 410 core
in vec2 UV;
in vec4 particlecolor;

out vec4 color;

uniform sampler2D myTextureSampler;

void main(){

    vec2 coord = UV - vec2(0.5);
    float dist = length(coord);

    
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    
    color = vec4(particlecolor.rgb, particlecolor.a * alpha);
}