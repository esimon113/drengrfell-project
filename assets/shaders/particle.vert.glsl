#version 410 core
layout(location = 0) in vec3 squareVertices;
layout(location = 1) in vec4 xyzs;
layout(location = 2) in vec4 color;

out vec2 UV;
out vec4 particlecolor;

uniform mat4 V;
uniform mat4 P;

void main()
{
    float particleSize = xyzs.w;
    vec3 particleCenter = xyzs.xyz;
    
    vec3 vertexPosition = particleCenter + vec3(squareVertices.x * particleSize, squareVertices.y * particleSize, 0.0);
     gl_Position = P * V * vec4(vertexPosition, 1.0);
    
    UV = squareVertices.xy + vec2(0.5, 0.5);
    particlecolor = color;
}