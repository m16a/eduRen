#version 400 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightCol;
uniform vec3 lightPos;

out vec4 fColor;

void main()
{
	// diffuse 
  vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightCol;

	fColor = vec4( diffuse, 1.0 );
	//fColor = vec4( norm, 1.0 );
}
