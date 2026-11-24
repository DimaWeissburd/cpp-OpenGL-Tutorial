#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexMemory.hpp"
#include "Models/Box.hpp"
#include "../Algorithms/Bounds.h"
#include "../Physics/CollisionMesh.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;

    static std::vector<struct Vertex> genList(float* vertices, int noVertices);

    static void averageVectors(glm::vec3& baseVec, glm::vec3 addition, unsigned char existingContributions);

    static void calcTanVectors(std::vector<Vertex>& list, std::vector<unsigned int>& indices);
};
typedef struct Vertex Vertex;

class Mesh {
public:
    BoundingRegion boundingRegion;
    CollisionMesh* collision;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    ArrayObject VAO;

    std::vector<Texture> textures;
    aiColor4D diffuse;
    aiColor4D specular;

    Mesh();
    Mesh(BoundingRegion boundingRegion);
    Mesh(BoundingRegion boundingRegion, std::vector<Texture> textures);
    Mesh(BoundingRegion boundingRegion, aiColor4D diff, aiColor4D spec);
    Mesh(BoundingRegion boundingRegion, Material m);

    void loadData(std::vector<Vertex> vertices, std::vector<unsigned int> indices, bool pad = false);

    void loadCollisionMesh(unsigned int nPoints, float* coordinates, unsigned int nFaces, unsigned int* indices);

    void setupTextures(std::vector<Texture> textures);

    void setupColors(aiColor4D diff, aiColor4D spec);

    void setupMaterial(Material mat);

    void render(Shader shader, unsigned int noInstances);

    void cleanup();

private:
    unsigned int VBO, EBO;

    bool noTex;

    void setup();
};

#endif