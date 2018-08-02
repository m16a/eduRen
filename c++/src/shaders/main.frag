#version 400 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec4 fColor;

void main()
{
	// diffuse 
  vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightCol;

	//specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(camPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
	vec3 specular = specularStrength  * spec * lightCol;

	//vec3 finalCol =  (specular) * vec3(1.0, 0.0, 0.0);
	vec3 finalCol =  (diffuse + specular) * vec3(0.9, 0.9, 0.9);
	fColor = vec4( finalCol, 1.0);
	//fColor = vec4( norm, 1.0 );
}
