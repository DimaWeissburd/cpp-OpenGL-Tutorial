#include "Model.h"
#include "../Physics/Environment.h"
#include "../Algorithms/States.hpp"

Model::Model(std::string id, unsigned int maxNoInstances, glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass, unsigned int flags)
    : id(id), switches(flags), currentNoInstances(0), maxNoInstances(maxNoInstances), position(position), rotation(rotation), size(size), mass(mass), collision(nullptr) {}

RigidBody* Model::generateInstance(glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass) {
    if (currentNoInstances >= maxNoInstances) {
        return nullptr;
    }
    instances.push_back(new RigidBody(id, position, rotation, size, mass));
    return instances[currentNoInstances++];
}

void Model::initInstances() {
    GLenum usage = GL_DYNAMIC_DRAW;
    glm::mat4* modelData = nullptr;
    glm::mat3* normalModelData = nullptr;

    std::vector<glm::mat4> models(currentNoInstances);
    std::vector<glm::mat3> normalModels(currentNoInstances);

    if (States::isActive(&switches, CONST_INSTANCES)) {
        for (int i = 0; i < currentNoInstances; i++) {
            models[i] = instances[i]->model;
            normalModels[i] = instances[i]->normalModel;
        }

        if (currentNoInstances) {
            modelData = &models[0];
            normalModelData = &normalModels[0];
        }

        usage = GL_STATIC_DRAW;
    }

    modelVBO = BufferObject(GL_ARRAY_BUFFER);
    modelVBO.generate();
    modelVBO.bind();
    modelVBO.setData<glm::mat4>(UPPER_BOUND, modelData, usage);
    normalModelVBO = BufferObject(GL_ARRAY_BUFFER);
    normalModelVBO.generate();
    normalModelVBO.bind();
    normalModelVBO.setData<glm::mat3>(UPPER_BOUND, normalModelData, usage);

    for (unsigned int i = 0, size = meshes.size(); i < size; i++) {
        meshes[i].VAO.bind();

        modelVBO.bind();
        modelVBO.setAttPointer<glm::vec4>(4, 4, GL_FLOAT, 4, 0, 1);
        modelVBO.setAttPointer<glm::vec4>(5, 4, GL_FLOAT, 4, 1, 1);
        modelVBO.setAttPointer<glm::vec4>(6, 4, GL_FLOAT, 4, 2, 1);
        modelVBO.setAttPointer<glm::vec4>(7, 4, GL_FLOAT, 4, 3, 1);

        normalModelVBO.bind();
        normalModelVBO.setAttPointer<glm::vec3>(8, 3, GL_FLOAT, 3, 0, 1);
        normalModelVBO.setAttPointer<glm::vec3>(9, 3, GL_FLOAT, 3, 1, 1);
        normalModelVBO.setAttPointer<glm::vec3>(10, 3, GL_FLOAT, 3, 2, 1);

        ArrayObject::clear();
    }
}

void Model::removeInstance(unsigned int idx) {
    instances.erase(instances.begin() + idx);
    currentNoInstances--;
}

void Model::removeInstance(std::string instanceId) {
    int idx = getIdx(instanceId);
    if (idx != -1) {
        removeInstance(idx);
    }
}

unsigned int Model::getIdx(std::string id) {
    for (int i = 0; i < currentNoInstances; i++) {
        if (instances[i]->instanceId == id) {
            return i;
        }
    }
    return -1;
}

void Model::init() {}

void Model::render(Shader shader, float dt, Scene* scene) {
    if (!States::isActive(&switches, CONST_INSTANCES)) {
        std::vector<glm::mat4> models(currentNoInstances);
        std::vector<glm::mat3> normalModels(currentNoInstances);

        bool doUpdate = States::isActive(&switches, DYNAMIC);

        for (int i = 0; i < currentNoInstances; i++) {
            if (doUpdate) {
                instances[i]->update(dt);
                States::activate(&instances[i]->state, INSTANCE_MOVED);
            } else {
                States::deactivate(&instances[i]->state, INSTANCE_MOVED);
            }

            models[i] = instances[i]->model;
            normalModels[i] = instances[i]->normalModel;
        }

        if (currentNoInstances) {
            modelVBO.bind();
            modelVBO.updateData<glm::mat4>(0, currentNoInstances, &models[0]);
            normalModelVBO.bind();
            normalModelVBO.updateData<glm::mat3>(0, currentNoInstances, &normalModels[0]);
        }
    }

    shader.setFloat("material.shininess", 0.05f);

    for (unsigned int i = 0, noMeshes = meshes.size(); i < noMeshes; i++) {
        meshes[i].render(shader, currentNoInstances);
    }
}

