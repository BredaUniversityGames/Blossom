#pragma once
#include "resources/resource_handle.hpp"
#include "resources/material/material.hpp"

namespace bee
{

class IBLRenderer
{
public:
    IBLRenderer();
    ~IBLRenderer();

    NON_COPYABLE(IBLRenderer);
    NON_MOVABLE(IBLRenderer);

    void Render(std::shared_ptr<Image> envCubemap, const Material::IBL& ibl);

    uint32_t SpecularMipCount() const { return m_specularMipCount; }

    void SetTextureSizeDiffuse(uint32_t size) { m_textureSizeDiffuse = size; }
    void SetTextureSizeSpecular(uint32_t size) { m_textureSizeDiffuse = size; }
    void SetTextureSizeLut(uint32_t size) { m_textureSizeDiffuse = size; }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    uint32_t m_specularMipCount;
    uint32_t m_lowestMipLevel = 4;
    uint32_t m_textureSizeDiffuse = 128;
    uint32_t m_textureSizeSpecular = 512;
    uint32_t m_textureSizeLut = 1024;
};
    
}
