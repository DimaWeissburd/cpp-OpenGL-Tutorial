#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <string>
#include <vector>
#include "Shader.h"
#include "GLMemory.hpp"

class Scene;

class Cubemap {
public:
	unsigned int id;

	Cubemap();

	void generate();

	void bind();

	void loadTextures(std::string dir,
		std::string right   = "Right.png",
		std::string left    = "Left.png",
		std::string top     = "Top.png",
		std::string bottom  = "Bottom.png",
		std::string front   = "Front.png",
		std::string back    = "Back.png");

	void allocate(GLenum format, GLuint width, GLuint height, GLenum type);

	void init();

	void render(Shader shader, Scene* scene);

	void cleanup();

private:
	std::string dir;
	std::vector<std::string> faces;
	bool hasTextures;
	ArrayObject VAO;
};

#endif