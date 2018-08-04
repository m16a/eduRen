#version 400 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 camPos;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform Material material;

out vec4 fColor;

void main()
{
	vec3 ambient = lightCol * material.ambient;
	
	// diffuse 
  vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = lightCol * diff * material.diffuse;

	//specular
	vec3 viewDir = normalize(camPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * lightCol * material.specular;

	//vec3 finalCol =  diffuse;
	vec3 finalCol =  diffuse + specular + ambient;
	fColor = vec4( finalCol, 1.0);

	//fColor = vec4( norm, 1.0 );
}
