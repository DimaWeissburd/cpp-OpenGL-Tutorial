#version 330 core

struct Material {
	vec3 ambient;
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirectionalLight directionalLight;

#define MAX_POINT_LIGHTS 20
struct PointLight {
	vec3 position;
	float k0;
	float k1;
	float k2;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int noPointLights;

#define MAX_SPOT_LIGHTS 5
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float k0;
    float k1;
    float k2;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int noSpotLights;

out vec4 FragColor;

in vec3 FragPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform Material material;

uniform vec3 viewPosition;

vec3 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap);
vec3 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap);
vec3 calcSpotLight(int index, vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap);

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragPosition);
    vec3 diffuseMap = vec3(texture(material.diffuse, TexCoord));
    vec3 specularMap = vec3(texture(material.specular, TexCoord));
    vec3 result;
    result = calcDirectionalLight(norm, viewDirection, diffuseMap, specularMap);
    for (int i = 0; i < noPointLights; i++) {
        result += calcPointLight(i, norm, viewDirection, diffuseMap, specularMap);
    }
    for (int i = 0; i < noSpotLights; i++) {
        result += calcSpotLight(i, norm, viewDirection, diffuseMap, specularMap);
    }
	FragColor = vec4(result, 1.0);
}

vec3 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap) {
	vec3 ambient = directionalLight.ambient * diffuseMap;
	vec3 lightDirection = normalize(-directionalLight.direction);
	vec3 diffuse = directionalLight.diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);
	vec3 reflectDirection = reflect(-lightDirection, norm);
	vec3 specular = directionalLight.specular * specularMap;
	specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128);
	return vec3(ambient + diffuse + specular);
}

vec3 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap) {
	vec3 ambient = pointLights[index].ambient * diffuseMap;
	vec3 lightDirection = normalize(pointLights[index].position - FragPosition);
	vec3 diffuse = pointLights[index].diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);
	vec3 reflectDirection = reflect(-lightDirection, norm);
	vec3 specular = pointLights[index].specular * specularMap;
	specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128);
	float distance = length(pointLights[index].position - FragPosition);
	float attenuation = 1.0 / (pointLights[index].k0 + pointLights[index].k1 * distance + pointLights[index].k2 * distance * distance);
	return vec3(ambient + diffuse + specular) * attenuation;
}

vec3 calcSpotLight(int index, vec3 norm, vec3 viewDirection, vec3 diffuseMap, vec3 specularMap) {
    vec3 lightDirection = normalize(spotLights[index].position - FragPosition);
    float theta = dot(lightDirection, normalize(-spotLights[index].direction));
    vec3 ambient = spotLights[index].ambient * diffuseMap;
    float distance = length(spotLights[index].position - FragPosition);
    float attenuation = 1.0 / (spotLights[index].k0 + spotLights[index].k1 * distance + spotLights[index].k2 * distance * distance);
    if (theta > spotLights[index].outerCutOff) {
        vec3 diffuse = spotLights[index].diffuse * diffuseMap;
        diffuse *= max(dot(norm, lightDirection), 0.0);
        vec3 reflectDirection = reflect(-lightDirection, norm);
        vec3 specular = spotLights[index].specular * specularMap;
        specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128);
        float intensity = (theta - spotLights[index].outerCutOff) / (spotLights[index].cutOff - spotLights[index].outerCutOff);
        intensity = clamp(intensity, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;
        return vec3(ambient + diffuse + specular) * attenuation;
    } else {
        return ambient * attenuation;
    }
}