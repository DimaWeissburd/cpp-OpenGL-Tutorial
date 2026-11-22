#ifndef BRICKWALL_HPP
#define BRICKWALL_HPP

#include "Plane.hpp"

class Brickwall : public Plane {
public:
    void init() {
        std::vector<Texture> textures = {
            Texture("Assets/Textures", "Brickwall/Brickwall_Diffuse.jpg", aiTextureType_DIFFUSE),
            Texture("Assets/Textures", "Brickwall/Brickwall_Normal.jpg", aiTextureType_NORMALS),
            Texture("Assets/Textures", "Brickwall/Brickwall_Specular.jpg", aiTextureType_SPECULAR)
        };

        for (Texture t : textures) {
            t.load();
        }

        Plane::init(textures);
    }
};

#endif