#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

struct DirectionalLight {
	sampler2D depthBuffer;
};
uniform DirectionalLight directionalLight;

uniform sampler2D bufferTex;

float near = 0.1;
float far = 100.0;

void main() {
	FragColor = vec4(vec3(texture(directionalLight.depthBuffer, TexCoord).r), 1.0);
}