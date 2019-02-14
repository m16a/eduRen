#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    TexCoords = vTexCoord;
    WorldPos = vec3(model * vec4(vPosition, 1.0));
    Normal = mat3(model) * vNormal;   

    gl_Position =  proj * view * vec4(WorldPos, 1.0);
}
