#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTxt;

uniform vec3 samples[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 32;
float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1280.0/4.0, 800.0/4.0); 

uniform mat4 viewMat;
uniform mat4 vpMat;

void main()
{
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

	vec3 randomVec = normalize(texture(noiseTxt, TexCoords * noiseScale).xyz);
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	vec4 fragPosView = viewMat * vec4(fragPos, 1);

	// iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
			vec3 sample = TBN * samples[i];
			sample = fragPos + sample * radius; 
			
			vec4 offset = vec4(sample, 1.0);

			float sampleZ = (viewMat * offset).z; 

			//from world to clip-space
			offset = vpMat * offset;
			offset.xyz /= offset.w; // perspective divide
			offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

			vec3 tmp = texture(gPosition, offset.xy).xyz;
			vec4 tmp2 = viewMat * vec4(tmp, 1.0);
			float sampleDepth = tmp2.z; 

			float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPosView.z - sampleDepth));
			occlusion += ((sampleDepth >= sampleZ + bias) ? 1.0 : 0.0) * rangeCheck;           
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	FragColor = occlusion;
}
