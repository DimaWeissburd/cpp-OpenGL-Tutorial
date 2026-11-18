#ifndef TEXT_H
#define TEXT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <map>
#include "VertexMemory.hpp"
#include "Shader.h"

typedef struct {
	unsigned int	textureId;
	glm::ivec2		size;
	glm::ivec2		bearing;
	unsigned int	advance;
} Character;

class TextRenderer {
public:
	TextRenderer();
	TextRenderer(int height);
	bool loadFont(FT_Library& ft, std::string path);
	void render(Shader shader, std::string text, float x, float y, glm::vec2 scale, glm::vec3 color);
	void cleanup();

private:
	int height;
	std::map<char, Character> chars;
	ArrayObject VAO;
};

#endif