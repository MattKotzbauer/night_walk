// rain.frag
#version 330 core
in vec2 TexCoord;
in vec2 Velocity;
in float VerticalFade;

out vec4 FragColor;

uniform sampler2D rainTexture;

void main() {
    // Basic raindrop color
    // (Alpha is last element in vec4, end alpha may be more like .5-.6, their recommendation is .6)
    vec4 baseColor = vec4(0.8, 0.8, 0.9, 1);
    
    // Sample the raindrop texture
    float alpha = texture(rainTexture, TexCoord).r;

    float lifetimeFade = 1.0 * (4.0 * (1.0 - VerticalFade) * VerticalFade);
    // (Optional: velocity-based motion-blur)
    // float motionBlur = length(Velocity) * 0.01;
    // alpha *= 1.0 + motionBlur;

    float finalAlpha = mix(0.8, 0.6, VerticalFade) * lifetimeFade;
    
    FragColor = baseColor * finalAlpha;
}
