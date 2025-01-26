// rain.vert
#version 330 core
layout (location = 0) in vec2 aPos;         // Vertex position
layout (location = 1) in vec2 aTexCoord;    // Texture coordinates
layout (location = 2) in vec2 aOffset;      // Instance position
layout (location = 3) in vec2 aVelocity;    // Raindrop velocity for motion blur

out vec2 TexCoord;
out vec2 Velocity;

void main() {
    // Scale and position the raindrop
    vec2 pos = aPos + aOffset;
    
    // Convert from pixel coordinates to OpenGL coordinates (-1 to 1)
    vec2 normalizedPos = vec2(
        (pos.x / 320.0) * 2.0 - 1.0,  // Using InternalWidth
        (pos.y / 180.0) * 2.0 - 1.0   // Using  InternalHeight
    );
    
    gl_Position = vec4(normalizedPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Velocity = aVelocity;
}