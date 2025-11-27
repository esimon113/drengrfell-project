#version 410 core

layout(location = 0) in vec2 frag_uv;

uniform sampler2D sprite;
uniform vec3 fcolor;
uniform float time; // used for pulsing effect

layout(location = 0) out vec4 color;

void main() {
    vec4 texColor = texture(sprite, frag_uv);

    // discard transparent pixels
    if (texColor.a < 0.01) {
        discard;
    }

    // add pulsing brightness for hovering settlement-texture
    float pulse = 0.9 + 0.2 * sin(time * 2.0);
    vec3 finalColor = fcolor * texColor.rgb * pulse;

    // use alpha channel (.a) of texture
    color = vec4(finalColor, texColor.a);
}
