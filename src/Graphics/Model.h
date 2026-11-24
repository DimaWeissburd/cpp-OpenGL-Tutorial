#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Models/Box.hpp"
#include "../Physics/CollisionModel.h"
#include "../Physics/RigidBody.h"
#include "../Algorithms/Bounds.h"
#include "../Scene.h"

#define DYNAMIC         (unsigned int)1
#define CONST_INSTANCES (unsigned int)2
#define NO_TEX          (unsigned int)4

class Scene;

class Model {
public:
    std::string id;
    RigidBody rigidBody;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 size;
    float mass;

    std::vector<Mesh> meshes;
    CollisionModel* collision;
    std::vector<BoundingRegion> boundingRegions;

    std::vector<RigidBody*> instances;

    unsigned int maxNoInstances;
    unsigned int currentNoInstances;

    unsigned int switches;

    Model(std::string id, unsigned int maxNoInstances, glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass, unsigned int flags = 0);

    virtual void init();

    RigidBody* generateInstance(glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass);

    void initInstances();

    void loadModel(std::string path);

    void enableCollisionModel();

    void addMesh(Mesh* mesh);

    virtual void render(Shader shader, float dt, Scene* scene);

    void cleanup();

    void removeInstance(unsigned int idx);

    void removeInstance(std::string instanceId);

    unsigned int getIdx(std::string id);

protected:
    bool noTex;

    std::string directory;

    std::vector<Texture> textures_loaded;

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    Mesh processMesh(BoundingRegion br,
        unsigned int noVertices, float* vertices,
        unsigned int noIndices, unsigned int* indices,
        bool calcTanVectors = true,
        unsigned int nCollisionPoints = 0, float* collisionPoints = NULL,
        unsigned int nCollisionFaces = 0, unsigned int* collisionIndices = NULL,
        bool pad = false);
    std::vector<Texture> loadTextures(aiMaterial* mat, aiTextureType type);

    BufferObject modelVBO;
    BufferObject normalModelVBO;
};

#endif