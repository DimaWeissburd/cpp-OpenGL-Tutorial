#include "Model.h"
#include "../Physics/Environment.h"

Model::Model(BoundTypes boundType, glm::vec3 position, glm::vec3 rotation, glm::vec3 size, bool noTex)
    : boundType(boundType), size(size), rotation(rotation), noTex(noTex) {
    rb.position = position;
}

void Model::init() {}

void Model::render(Shader shader, float dt, Box* box, bool setModel, bool doRender) {
    rb.update(dt);

    if (setModel) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, rb.position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, size);
        shader.setMat4("model", model);
    }

    shader.setFloat("material.shininess", 0.5f);


    for (Mesh mesh : meshes) {
        mesh.render(shader, rb.position, size, box, doRender);
    }
}

void Model::cleanup() {
    for (Mesh mesh : meshes) {
        mesh.cleanup();
    }
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
        meshes.push_back(processMesh(mesh, scene));
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
        br.max = max;
    } else {
        br.center = BoundingRegion(min, max).calculateCenter();
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
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    if (noTex) {
        aiColor4D diff(1.0f);
        aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diff);

        aiColor4D spec(1.0f);
        aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec);

        return Mesh(br, vertices, indices, diff, spec);
    }

    std::vector<Texture> diffuseMaps = loadTextures(material, aiTextureType_DIFFUSE);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    std::vector<Texture> specularMaps = loadTextures(material, aiTextureType_SPECULAR);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    return Mesh(br, vertices, indices, textures);
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