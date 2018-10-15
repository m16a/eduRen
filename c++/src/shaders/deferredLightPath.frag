#version 400 core

in vec2 TexCoords;
//in vec4 FragPosLightSpace;

struct PointLight
{
	vec3 pos;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
	
	samplerCube shadowMapTexture;
	float farPlane;
};

#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int nPointLights;

struct DirLight
{
	vec3 dir;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	sampler2D shadowMapTexture;
};

#define NR_DIR_LIGHTS 1
uniform DirLight dirLights[NR_DIR_LIGHTS];
uniform int nDirLights;

uniform vec3 camPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

out vec4 fColor;

void main()
{    
	// retrieve data from gbuffer
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;
	
	// then calculate lighting as usual
	vec3 lighting  = Diffuse * 0.0; // hard-coded ambient component
	vec3 viewDir  = normalize(camPos - FragPos);
	for(int i = 0; i < nPointLights; ++i)
	{
			// diffuse
			vec3 lightDir = normalize(pointLights[i].pos- FragPos);
			vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * pointLights[i].diffuse;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);  
			float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
			vec3 specular = pointLights[i].specular * spec * Specular;
			// attenuation
			float distance = length(pointLights[i].pos - FragPos);
			float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
			diffuse *= attenuation;
			specular *= attenuation;
			lighting += diffuse + specular;        
	}

	fColor = vec4(lighting, 1.0);
}
