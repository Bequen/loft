#pragma once

#include "../mesh/data/SceneData.hpp"

/**
 * Defines interface to load objects from file
 */
class SceneLoader {
public:
    virtual const SceneData from_file(std::string path) = 0;
};
