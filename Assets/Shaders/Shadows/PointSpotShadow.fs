#version 330 core

in vec4 FragPosition;

uniform vec3 lightPosition;
uniform float farPlane;

void main() {
	float lightDistance = length(FragPosition.xyz - lightPosition);
	lightDistance /= farPlane;
	gl_FragDepth = lightDistance;
}