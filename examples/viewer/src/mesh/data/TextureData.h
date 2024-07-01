//
// Created by martin on 6/10/24.
//

#ifndef LOFT_TEXTUREDATA_H
#define LOFT_TEXTUREDATA_H

#include <string>

struct TextureData {
    std::string m_name;
    std::string m_path;
    bool m_isNormal;

    TextureData() :
            m_isNormal(false){

    }

    TextureData(std::string name, std::string path) :
            m_name(std::move(name)),
            m_path(std::move(path)),
            m_isNormal(false) {

    }
};

#endif //LOFT_TEXTUREDATA_H
