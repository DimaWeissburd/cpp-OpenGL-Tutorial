#include "Model.h"
#include "../Physics/Environment.h"
#include "../Algorithms/States.hpp"

Model::Model(std::string id, BoundTypes boundType, unsigned int maxNoInstances, unsigned int flags)
    : id(id), boundType(boundType), switches(flags), currentNoInstances(0), maxNoInstances(maxNoInstances) {}

RigidBody* Model::generateInstance(glm::vec3 size, float mass, glm::vec3 position) {
    if (currentNoInstances >= maxNoInstances) {
        return nullptr;
    }
    instances.push_back(new RigidBody(id, size, mass, position));
    return instances[currentNoInstances++];
}

void Model::initInstances() {
    glm::vec3* positionData = nullptr;
    glm::vec3* sizeData = nullptr;
    GLenum usage = GL_DYNAMIC_DRAW;

    std::vector<glm::vec3> positions, sizes;

    if (States::isActive(&switches, CONST_INSTANCES)) {
        for (int i = 0; i < currentNoInstances; i++) {
            positions.push_back(instances[i]->position);
            sizes.push_back(instances[i]->size);
        }

        if (positions.size() > 0) {
            positionData = &positions[0];
            sizeData = &sizes[0];
        }

        usage = GL_STATIC_DRAW;
    }

    positionVBO = BufferObject(GL_ARRAY_BUFFER);
    positionVBO.generate();
    positionVBO.bind();
    positionVBO.setData<glm::vec3>(UPPER_BOUND, positionData, GL_DYNAMIC_DRAW);

    sizeVBO = BufferObject(GL_ARRAY_BUFFER);
    sizeVBO.generate();
    sizeVBO.bind();
    sizeVBO.setData<glm::vec3>(UPPER_BOUND, sizeData, GL_DYNAMIC_DRAW);

    for (unsigned int i = 0, size = meshes.size(); i < size; i++) {
        meshes[i].VAO.bind();
        positionVBO.bind();
        positionVBO.setAttPointer<glm::vec3>(3, 3, GL_FLOAT, 1, 0, 1);
        sizeVBO.bind();
        sizeVBO.setAttPointer<glm::vec3>(4, 3, GL_FLOAT, 1, 0, 1);
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

void Model::render(Shader shader, float dt, Scene* scene, bool setModel) {
    if (setModel) {
        shader.setMat4("model", glm::mat4(1.0f));
    }

    if (!States::isActive(&switches, CONST_INSTANCES)) {
        std::vector<glm::vec3> positions, sizes;

        bool doUpdate = States::isActive(&switches, DYNAMIC);

        for (int i = 0; i < currentNoInstances; i++) {
            if (doUpdate) {
                instances[i]->update(dt);
                States::activate(&instances[i]->state, INSTANCE_MOVED);
            } else {
                States::deactivate(&instances[i]->state, INSTANCE_MOVED);
            }

            positions.push_back(instances[i]->position);
            sizes.push_back(instances[i]->size);
        }

        positionVBO.bind();
        positionVBO.updateData<glm::vec3>(0, currentNoInstances, &positions[0]);

        sizeVBO.bind();
        sizeVBO.updateData<glm::vec3>(0, currentNoInstances, &sizes[0]);
    }

    shader.setFloat("material.shininess", 0.05f);

    for (unsigned int i = 0, noMeshes = meshes.size(); i < noMeshes; i++) {
        meshes[i].render(shader, currentNoInstances);
    }
}

void Model::cleanup() {
    for (Mesh mesh : meshes) {
        mesh.cleanup();
    }

    positionVBO.cleanup();
    sizeVBO.cleanup();
}

void Model::loadModel(std::string path) {
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Could not load model at " << path << std::endl << import.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of("/"));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh newMesh = processMesh(mesh, scene);
        meshes.push_back(newMesh);
        boundingRegions.push_back(newMesh.boundingRegion);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    BoundingRegion br(boundType);
    glm::vec3 min((float)(~0));
    glm::vec3 max(-(float)(~0));

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

        vertices.push_back(vertex);
    }

    if (boundType == BoundTypes::AABB) {
        br.min = min;
        br.ogMin = min;
        br.max = max;
        br.ogMax = max;
    } else {
        br.center = BoundingRegion(min, max).calculateCenter();
        br.ogCenter = br.center;
        float maxRadiusSquared = 0.0f;
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            float radiusSquared = 0.0f;
            for (int j = 0; j < 3; j++) {
                radiusSquared += (vertices[i].position[j] - br.center[j]) * (vertices[i].position[j] - br.center[j]);
            }
            if (radiusSquared > maxRadiusSquared) {
                maxRadiusSquared = radiusSquared;
            }
        }
        br.radius = sqrt(maxRadiusSquared);
        br.ogRadius = br.radius;
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh ret;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        if (States::isIndexActive<unsigned int>(&switches, NO_TEX)) {
            aiColor4D diff(1.0f);
            aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diff);
            aiColor4D spec(1.0f);
            aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec);

            ret = Mesh(br, diff, spec);
        }
        else {
            std::vector<Texture> diffuseMaps = loadTextures(material, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            std::vector<Texture> specularMaps = loadTextures(material, aiTextureType_SPECULAR);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            ret = Mesh(br, textures);
        }
    }

    ret.loadData(vertices, indices);
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