void Model::cleanup() {
    for (unsigned int i = 0, len = instances.size(); i < len; i++) {
        if (instances[i]) {
            free(instances[i]);
        }
    }
    instances.clear();

    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].cleanup();
    }

    modelVBO.cleanup();
    normalModelVBO.cleanup();
}

void Model::loadModel(std::string path) {
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Could not load model at " << path << std::endl << import.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of("/"));
    processNode(scene->mRootNode, scene);
}

void Model::enableCollisionModel() {
    if (!this->collision) {
        this->collision = new CollisionModel(this);
    }
}

void Model::addMesh(Mesh* mesh) {
    meshes.push_back(*mesh);
    boundingRegions.push_back(mesh->boundingRegion);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh newMesh = processMesh(mesh, scene);
        addMesh(&newMesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    BoundingRegion boundingRegion(BoundTypes::SPHERE);
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::min());

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        for (int j = 0; j < 3; j++) {
            if (vertex.position[j] < min[j]) min[j] = vertex.position[j];
            if (vertex.position[j] > max[j]) max[j] = vertex.position[j];
        }

        vertex.normal = glm::vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z
        );

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        } else {
            vertex.texCoord = glm::vec2(0.0f);
        }

        vertex.tangent = {
            mesh->mTangents[i].x,
            mesh->mTangents[i].y,
            mesh->mTangents[i].z
        };

        vertices.push_back(vertex);
    }

    boundingRegion.center = (min + max) / 2.0f;
    boundingRegion.ogCenter = boundingRegion.center;
    boundingRegion.collisionMesh = NULL;
    float maxRadiusSquared = 0.0f;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        float radiusSquared = 0.0f;
        for (int j = 0; j < 3; j++) {
            radiusSquared += (vertices[i].position[j] - boundingRegion.center[j]) * (vertices[i].position[j] - boundingRegion.center[j]);
        }
        if (radiusSquared > maxRadiusSquared) {
            maxRadiusSquared = radiusSquared;
        }
    }
    boundingRegion.radius = sqrt(maxRadiusSquared);
    boundingRegion.ogRadius = boundingRegion.radius;

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh ret;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        if (States::isActive<unsigned int>(&switches, NO_TEX)) {
            aiColor4D diff(1.0f);
            if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &diff) != AI_SUCCESS) {
                aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diff);
            }
            aiColor4D spec(1.0f);
            aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec);
            
            ret = Mesh(boundingRegion, diff, spec);
        }
        else {
            std::vector<Texture> diffuseMaps = loadTextures(material, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            std::vector<Texture> specularMaps = loadTextures(material, aiTextureType_SPECULAR);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            std::vector<Texture> normalMaps = loadTextures(material, aiTextureType_NORMALS);
            if (normalMaps.empty())
                normalMaps = loadTextures(material, aiTextureType_HEIGHT); // .obj stores normal maps as "bump"
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            ret = Mesh(boundingRegion, textures);
        }
    }

    ret.loadData(vertices, indices);
    return ret;
}

Mesh Model::processMesh(BoundingRegion br,
    unsigned int noVertices, float* vertices,
    unsigned int noIndices, unsigned int* indices,
    bool calcTanVectors,
    unsigned int nCollisionPoints, float* collisionPoints,
    unsigned int nCollisionFaces, unsigned int* collisionIndices,
    bool pad) {
    std::vector<Vertex> vertexList = Vertex::genList(vertices, noVertices);

    std::vector<unsigned int> indexList(noIndices);
    if (indices) {
        memcpy(indexList.data(), indices, noIndices * sizeof(unsigned int));
    } else {
        for (unsigned int i = 0; i < noIndices; i++) {
            indexList[i] = i;
        }
    }

    if (calcTanVectors) {
        Vertex::calcTanVectors(vertexList, indexList);
    }

    Mesh ret(br);
    ret.loadData(vertexList, indexList, pad);

    if (nCollisionPoints) {
        enableCollisionModel();
        ret.loadCollisionMesh(nCollisionPoints, collisionPoints, nCollisionFaces, collisionIndices);
    }

    return ret;
}

std::vector<Texture> Model::loadTextures(aiMaterial* material, aiTextureType type) {
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        aiString str;
        material->GetTexture(type, i, &str);

        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }

        if (!skip) {
            Texture texture(directory, str.C_Str(), type);
            texture.load(false);
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }

    return textures;
}