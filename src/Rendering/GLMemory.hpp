#ifndef GLMEMORY_HPP
#define GLMEMORY_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>

class BufferObject {
public:
	GLuint value;
	GLenum type;

	BufferObject() {}

	BufferObject(GLenum type)
	: type(type) {}

	void generate() {
		glGenBuffers(1, &value);
	}

	void bind() {
		glBindBuffer(type, value);
	}

	template<typename T>
	void setData(GLuint noElements, T* data, GLenum usage) {
		glBufferData(type, noElements * sizeof(T), data, usage);
	}

	template<typename T>
	void updateData(GLintptr offset, GLuint noElements, T* data) {
		glBufferSubData(type, offset, noElements * sizeof(T), data);
	}

	template<typename T>
	void setAttPointer(GLuint idx, GLint size, GLenum type, GLuint stride, GLuint offset, GLuint divisor = 0) {
		glVertexAttribPointer(idx, size, type, GL_FALSE, stride * sizeof(T), (void*)(offset * sizeof(T)));
		glEnableVertexAttribArray(idx);
		if (divisor > 0) {
			glVertexAttribDivisor(idx, divisor);
		}
	}

	void clear() {
		glBindBuffer(type, 0);
	}

	void cleanup() {
		glDeleteBuffers(1, &value);
	}
};

class ArrayObject {
public:
	GLuint value;
	std::map<std::string, BufferObject> buffers;

	BufferObject& operator[](const std::string& key) {
		return buffers[key];
	}

	void generate() {
		glGenVertexArrays(1, &value);
	}

	void bind() {
		glBindVertexArray(value);
	}

	void draw(GLenum mode, GLuint count, GLenum type, GLint indices, GLuint instanceCount = 1) {
		glDrawElementsInstanced(mode, count, type, (void*)indices, instanceCount);
	}

	void cleanup() {
		glDeleteVertexArrays(1, &value);
		for (auto& pair : buffers) {
			pair.second.cleanup();
		}
	}

	static void clear() {
		glBindVertexArray(0);
	}
};

#endif