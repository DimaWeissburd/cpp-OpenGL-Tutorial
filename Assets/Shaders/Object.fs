#version 330 core

struct Material {
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform sampler2D diffuse0;
uniform sampler2D specular0;

struct DirectionalLight {
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    sampler2D depthBuffer;
    mat4 lightSpaceMatrix;
};
uniform DirectionalLight directionalLight;

#define MAX_POINT_LIGHTS 20
struct PointLight {
	vec3 position;
	float k0;
	float k1;
	float k2;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
    float farPlane;
    samplerCube depthBuffer;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int nPointLights;

#define MAX_SPOT_LIGHTS 5
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float k0;
    float k1;
    float k2;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float nearPlane;
    float farPlane;
    sampler2D depthBuffer;
    mat4 lightSpaceMatrix;
};
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int nSpotLights;

out vec4 FragColor;

in vec3 FragPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform Material material;
uniform int noTex;

uniform vec3 viewPosition;

uniform bool useGammaCorrection;

float calcDirectionalLightShadow(vec3 normal, vec3 lightDirection);
vec4 calcDirectionalLight(vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcPointLight(int index, vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
float calcSpotLightShadow(int index, vec3 normal, vec3 lightDirection);
vec4 calcSpotLight(int index, vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcSpecular(vec3 lightDirection, vec3 viewDirection, vec3 normal, float shininess, vec4 lightSpecular, vec4 specularMap);
float calcShadowPCF(sampler2D depthBuffer, vec3 normal, vec3 lightDirection, vec3 projectionCoords);

void main() {
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragPosition);

    vec4 diffuseMap = vec4(0.0);
    vec4 specularMap = vec4(0.0);

    if (noTex == 1) {
        diffuseMap = material.diffuse;
        specularMap = material.specular;
    } else {
        diffuseMap = texture(diffuse0, TexCoord);
        specularMap = texture(specular0, TexCoord);
    }

    vec4 result = vec4(0.0, 0.0, 0.0, 1.0);

    // Directional
    result += calcDirectionalLight(normal, viewDirection, diffuseMap, specularMap);

    // Point lights
    for (int i = 0; i < nPointLights; i++) {
        result += calcPointLight(i, normal, viewDirection, diffuseMap, specularMap);
    }

    // Spot lights
    for (int i = 0; i < nSpotLights; i++) {
        result += calcSpotLight(i, normal, viewDirection, diffuseMap, specularMap);
    }

    // Depth testing
    if (useGammaCorrection) {
        float gamma = 2.2;
        result.rgb = pow(result.rgb, vec3(1.0 / gamma));
    }

    float near = 0.1;
    float far = 100.0;
    float z = gl_FragCoord.z * 2.0 - 1.0; // Transform to NDC(Normalized Device Coordinated) [0, 1] => [-1, 1]
    float linearDepth = (2.0 * near * far) / (far + near - z * (far - near)); // The the inverse of the projection matrix (perspective)
    float factor = (near + linearDepth) / (near - far); // Convert back to [0, 1]

    result.rgb *= 1 - factor;

	FragColor = result;
}

float calcDirectionalLightShadow(vec3 normal, vec3 lightDirection) {
    vec4 fragPositionLightSpace = directionalLight.lightSpaceMatrix * vec4(FragPosition, 1.0);

    // Perspective divide (transforming coordinates NDC)
    vec3 projectionCoords = fragPositionLightSpace.xyz / fragPositionLightSpace.w; // [depth relative to light] => [-1, 1]

    // NDC to depth range
    projectionCoords = projectionCoords * 0.5 + 0.5; // [-1, 1] => [0, 1]

    // If too far from light, do not return any shadow
    if (projectionCoords.z > 1.0) {
        return 0.0;
    }

    float bias = calcShadowPCF(directionalLight.depthBuffer, normal, lightDirection, projectionCoords);
    return bias;
}

vec4 calcDirectionalLight(vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    // Ambient
	vec4 ambient = directionalLight.ambient * diffuseMap;

    // Diffuse
	vec3 lightDirection = normalize(-directionalLight.direction);
	vec4 diffuse = directionalLight.diffuse * diffuseMap;
	diffuse *= max(dot(normal, lightDirection), 0.0);

    // Specular
    vec4 specular = calcSpecular(lightDirection, viewDirection, normal, material.shininess, directionalLight.specular, specularMap);

    float shadow = calcDirectionalLightShadow(normal, lightDirection);

	return ambient + (1.0 - shadow) * (diffuse + specular);
}

float calcPointLightShadow(int index, vec3 normal, vec3 lightDirection) {
    vec3 lightToFrag = FragPosition - pointLights[index].position; // Get vector from light to the fragment
    float closestDepth = texture(pointLights[index].depthBuffer, lightToFrag).r; // Get depth from cubemap
    closestDepth *= pointLights[index].farPlane; // [0, 1] => original depth value
    float currentDepth = length(lightToFrag); // Get current depth
    // Calculate bias
    float maxBias = 0.05;
    float minBias = 0.005;
    float bias = max(maxBias * (1.0 - dot(normal, lightDirection)), minBias);
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

vec4 calcPointLight(int index, vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    // Ambient 
	vec4 ambient = pointLights[index].ambient * diffuseMap;

    // Diffuse
	vec3 lightDirection = normalize(pointLights[index].position - FragPosition);
	vec4 diffuse = pointLights[index].diffuse * diffuseMap;
	diffuse *= max(dot(normal, lightDirection), 0.0);

    // Specular
    vec4 specular = calcSpecular(lightDirection, viewDirection, normal, material.shininess, pointLights[index].specular, specularMap);

    // Attenuation
	float distance = length(pointLights[index].position - FragPosition);
	float attenuation = 1.0 / (pointLights[index].k0 + pointLights[index].k1 * distance + pointLights[index].k2 * distance * distance);

    ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

    //float shadow = calcPointLightShadow(index, normal, lightDirection);
    float shadow = 0.0; // Some issue here

	return vec4(ambient + (1.0 - shadow) * (diffuse + specular));
}

float calcSpotLightShadow(int index, vec3 normal, vec3 lightDirection) {
    vec4 fragPositionLightSpace = spotLights[index].lightSpaceMatrix * vec4(FragPosition, 1.0);

    // Perspective divide (transform to NDC)
    vec3 projectionCoords = fragPositionLightSpace.xyz / fragPositionLightSpace.w; // [near, far] => [-1, 1]

    // NDC to depth range
    projectionCoords = projectionCoords * 0.5 + 0.5; // [-1, 1] => [0, 1]

    // If too far from light, do not return any shadow
    if (projectionCoords.z > 1.0) {
        return 0.0;
    }

    // Get closest depth in depth buffer
    float closestDepth = texture(spotLights[index].depthBuffer, projectionCoords.xy).r;

    // Linearize Depth
    float z = closestDepth * 2.0 - 1.0;
    closestDepth = (2.0 * spotLights[index].nearPlane * spotLights[index].farPlane) /
        (spotLights[index].farPlane + spotLights[index].nearPlane
        - z * (spotLights[index].farPlane - spotLights[index].nearPlane));
        closestDepth /= spotLights[index].farPlane;

    float bias = calcShadowPCF(spotLights[index].depthBuffer, normal, lightDirection, projectionCoords);
    return bias;
}

vec4 calcSpotLight(int index, vec3 normal, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    vec3 lightDirection = normalize(spotLights[index].position - FragPosition);
    float theta = dot(lightDirection, normalize(-spotLights[index].direction));
    vec4 ambient = spotLights[index].ambient * diffuseMap;
    
    if (theta > spotLights[index].outerCutOff) {
        // Diffuse
        vec4 diffuse = spotLights[index].diffuse * diffuseMap;
        diffuse *= max(dot(normal, lightDirection), 0.0);

        // Specular
        vec4 specular = calcSpecular(lightDirection, viewDirection, normal, material.shininess, spotLights[index].specular, specularMap);

        // Intensity
        float intensity = (theta - spotLights[index].outerCutOff) / (spotLights[index].cutOff - spotLights[index].outerCutOff);
        intensity = clamp(intensity, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;

        float distance = length(spotLights[index].position - FragPosition);
        float attenuation = 1.0 / (spotLights[index].k0 + spotLights[index].k1 * distance + spotLights[index].k2 * distance * distance);

        ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;

        float shadow = calcSpotLightShadow(index, normal, lightDirection);

        return vec4(ambient + (1.0 - shadow) * (diffuse + specular));
    } else {
        return ambient;
    }
}

vec4 calcSpecular(vec3 lightDirection, vec3 viewDirection, vec3 normal, float shininess, vec4 lightSpecular, vec4 specularMap) {
    float diffuse = dot(normal, lightDirection);
    if (diffuse <= 0.0) { // the surface faces away from the light
        return vec4(0.0);
    }
    float dotProduct = 0.0;
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    dotProduct = dot(normal, halfwayDirection);
	float spec = pow(max(dotProduct, 0.0), shininess * 128.0);
    return lightSpecular * (spec * specularMap);
}

float calcShadowPCF(sampler2D depthBuffer, vec3 normal, vec3 lightDirection, vec3 projectionCoords) {
    float maxBias = 0.05;
    float minBias = 0.005;
    float bias = max(maxBias * (1.0 - dot(normal, lightDirection)), minBias);

    // PCF
    float shadowSum = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthBuffer, 0);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float pcfDepth = texture(depthBuffer, projectionCoords.xy + vec2(x, y) * texelSize).r;
            shadowSum += projectionCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadowSum / 9.0; // Return average
}