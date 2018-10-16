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

struct Color
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

void main()
{    
	Color res;
	res.ambient = vec4(0.0);
	res.diffuse = vec4(0.0);
	res.specular = vec4(0.0);

	// retrieve data from gbuffer
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;
	
	// then calculate lighting as usual
	vec3 viewDir  = normalize(camPos - FragPos);
	for(int i = 0; i < nPointLights; ++i)
	{
			// diffuse
			vec3 lightDir = normalize(pointLights[i].pos - FragPos);
			vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * pointLights[i].diffuse;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);  
			float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
			vec3 specular = pointLights[i].specular * spec * Specular;
			// attenuation
			float distance = length(pointLights[i].pos - FragPos);
			float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
			res.ambient += vec4(Diffuse * pointLights[i].ambient * attenuation, 1.0);
			diffuse *= attenuation;
			specular *= attenuation;
			res.diffuse += vec4(diffuse, 1.0);
			res.specular += vec4(specular, 1.0);
	}

	for (int i=0; i < nDirLights; ++i)
	{
		float diff = max(dot(Normal, dirLights[i].dir), 0.0);

		// specular shading
		vec3 reflectDir = reflect(-dirLights[i].dir, Normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);

		// combine results
		res.ambient  += vec4(dirLights[i].ambient * Diffuse.rgb, 1.0);
		res.diffuse  += vec4(dirLights[i].diffuse * diff * Diffuse.rgb, 1.0);
		res.specular += vec4(dirLights[i].specular * spec * Specular, 1.0);

	}

	float shadow = 0;
	fColor = vec4(vec3(res.ambient + (res.diffuse + res.specular) * (1.0 - shadow)), 1.0f);
}
