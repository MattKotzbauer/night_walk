// rain.frag
#version 330 core
in vec2 TexCoord;
in vec2 Velocity;

out vec4 FragColor;

uniform sampler2D rainTexture;

void main() {
    // Basic raindrop color
    vec4 baseColor = vec4(0.8, 0.8, 0.9, 0.6);
    
    // Sample the raindrop texture
    float alpha = texture(rainTexture, TexCoord).r;
    
    // Optional: Add motion blur based on velocity
    float stretch = length(Velocity) * 0.01;
    alpha *= 1.0 + stretch;
    
    FragColor = baseColor * alpha;
}
