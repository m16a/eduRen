#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
uniform float farPlane;

uniform samplerCube in_texture;

void main()
{    
    FragColor = vec4(vec3(texture(in_texture, TexCoords).r), 1.0f);
}
