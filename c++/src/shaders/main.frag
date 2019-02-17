#version 400 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in mat3 TBN;

in vec3 TangentCamPos;
in vec3 TangentFragPos;

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

#define NR_POINT_LIGHTS 6
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

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform Material material;

struct Texture
{
	sampler2D diff;
	sampler2D spec;
	sampler2D reflection;
	sampler2D norm;
	sampler2D opacity;
};

uniform Texture inTexture;

uniform samplerCube skybox;
uniform mat4 rotfix;

uniform mat4 model;

out vec4 fColor;

// ---------------------- subroutines ------------------------

subroutine vec3 getNormal(vec2 uv);

subroutine (getNormal) vec3 getNormalSimple(vec2 uv)
{
	return normalize(Normal);
}

subroutine (getNormal) vec3 getNormalBumped(vec2 uv)
{
	//TODO: bad for performance, but less code
	vec3 n = normalize(texture(inTexture.norm, uv).rgb);
	n = normalize(n * 2.0 - 1.0);
	n = normalize(TBN * n);
	return n;
}

subroutine (getNormal) vec3 getNormalFromHeight(vec2 uv)
{
	const vec2 size = vec2(1.0,0.0);
	const ivec3 off = ivec3(-1,0,1);

	vec4 wave = texture(inTexture.norm, uv);
	float s11 = wave.x;
#if 0
	float s01 = textureOffset(inTexture.norm, uv, off.xy).x;
	float s21 = textureOffset(inTexture.norm, uv, off.zy).x;
	float s10 = textureOffset(inTexture.norm, uv, off.yx).x;
	float s12 = textureOffset(inTexture.norm, uv, off.yz).x;
#else
	float s01 = 1-textureOffset(inTexture.norm, uv, off.xy).x;
	float s21 = 1-textureOffset(inTexture.norm, uv, off.zy).x;
	float s10 = 1-textureOffset(inTexture.norm, uv, off.yx).x;
	float s12 = 1-textureOffset(inTexture.norm, uv, off.yz).x;
#endif

	float scale = 0.3;
	vec3 va = scale * normalize(vec3(size.xy,scale * (-s21+s01)));
	vec3 vb = scale * normalize(vec3(size.yx,scale * (s12-s10)));
	//vec4 bump = vec4( cross(va,vb), s11 );	
	vec3 n = normalize(cross(va, vb));
	n = normalize(TBN * n);

	return n;
}

subroutine uniform getNormal getNormalSelection;
// -----------------------------------------------------------
/*
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height =  texture(inTexture.norm, texCoords).r;    
    vec2 p = viewDir.xy / viewDir.z * (height * 0.1);
    return texCoords - p;    
} 

subroutine vec4 getHeight(sampler2D tex, vec2 uv);

subroutine (getHeight) vec4 getHeightEmpty(sampler2D tex, vec2 uv)
{
	return texture(tex, uv);
}

subroutine (getHeight) vec4 getHeightBumped(sampler2D tex, vec2 uv)
{
	//vec3 viewDir   = normalize(TangentCamPos - TangentFragPos);
  //vec2 texCoords = ParallaxMapping(uv,  viewDir);
	return texture(tex, uv);
}

subroutine uniform getHeight getHeightSelection;
 */

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height =  texture(inTexture.norm, texCoords).r;    
		height = 1.0 - height;
    vec2 p = viewDir.xy / viewDir.z * (height * 0.001);
    return texCoords - p;    
} 


vec4 getHeightBumped(sampler2D tex, vec2 uv)
{
	vec3 viewDir   = normalize(TangentCamPos - TangentFragPos);
  vec2 texCoords = ParallaxMapping(uv,  viewDir);
	return texture(tex, texCoords);
}

// -----------------------------------------------------------
struct Color
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

subroutine Color baseColor(vec2 uv);

subroutine (baseColor) Color plainColor(vec2 uv)
{
	Color c;
	c.ambient = vec4(material.diffuse, 1.0);
	c.diffuse = vec4(material.diffuse, 1.0);
	c.specular = vec4(material.specular, 1.0);
	c.shininess = 16;//material.shininess;
	return c;
}

subroutine (baseColor) Color textColor(vec2 uv)
{
	Color c;
	c.diffuse = texture(inTexture.diff, uv);
	c.ambient = c.diffuse;
	c.specular = texture(inTexture.spec, uv);
	c.shininess = 16;
	return c;
}

subroutine (baseColor) Color textHeightColor(vec2 uv)
{
	Color c;
	c.diffuse  = getHeightBumped(inTexture.diff, uv);
	c.ambient = c.diffuse;
	c.specular = getHeightBumped(inTexture.spec, uv);
	c.shininess = 16;
	return c;
}

subroutine uniform baseColor baseColorSelection;

// ----------------------reflection map-------------------------------------
subroutine vec3 reflectionMap(vec2 uv);
subroutine (reflectionMap) vec3 emptyReflectionMap(vec2 uv)
{
	return vec3(0.0, 0.0, 0.0);
}

subroutine (reflectionMap) vec3 reflectionTexture(vec2 uv)
{
	vec3 I = normalize(FragPos - camPos);
	vec3 R = reflect(I, getNormalSelection(uv));
	R = normalize(vec3( rotfix * vec4(R, 0.0)));

	return texture(inTexture.reflection, uv).rgb * texture(skybox, R).rgb;
}

