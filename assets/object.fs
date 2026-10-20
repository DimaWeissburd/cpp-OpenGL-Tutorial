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

vec4 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);
vec4 calcSpotLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap);

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
    result = calcDirectionalLight(norm, viewDirection, diffuseMap, specularMap);
    for (int i = 0; i < nPointLights; i++) {
        result += calcPointLight(i, norm, viewDirection, diffuseMap, specularMap);
    }
    for (int i = 0; i < nSpotLights; i++) {
        result += calcSpotLight(i, norm, viewDirection, diffuseMap, specularMap);
    }
	FragColor = result;
}

vec4 calcDirectionalLight(vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
	vec4 ambient = directionalLight.ambient * diffuseMap;
	vec3 lightDirection = normalize(-directionalLight.direction);
	vec4 diffuse = directionalLight.diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);
	vec3 reflectDirection = reflect(-lightDirection, norm);
	vec4 specular = directionalLight.specular * specularMap;
	specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128.0);
	return ambient + diffuse + specular;
}

vec4 calcPointLight(int index, vec3 norm, vec3 viewDirection, vec4 diffuseMap, vec4 specularMap) {
	vec4 ambient = pointLights[index].ambient * diffuseMap;
	vec3 lightDirection = normalize(pointLights[index].position - FragPosition);
	vec4 diffuse = pointLights[index].diffuse * diffuseMap;
	diffuse *= max(dot(norm, lightDirection), 0.0);
	vec3 reflectDirection = reflect(-lightDirection, norm);
	vec4 specular = pointLights[index].specular * specularMap;
	specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128.0);
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
        vec4 diffuse = spotLights[index].diffuse * diffuseMap;
        diffuse *= max(dot(norm, lightDirection), 0.0);
        vec3 reflectDirection = reflect(-lightDirection, norm);
        vec4 specular = spotLights[index].specular * specularMap;
        specular *= pow(max(dot(viewDirection, reflectDirection), 0.0), material.shininess * 128.0);
        float intensity = (theta - spotLights[index].outerCutOff) / (spotLights[index].cutOff - spotLights[index].outerCutOff);
        intensity = clamp(intensity, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;
        return (ambient + diffuse + specular) * attenuation;
    } else {
        return ambient * attenuation;
    }
}