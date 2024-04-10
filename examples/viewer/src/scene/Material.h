#pragma once

struct Material {
    float albedo[4];
    float bsdf[4];

    int colorTexture;
    float colorTextureBlend;

    int normalTexture;
    float normalTextureBlend;

    int pbrTexture;
    float pbrTextureBlend;

    int padding[2];
};