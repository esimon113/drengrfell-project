#version 410 core

layout(location = 0) in vec2 frag_uv;

uniform sampler2D sprite;

layout(location = 0) out vec4 color;

void main() {
    vec4 texColor = texture(sprite, frag_uv);

    // render shadow where the settlement is not invisible
    // make transparent otherwise:
    if (texColor.a < 0.01) {
        discard;
    }

    // create gradient: shadow fades out -> no hard shadow-border
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(frag_uv, center);
    float gradientFade = 1.0 - smoothstep(0.2, 0.5, dist);

    vec3 shadowColor = vec3(0.0, 0.0, 0.0);

    // use alpha from texture as mask
    float baseAlpha = texColor.a;
    float finalAlpha = baseAlpha * gradientFade * 0.9;

    color = vec4(shadowColor, finalAlpha);
}
