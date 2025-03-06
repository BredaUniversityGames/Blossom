
#pragma once
#include "render.hpp"
#include "resources/resource_handle.hpp"
#include "resources/material/material.hpp"

namespace bee
{
class ModelRenderer
{
public:
    struct ToonData
    {
        unsigned int toonPaletteId = 0;
        bool doToonShading = false;
        ResourceHandle<Image> toonImage;
    } m_toonData;

    struct SubsurfaceData
    {
        float strength = 0.0025f;
        float distortion = 0.75f;
        float power = 1.0f;
    } m_subsurfaceData;

    ModelRenderer(const DebugData& debugFlags, uint32_t iblSpecularMipCount);
    ~ModelRenderer();
    NON_COPYABLE(ModelRenderer);
    NON_MOVABLE(ModelRenderer);

    void Render(const std::vector<Renderer::ObjectInfo>& objectsToDraw, const std::vector<Renderer::LightInfo>& lightsToDraw);
    void* InstancedTransformBuffer();
    const ToonData& GetToonData() const { return m_toonData; }
    SubsurfaceData& GetSubsurfaceData() { return m_subsurfaceData; }
    const Material::IBL& GetIBL() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    const DebugData& m_debugFlags;
    const int32_t m_iblSpecularMipCount;
};

}
