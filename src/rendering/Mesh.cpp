#include "Mesh.h"

std::vector<Vertex> Vertex::genList(float* vertices, int noVertices) {
    std::vector<Vertex> ret(noVertices);

    int stride = sizeof(Vertex) / sizeof(float);

    for (int i = 0; i < noVertices; i++) {
        ret[i].position = glm::vec3(
            vertices[i * stride],
            vertices[i * stride + 1],
            vertices[i * stride + 2]
        );

        ret[i].normal = glm::vec3(
            vertices[i * stride + 3],
            vertices[i * stride + 4],
            vertices[i * stride + 5]
        );

        ret[i].texCoord = glm::vec2(
            vertices[i * stride + 6],
            vertices[i * stride + 7]
        );
    }

    return ret;
}

Mesh::Mesh() {}

Mesh::Mesh(BoundingRegion boundingRegion, std::vector<Texture> textures)
    : boundingRegion(boundingRegion), textures(textures), noTex(false) {}

Mesh::Mesh(BoundingRegion boundingRegion, aiColor4D diffuse, aiColor4D specular)
    : boundingRegion(boundingRegion), diffuse(diffuse), specular(specular), noTex(true) {}

void Mesh::loadData(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices) {
    this->vertices = _vertices;
    this->indices = _indices;

    VAO.generate();
    VAO.bind();

    VAO["EBO"] = BufferObject(GL_ELEMENT_ARRAY_BUFFER);
    VAO["EBO"].generate();
    VAO["EBO"].bind();
    VAO["EBO"].setData<GLuint>(this->indices.size(), &this->indices[0], GL_STATIC_DRAW);

    VAO["VBO"] = BufferObject(GL_ARRAY_BUFFER);
    VAO["VBO"].generate();
    VAO["VBO"].bind();
    VAO["VBO"].setData<Vertex>(this->vertices.size(), &this->vertices[0], GL_STATIC_DRAW);

    VAO["VBO"].bind();
    VAO["VBO"].setAttPointer<GLfloat>(0, 3, GL_FLOAT, 8, 0);
    VAO["VBO"].setAttPointer<GLfloat>(1, 3, GL_FLOAT, 8, 3);
    VAO["VBO"].setAttPointer<GLfloat>(2, 2, GL_FLOAT, 8, 6);

    VAO["VBO"].clear();

    ArrayObject::clear();
}

void Mesh::render(Shader shader, unsigned int noInstances) {
    if (noTex) {
        shader.set4Float("material.diffuse", diffuse);
        shader.set4Float("material.specular", specular);
        shader.setInt("noTex", 1);
        shader.setFloat("material.shininess", 0.5f);
    } else {
        shader.setInt("noTex", 0);
        unsigned int diffuseIdx = 0;
        unsigned int specularIdx = 0;

        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string name;
            switch (textures[i].type) {
            case aiTextureType_DIFFUSE:
                name = "diffuse" + std::to_string(diffuseIdx++);
                break;
            case aiTextureType_SPECULAR:
                name = "specular" + std::to_string(specularIdx++);
                break;
            }

            shader.setInt(name, i);
            textures[i].bind();
        }
    }

    VAO.bind();
    VAO.draw(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, noInstances);
    ArrayObject::clear();

    glActiveTexture(GL_TEXTURE0);
}

void Mesh::setup() {
    VAO.generate();
    VAO.bind();

    VAO["EBO"] = BufferObject(GL_ELEMENT_ARRAY_BUFFER);
    VAO["EBO"].generate();
    VAO["EBO"].bind();
    VAO["EBO"].setData<GLuint>(indices.size(), &indices[0], GL_STATIC_DRAW);

    VAO["VBO"] = BufferObject(GL_ARRAY_BUFFER);
    VAO["VBO"].generate();
    VAO["VBO"].bind();
    VAO["VBO"].setData<Vertex>(vertices.size(), &vertices[0], GL_STATIC_DRAW);

    VAO["VBO"].setAttPointer<GLfloat>(0, 3, GL_FLOAT, 8, 0);
    VAO["VBO"].setAttPointer<GLfloat>(1, 3, GL_FLOAT, 8, 3);
    VAO["VBO"].setAttPointer<GLfloat>(2, 2, GL_FLOAT, 8, 6);

    VAO["VBO"].clear();

    ArrayObject::clear();
}

void Mesh::cleanup() {
    VAO.cleanup();
}