subroutine (reflectionMap) vec3 reflectionColor(vec2 uv)
{
	vec3 I = normalize(FragPos - camPos);
	vec3 R = reflect(I, getNormalSelection(uv));
	R = normalize(vec3( rotfix * vec4(R, 0.0)));

	return material.diffuse * texture(skybox, R).rgb;
}

subroutine uniform reflectionMap reflectionMapSelection;


// ----------------------shadow map-------------------------------------
subroutine float shadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light);

subroutine (shadowMap) float emptyShadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light)
{
	return 0.0f;
}

subroutine (shadowMap) float globalShadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	float shadow = 0.0;

	if (nDirLights > 0)
	{
		if (projCoords.z <= 1.0)
		{
			// transform to [0,1] range
			projCoords = projCoords * 0.5 + 0.5;
			// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
			//float closestDepth = texture(shadowMapTexture, projCoords.xy).r; 
			
			// get depth of current fragment from light's perspective
			float currentDepth = projCoords.z;
			
			float bias = 0.0005;	
			//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
			// check whether current frag pos is in shadow
			//
			//float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

			vec2 texelSize = 1.0 / textureSize(light.shadowMapTexture, 0);
			for(int x = -1; x <= 1; ++x)
			{
					for(int y = -1; y <= 1; ++y)
					{
							float pcfDepth = texture(light.shadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
							shadow += currentDepth - bias > pcfDepth ? 0.5 : 0.0;        
					}    
			}
			shadow /= 9.0;
		}
	}

	//calculate omnidirectional shadows
	for (int i = 0; i < nPointLights; i++)
	{
    // get vector between fragment position and light position
    vec3 fragToLight = FragPos - pointLights[i].pos;

    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);

		if (currentDepth < pointLights[i].farPlane)
		{
			// use the light to fragment vector to sample from the depth map    
			float closestDepth = texture(pointLights[i].shadowMapTexture, (fragToLight)).r;
			// it is currently in linear range between [0,1]. Re-transform back to original value
			closestDepth *= pointLights[i].farPlane;
			// now test for shadows
			float bias = 0.5; 
			shadow += (currentDepth -  bias > closestDepth ? 0.5 : 0.0);
		}
	}

	return shadow;
}

subroutine uniform shadowMap shadowMapSelection;

// ---------------------- opacity ------------------------
subroutine float getOpacity(vec2 uv);

subroutine (getOpacity) float emptyOpacity(vec2 uv)
{
	return 1.0;
}

subroutine (getOpacity) float maskOpacity(vec2 uv)
{
	return texture(inTexture.opacity, uv).r;
}

subroutine uniform getOpacity opacitySelection;

// -----------------------------------------------------------

Color CalcPointLight(Color baseColor, PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	Color res;
	vec3 lightDir = normalize(light.pos - fragPos);

	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);

	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), baseColor.shininess);

	// attenuation
	float distance    = length(light.pos - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

	// combine results
	res.ambient  = vec4(light.ambient * baseColor.diffuse.rgb * attenuation, 1.0);
	res.diffuse  = vec4(light.diffuse * diff * baseColor.diffuse.rgb * attenuation, 1.0);
	res.specular = vec4(light.specular * spec * baseColor.specular.r * attenuation, 1.0);

	return res;
}

Color CalcDirLight(Color baseColor, DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	Color res;
	// diffuse shading
	float diff = max(dot(normal, light.dir), 0.0);

	// specular shading
	vec3 reflectDir = reflect(-light.dir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), baseColor.shininess);

	// combine results
	res.ambient  = vec4(light.ambient * baseColor.diffuse.rgb, 1.0);
	res.diffuse  = vec4(light.diffuse * diff * baseColor.diffuse.rgb, 1.0);
	res.specular = vec4(light.specular * spec * baseColor.specular.r, 1.0);

	return res;
}

void main()
{
	Color res;
	res.ambient = vec4(0.0);
	res.diffuse = vec4(0.0);
	res.specular = vec4(0.0);

	float opacity = opacitySelection(TexCoords);

	if (opacity < 0.03)
	    discard;

  vec3 norm = getNormalSelection(TexCoords);
	vec3 viewDir = normalize(camPos - FragPos);

	Color baseColor = baseColorSelection(TexCoords);

	for (int i = 0; i < nPointLights; i++)
	{
		Color tmp = CalcPointLight(baseColor, pointLights[i], norm, FragPos, viewDir); 
		res.ambient += tmp.ambient;
		res.diffuse += tmp.diffuse;
		res.specular += tmp.specular;
	}
	
	for (int i = 0; i < nDirLights; i++)
	{
		Color tmp = CalcDirLight(baseColor, dirLights[i], norm, FragPos, viewDir); 
		res.ambient += tmp.ambient;
		res.diffuse += tmp.diffuse;
		res.specular += tmp.specular;
	}

	res.diffuse += vec4(reflectionMapSelection(TexCoords), 0.0); 

	float shadow = shadowMapSelection(FragPosLightSpace, norm, dirLights[0]);
	//shadow = 0.0f;

	fColor = vec4(vec3(res.ambient + (res.diffuse + res.specular) * (1.0 - shadow)), 1.0f);

	//fColor = vec4(baseColor.diffuse);
	//fColor = vec4(norm, 1.0);
	//fColor = vec4(vec2(TexCoords), 0, 1.0);
}
