#version 400 core

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec3 vNormal;
layout( location = 2 ) in vec2 vTexCoord;


uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main()
{
	Normal = vec3(model * vec4(vNormal, 0.0));
	FragPos = vec3(model * vPosition);
	TexCoords = vTexCoord;
	gl_Position = proj * view * model * vPosition;
}

