// Fragment Shader

#version 330 core

in vec3 OurColor;

out vec4 FragColor;

void main()
{
   FragColor = vec4(OurColor.xyz, 1.0);
}

