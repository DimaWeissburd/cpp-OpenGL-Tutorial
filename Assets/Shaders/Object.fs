struct Material {
	vec4 diffuse;
	vec4 specular;
	float shininess;
};
uniform Material material;

uniform bool noTex;
uniform bool noNormalMap;

uniform sampler2D diffuse0;
uniform sampler2D specular0;
uniform sampler2D normal0;

uniform sampler2D directionalLightBuffer;
uniform samplerCube pointLightBuffers[MAX_POINT_LIGHTS];
uniform sampler2D spotLightBuffers[MAX_SPOT_LIGHTS];

out vec4 FragColor;

in VS_OUT {
	vec3 FragPosition;
	vec2 TexCoord;
	TangentLights tanLights;
} fs_in;

vec4 calcDirectionalLight(vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap);
vec4 calcPointLight(int index, vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap);
vec4 calcSpotLight(int index, vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap);

#define NUM_SAMPLES 20
#define NUM_SAMPLESF 20.0
vec3 sampleOffsetDirections[NUM_SAMPLES] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1,  -1)
);

void main() {
	// properties
	vec3 normal = normalize(fs_in.tanLights.Normal);

	if (!noNormalMap) {
		normal = texture(normal0, fs_in.TexCoord).rgb;
		normal = normalize(normal * 2.0 - 1.0); // map from [0, 1] to [-1, 1]
	}

	vec3 viewVec = fs_in.tanLights.viewPosition - fs_in.tanLights.FragPosition;
	vec3 viewDir = normalize(viewVec);

	vec4 diffMap;
	vec4 specMap;

	if (noTex) {
		diffMap = material.diffuse;
		specMap = material.specular;
	} else {
		diffMap = texture(diffuse0, fs_in.TexCoord);
		specMap = texture(specular0, fs_in.TexCoord);
	}

	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);

	// directional
	result += calcDirectionalLight(normal, viewVec, viewDir, diffMap, specMap);

	// point lights
	for (int i = 0; i < nPointLights; i++) {
		result += calcPointLight(i, normal, viewVec, viewDir, diffMap, specMap);
	}

	// spot lights
	for (int i = 0; i < nSpotLights; i++) {
		result += calcSpotLight(i, normal, viewVec, viewDir, diffMap, specMap);
	}

	// gamma correction
	float gamma = 2.2;
	result.rgb = pow(result.rgb, vec3(1.0 / gamma));

	// depth testing
	float near = 0.1;
	float far = 100.0;
	float z = gl_FragCoord.z * 2.0 - 1.0; // transform to NDC [0, 1] => [-1, 1]
	float linearDepth = (2.0 * near * far) / (far + near - z * (far - near)); // take inverse of the projection matrix (perspective)
	float factor = (near + linearDepth) / (near - far); // convert back to [0, 1]

	result.rgb *= 1 - factor;

	FragColor = result;
}

float calcDirectionalLightShadow(vec3 normal, vec3 viewVec, vec3 lightDir) {
	vec4 fragPosLightSpace = directionalLight.lightSpaceMatrix * vec4(fs_in.FragPosition, 1.0);

	// perspective divide (transforming coordinates NDC)
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // [depth relative to light] => [-1, 1]

	// NDC to depth range
	projCoords = projCoords * 0.5 + 0.5; // [-1, 1] => [0, 1]

	// if too far from light, do not return any shadow
	if (projCoords.z > 1.0) {
		return 0.0;
	}

	// get depth of fragment
	float currentDepth = projCoords.z;

	// calculate bias
	float maxBias = 0.05;
	float minBias = 0.005;
	float bias = max(maxBias * (1.0 - dot(normal, lightDir)), minBias);

	// PCF
	float shadowSum = 0.0;
	vec2 texelSize = 1.0 / textureSize(directionalLightBuffer, 0);
	float viewDist = length(viewVec);
	float diskRadius = (1.0 + (viewDist / directionalLight.farPlane)) / 30.0;
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			float pcfDepth = texture(directionalLightBuffer,
				projCoords.xy + vec2(x, y) * texelSize * diskRadius).r;
			shadowSum += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}

	// return average
	return shadowSum / 9.0;
}

vec4 calcDirectionalLight(vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap) {
	// ambient
	vec4 ambient = directionalLight.ambient * diffMap;

	// diffuse
	vec3 lightDir = normalize(-fs_in.tanLights.directionalLightDirection);
	float diff = max(dot(normal, lightDir), 0.0);
	vec4 diffuse = directionalLight.diffuse * (diff * diffMap);

	// specular
	vec4 specular = vec4(0.0, 0.0, 0.0, 1.0);
	if (diff > 0) {
		// if diff <= 0, object is "behind" light

		float dotProd = 0.0;
		// calculate using Blinn-Phong model
		vec3 halfwayDir = normalize(lightDir + viewDir);
		dotProd = dot(normal, halfwayDir);

		float spec = pow(max(dotProd, 0.0), material.shininess * 128);
		specular = directionalLight.specular * (spec * specMap);
	}

	float shadow = calcDirectionalLightShadow(normal, viewVec, lightDir);

	return vec4(ambient + (1.0 - shadow) * (diffuse + specular));
}

