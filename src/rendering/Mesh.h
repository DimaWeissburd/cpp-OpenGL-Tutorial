#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Texture.h"
#include "GLMemory.hpp"
#include "Models/Box.hpp"
#include "../Algorithms/Bounds.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static std::vector<struct Vertex> genList(float* vertices, int noVertices);
};
typedef struct Vertex Vertex;

class Mesh {
public:
    BoundingRegion br;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    ArrayObject VAO;

    std::vector<Texture> textures;
    aiColor4D diffuse;
    aiColor4D specular;

    Mesh();
    Mesh(BoundingRegion br, std::vector<Texture> textures = {});
    Mesh(BoundingRegion br, aiColor4D diff, aiColor4D spec);

    void loadData(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    void render(Shader shader, unsigned int noInstances);

    void cleanup();

private:
    unsigned int VBO, EBO;

    bool noTex;

    void setup();
};

#endif