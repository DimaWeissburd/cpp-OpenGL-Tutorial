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

float calcDirectionalLightShadow();
vec4 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcSpotLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcSpecular(vec3 lightDirection, vec3 viewDirection, vec3 norm, float shininess, vec4 lightSpecular, vec4 specularMap);

void main() {
    vec3 norm = normalize(Normal);
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

    vec4 result;

    // Directional
    result = calcDirectionalLight(norm, viewDirection, diffuseMap, specularMap);

    // Point lights
    for (int i = 0; i < nPointLights; i++) {
        result += calcPointLight(i, norm, viewDirection, diffuseMap, specularMap);
    }

    // Spot lights
    for (int i = 0; i < nSpotLights; i++) {
        result += calcSpotLight(i, norm, viewDirection, diffuseMap, specularMap);
    }

    // Depth testing
    if (useGammaCorrection) {
        float gamma = 2.2;
        result.rgb = pow(result.rgb, vec3(1.0 / gamma));
    }

    float near = 0.1;
    float far = 100.0;
    float z = gl_FragCoord.z * 2.0 - 1.0; // Transform to NDC(Normalized Device Coordinated) [0, 1] => [-1, 1]
    float linearDepth = (2.0 * near * far) / (z * (far - near) - (far + near)); // The the inverse of the projection matrix (perspective)
    float factor = (near + linearDepth) / (near - far); // Convert back to [0, 1]

    result.rgb *= 1 - factor;

	FragColor = result;
}

float calcDirectionalLightShadow() {
    vec4 fragPositionLightSpace = directionalLight.lightSpaceMatrix * vec4(FragPosition, 1.0);

    // Perspective divide (transforming coordinates NDC)
    vec3 projectionCoords = fragPositionLightSpace.xyz / fragPositionLightSpace.w; // [depth relative to light] => [-1, 1]

    // NDC to depth range
    projectionCoords = projectionCoords * 0.5 + 0.5; // [-1, 1] => [0, 1]

    // Get closest depth in depth buffer
    float closestDepth = texture(directionalLight.depthBuffer, projectionCoords.xy).r;

    // Get depth of fragment
    float currentDepth = projectionCoords.z;

    // If depth is greater (further), return 1
    return currentDepth > closestDepth ? 1.0 : 0.0;
}

vec4 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    // Ambient
	vec4 ambient = directionalLight.ambient * diffuseMap;

    // Diffuse
	vec3 lightDirection = normalize(-directionalLight.direction);
	vec4 diffuse = directionalLight.diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);

    // Specular
    vec4 specular = calcSpecular(lightDirection, viewDirection, norm, material.shininess, directionalLight.specular, specularMap);

    float shadow = calcDirectionalLightShadow();

	return ambient + (1.0 - shadow) * (diffuse + specular);
}

vec4 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    // Ambient 
	vec4 ambient = pointLights[index].ambient * diffuseMap;

    // Diffuse
	vec3 lightDirection = normalize(pointLights[index].position - FragPosition);
	vec4 diffuse = pointLights[index].diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);

    // Specular
    vec4 specular = calcSpecular(lightDirection, viewDirection, norm, material.shininess, pointLights[index].specular, specularMap);

    // Attenuation
	float distance = length(pointLights[index].position - FragPosition);
	float attenuation = 1.0 / (pointLights[index].k0 + pointLights[index].k1 * distance + pointLights[index].k2 * distance * distance);

	return (ambient + diffuse + specular) * attenuation;
}

vec4 calcSpotLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
    vec3 lightDirection = normalize(spotLights[index].position - FragPosition);
    float theta = dot(lightDirection, normalize(-spotLights[index].direction));
    vec4 ambient = spotLights[index].ambient * diffuseMap;
    float distance = length(spotLights[index].position - FragPosition);
    float attenuation = 1.0 / (spotLights[index].k0 + spotLights[index].k1 * distance + spotLights[index].k2 * distance * distance);
    if (theta > spotLights[index].outerCutOff) {
        // Diffuse
        vec4 diffuse = spotLights[index].diffuse * diffuseMap;
        diffuse *= max(dot(norm, lightDirection), 0.0);

        // Specular
        vec4 specular = calcSpecular(lightDirection, viewDirection, norm, material.shininess, spotLights[index].specular, specularMap);

        // Intensity
        float intensity = (theta - spotLights[index].outerCutOff) / (spotLights[index].cutOff - spotLights[index].outerCutOff);
        intensity = clamp(intensity, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;

        return (ambient + diffuse + specular) * attenuation;
    } else {
        return ambient * attenuation;
    }
}

vec4 calcSpecular(vec3 lightDirection, vec3 viewDirection, vec3 norm, float shininess, vec4 lightSpecular, vec4 specularMap) {
    float diffuse = dot(norm, lightDirection);
    if (diffuse <= 0.0) { // the surface faces away from the light
        return vec4(0.0);
    }
    float dotProduct = 0.0;
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    dotProduct = dot(norm, halfwayDirection);
	float spec = pow(max(dotProduct, 0.0), shininess * 128.0);
    return lightSpecular * (spec * specularMap);
}