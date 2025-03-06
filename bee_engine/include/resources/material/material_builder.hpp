#pragma once
#include <core/fileio.hpp>
#include <resources/resource_handle.hpp>
#include <string_view>

#include <resources/material/material.hpp>

namespace bee {


class MaterialBuilder
{
public:
    MaterialBuilder() 
    {
        images.resize(TextureSlotIndex::MAX_TEXTURES);
        samplers.resize(TextureSlotIndex::MAX_TEXTURES);
        factors.resize(TextureSlotIndex::MAX_TEXTURES, glm::vec4(1.0f));
    }

    ~MaterialBuilder() = default;

    MaterialBuilder& WithName(std::string_view name) {
        material_name = name;
        return *this;
    }

    MaterialBuilder& WithTexture(uint32_t index, ResourceHandle<Image> handle) {
        assert(index < TextureSlotIndex::MAX_TEXTURES);
        images[index] = handle;
        return *this;
    }

    MaterialBuilder& WithSampler(uint32_t index, Sampler sampler) {
        assert(index < TextureSlotIndex::MAX_TEXTURES);
        samplers[index] = sampler;
        return *this;
    }

    //Set the factor for an input texture
    //PBR: first component relates to metallic, second to roughness
    //NormalMap: only the first component scales the texture
    MaterialBuilder& WithFactor(uint32_t index, glm::vec4& multiplier) {
        assert(index < TextureSlotIndex::MAX_TEXTURES);
        factors[index] = multiplier;
        return *this;
    }

    MaterialBuilder& DoubleSided(bool isDoubleSided) {
        doubleSided = isDoubleSided;
        return *this;
    }

    MaterialBuilder& WithDithering(bool dithering) {
        this->dithering = dithering;
        return *this;
    }

    ResourceHandle<Material> Build() const;

private:
    std::string material_name{};
    std::vector<ResourceHandle<Image>> images;
    std::vector<Sampler> samplers;
    std::vector<glm::vec4> factors;
    bool dithering = true;
    bool doubleSided = false;
};

}