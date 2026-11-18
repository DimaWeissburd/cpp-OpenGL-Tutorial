#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D bufferTex;

float near = 0.1;
float far = 100.0;

void main() {
	FragColor = texture(bufferTex, TexCoord);
}