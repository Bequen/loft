#pragma once

#include "SceneLoader.hpp"

class GltfSceneLoader : public SceneLoader {
public:
    const SceneData from_file(std::string path) override;
};
