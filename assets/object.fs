#version 330 core

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

out vec4 FragColor;

in vec3 FragPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform Material material;
uniform Light light;

uniform vec3 viewPosition;

void main() {
    vec3 norm = normalize(Normal);

	vec3 ambient = light.ambient * material.ambient;

	vec3 lightDir = normalize(light.position - FragPosition);
	vec3 diffuse = light.diffuse * (max(dot(norm, lightDir), 0.0) * material.diffuse);

	vec3 viewDir = normalize(viewPosition - FragPosition);
	vec3 reflectDir = reflect(-lightDir, norm);
	vec3 specular = light.specular * material.specular * pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);

	FragColor = vec4(vec3(ambient + diffuse + specular), 1.0);
}