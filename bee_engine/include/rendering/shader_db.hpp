#pragma once
#include <cassert>
#include <memory>
#include <unordered_map>

#include "code_utils/bee_utils.hpp"

namespace bee
{
class Shader;

class ShaderDB
{
public:
    enum class Type
    {
        FORWARD,
        TERRAIN,
        TERRAIN_SHADOW,
        UI,
        SKYBOX,
        GRASS,
        GRASS_COMPUTE,
        SHADOW,
        TONEMAPPING,
        FILTER_IBL,
        FILTER_IBL_DIFFUSE,
        FILTER_IBL_SPECULAR,
        FILTER_IBL_LOOKUP,
        EQUIRECTANGULAR_TO_CUBEMAP,
        MIPMAP,
        DOF_PASSTHROUGH,
        FULLSCREEN,
        BLOOM_THRESHOLD,
        BLOOM_GAUSSIAN,
        BLOOM_FINAL,
        CLEAR_TEX,
        WRITE_DISPLACMENTS,
        GAUSSIAN_9TAP_FILTER,
        DOF_COMPOSITE,
        POPULATE_3DTEX_PERLIN,
    };

    ShaderDB();
    ~ShaderDB();
    NON_COPYABLE(ShaderDB);
    NON_MOVABLE(ShaderDB);

    std::shared_ptr<bee::Shader> Get(Type type)
    {
        assert(m_shaders.find(type) != m_shaders.end() && "Shader type not stored in the shader DB!");
        return m_shaders[type];
    }
    std::shared_ptr<bee::Shader> operator[] (Type type) { return Get(type); }

private:
    std::unordered_map<Type, std::shared_ptr<bee::Shader>> m_shaders;
};
}