float calcPointLightShadow(int index, vec3 normal, vec3 viewVec, vec3 lightDir) {
	// get vector from the light to the fragment
	vec3 lightToFrag = fs_in.FragPosition - pointLights[index].position;

	// get current depth
	float currentDepth = length(lightToFrag);

	// calculate bias
	float minBias = 0.005;
	float maxBias = 0.05;
	float bias = max(maxBias * (1.0 - dot(normal, lightDir)), minBias);

	// PCF
	float shadow = 0.0;
	float viewDist = length(viewVec);
	float diskRadius = (1.0 + (viewDist / pointLights[index].farPlane)) / 30.0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		float pcfDepth = texture(pointLightBuffers[index], 
			lightToFrag + sampleOffsetDirections[i] * diskRadius).r;
		pcfDepth *= pointLights[index].farPlane;

		if (currentDepth - bias > pcfDepth) {
			shadow += 1.0;
		}
	}

	shadow /= NUM_SAMPLESF;

	return shadow;
}

vec4 calcPointLight(int index, vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap) {
	// ambient
	vec4 ambient = pointLights[index].ambient * diffMap;

	// diffuse
	vec3 lightDir = normalize(fs_in.tanLights.pointLightPositions[index] - fs_in.tanLights.FragPosition);
	float diff = max(dot(normal, lightDir), 0.0);
	vec4 diffuse = pointLights[index].diffuse * (diff * diffMap);

	// specular
	vec4 specular = vec4(0.0, 0.0, 0.0, 1.0);
	if (diff > 0) {
		// if diff <= 0, object is "behind" light

		float dotProd = 0.0;
		// calculate using Blinn-Phong model
		vec3 halfwayDir = normalize(lightDir + viewDir);
		dotProd = dot(normal, halfwayDir);

		float spec = pow(max(dotProd, 0.0), material.shininess * 128);
		specular = pointLights[index].specular * (spec * specMap);
	}

	// calculate attenuation
	float dist = length(pointLights[index].position - fs_in.FragPosition);
	float attenuation = 1.0 / (pointLights[index].k0 + pointLights[index].k1 * dist + pointLights[index].k2 * (dist * dist));

	// apply attenuation
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	float shadow = calcPointLightShadow(index, normal, viewVec, lightDir);

	return vec4(ambient + (1.0 - shadow) * (diffuse + specular));
}

float calcSpotLightShadow(int index, vec3 normal, vec3 viewVec, vec3 lightDir) {
	vec4 fragPosLightSpace = spotLights[index].lightSpaceMatrix * vec4(fs_in.FragPosition, 1.0);

	// perspective divide (transform to NDC)
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // [near, far] => [-1, 1]

	// NDC to depth range
	projCoords = projCoords * 0.5 + 0.5; // [-1, 1] => [0, 1]

	// if too far from light, do not return any shadow
	if (projCoords.z > 1.0) {
		return 0.0;
	}

	// get depth of fragment
	float currentDepth = projCoords.z;

	// calculate bias
	float maxBias = 0.05;
	float minBias = 0.005;
	float bias = max(maxBias * (1.0 - dot(normal, lightDir)), minBias);

	// PCF
	float shadowSum = 0.0;
	vec2 texelSize = 1.0 / textureSize(spotLightBuffers[index], 0);
	float viewDist = length(viewVec);
	float diskRadius = (1.0 + (viewDist / spotLights[index].farPlane)) / 30.0;
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			float pcfDepth = texture(spotLightBuffers[index],
				projCoords.xy + vec2(x, y) * texelSize * diskRadius).r;
			pcfDepth *= spotLights[index].farPlane;

			shadowSum += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}

	// return average
	return shadowSum / 9.0;
}

vec4 calcSpotLight(int index, vec3 normal, vec3 viewVec, vec3 viewDir, vec4 diffMap, vec4 specMap) {
	vec3 lightDir = normalize(fs_in.tanLights.spotLightPositions[index] - fs_in.tanLights.FragPosition);
	float theta = dot(lightDir, normalize(-fs_in.tanLights.spotLightDirections[index]));

	// ambient
	vec4 ambient = spotLights[index].ambient * diffMap;

	if (theta > spotLights[index].outerCutOff) {
		// if in cutoff, render light
		// > because using cosines and not degrees

		// diffuse
		float diff = max(dot(normal, lightDir), 0.0);
		vec4 diffuse = spotLights[index].diffuse * (diff * diffMap);

		// specular
		vec4 specular = vec4(0.0, 0.0, 0.0, 1.0);
		if (diff > 0) {
			// if diff <= 0, object is "behind" light

			float dotProd = 0.0;
			// calculate using Blinn-Phong model
			vec3 halfwayDir = normalize(lightDir + viewDir);
			dotProd = dot(normal, halfwayDir);

			float spec = pow(max(dotProd, 0.0), material.shininess * 128);
			specular = spotLights[index].specular * (spec * specMap);
		}

		// calculate Intensity
		float intensity = clamp((theta - spotLights[index].outerCutOff) / (spotLights[index].cutOff - spotLights[index].outerCutOff), 0.0, 1.0);
		diffuse *= intensity;
		specular *= intensity;

		// calculate attenuation
		float dist = length(spotLights[index].position - fs_in.FragPosition);
		float attenuation = 1.0 / (spotLights[index].k0 + spotLights[index].k1 * dist + spotLights[index].k2 * (dist * dist));

		// apply attenuation
		ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;

		float shadow = calcSpotLightShadow(index, normal, viewVec, lightDir);

		return ambient + (1.0 - shadow) * (diffuse + specular);
	}
	else {
		// render just ambient
		return ambient;
	}
}