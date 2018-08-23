#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 camPos;
uniform mat4 rotfix;
uniform samplerCube skybox;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform Material material;
void main()
{             
	vec3 I = normalize(Position - camPos);
	vec3 R = reflect(I, normalize(Normal));
	R = normalize(vec3( rotfix * vec4(R, 0.0)));
	FragColor = vec4(material.diffuse * texture(skybox, R).rgb, 1.0);
}
