#include "text.h"

TextRenderer::TextRenderer() {}

TextRenderer::TextRenderer(int height)
	: height(height) {}

bool TextRenderer::loadFont(FT_Library& ft, std::string path) {
	FT_Face fontFace;

	if (FT_New_Face(ft, path.c_str(), 0, &fontFace)) {
		return false;
	}

	FT_Set_Pixel_Sizes(fontFace, 0, height);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (unsigned char c = 0; c < 128; c++) {
		if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER)) {
			continue;
		}

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			fontFace->glyph->bitmap.width,
			fontFace->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			fontFace->glyph->bitmap.buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		chars[c] = {
			texture,
			glm::ivec2(fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows),
			glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
			(unsigned int)fontFace->glyph->advance.x
		};
	}

	FT_Done_Face(fontFace);

	VAO.generate();
	VAO.bind();

	VAO["VBO"] = BufferObject(GL_ARRAY_BUFFER);
	VAO["VBO"].generate();
	VAO["VBO"].bind();

	VAO["VBO"].setData<GLfloat>(6 * 4, NULL, GL_DYNAMIC_DRAW);
	VAO["VBO"].setAttPointer<GLfloat>(0, 4, GL_FLOAT, 4, 0);

	VAO["VBO"].clear();

	ArrayObject::clear();

	return true;
}

void TextRenderer::render(Shader shader, std::string text, float x, float y, glm::vec2 scale, glm::vec3 color) {
	shader.activate();
	shader.set3Float("textColor", color);

	glActiveTexture(GL_TEXTURE0);

	VAO.bind();

	for (int i = 0, len = text.size(); i < len; i++) {
		Character c = chars[text[i]];

		float xPos = x + c.bearing.x * scale.x;
		float yPos = y - (c.size.y - c.bearing.y) * scale.y;

		float width = c.size.x * scale.x;
		float height = c.size.y * scale.y;

		float vertices[6 * 4] = {
				xPos        , yPos + height, 0.0f, 0.0f,
				xPos        , yPos         , 0.0f, 1.0f,
				xPos + width, yPos         , 1.0f, 1.0f,

				xPos        , yPos + height, 0.0f, 0.0f,
				xPos + width, yPos         , 1.0f, 1.0f,
				xPos + width, yPos + height, 1.0f, 0.0f
		};

		glBindTexture(GL_TEXTURE_2D, c.textureId);

		VAO["VBO"].bind();
		VAO["VBO"].updateData<GLfloat>(0, 6 * 4, vertices);
		VAO.draw(GL_TRIANGLES, 0, 6);

		x += (c.advance >> 6) * scale.x;
	}

	ArrayObject::clear();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::cleanup() {
	VAO.cleanup();
}