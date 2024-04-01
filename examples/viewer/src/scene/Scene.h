#pragma once

#include "Gpu.hpp"
#include "../mesh/SceneData.hpp"

class Scene {
private:
    Gpu *m_pGpu;

public:
    Scene(Gpu *pGpu);

    void add_scene_data(SceneData *pData);
};
