// shader.vert (Vertex Shader)

#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform float screenWidth;
uniform float screenHeight;

out vec2 TexCoord;
out vec2 FragPos;

void main() {
  gl_Position = vec4(aPos, 1.0); 
  TexCoord = aTexCoord;
  // (Fragment position in screen space for lighting)
  FragPos = vec2((aPos.x + 1.0) * screenWidth / 2.0,
		 (aPos.y + 1.0) * screenHeight / 2.0);
}


