#pragma once
#include <glm/glm.hpp>
#include <resources/resource_handle.hpp>

#if defined(BEE_PLATFORM_PC)
#include "resources/image/image_gl.hpp"
#endif

namespace bee
{
struct DebugData;


struct Sampler
{
    enum class Filter
    {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    enum class Wrap
    {
        Repeat,
        ClampToEdge,
        MirroredRepeat
    };

    Filter MagFilter = Filter::Linear;
    Filter MinFilter = Filter::LinearMipmapLinear;
    Wrap WrapS = Wrap::ClampToEdge;
    Wrap WrapT = Wrap::ClampToEdge;
};

enum TextureSlotIndex : uint32_t
{
    BASE_COLOR,
    EMISSIVE,
    NORMAL_MAP,
    OCCLUSION,
    METALLIC_ROUGHNESS,
    SUBSURFACE_OCCLUSION,
    MAX_TEXTURES
};

class Material
{
public:

    glm::vec4 BaseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // 16
    bool UseBaseTexture = false;                                    // 4

    glm::vec3 EmissiveFactor = glm::vec3(1.0f, 1.0f, 1.0f);  // 12
    bool UseEmissiveTexture = false;                         // 4

    float NormalTextureScale = 1.0f;  // 4
    bool UseNormalTexture = false;    // 4

    float OcclusionTextureStrength = 1.0f;  // 4
    bool UseOcclusionTexture = false;       // 4

    bool UseMetallicRoughnessTexture = false;  // 4
    float MetallicFactor = 0.0f;               // 4
    float RoughnessFactor = 1.0f;              // 4

    bool UseSubsurfaceTexture = false;
    float SubsurfaceFactor = 1.0f;

    bool IsUnlit = false;  // 4
    bool ReceiveShadows = true;
    bool DoubleSided = false;
    bool IsDitherable = true;

    glm::vec2 UVTilingFactor = glm::vec2(1.0f);
    glm::vec2 UVOffset = glm::vec2(0.0f);

    ResourceHandle<Image> BaseColorTexture;
    Sampler BaseColorSampler;

    ResourceHandle<Image> EmissiveTexture;
    Sampler EmissiveSampler;

    ResourceHandle<Image> NormalTexture;
    Sampler NormalSampler;

    ResourceHandle<Image> OcclusionTexture;
    Sampler OcclusionSampler;

    ResourceHandle<Image> MetallicRoughnessTexture;
    Sampler MetallicSampler;

    ResourceHandle<Image> SubsurfaceOcclusionTexture;
    Sampler SubsurfaceSampler;

    struct IBL
    {
        std::shared_ptr<Image> diffuse;
        std::shared_ptr<Image> specular;
        std::shared_ptr<Image> LUT;
    };

    static void Apply(std::shared_ptr<Material> material, std::shared_ptr<Shader> shader, const DebugData& debugFlags, const IBL& ibl, int specularMipCount);
    static void ApplyAlbedo(std::shared_ptr<Material> material);
};

}
