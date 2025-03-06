#include "precompiled/engine_precompiled.hpp"

#include <platform/opengl/open_gl.hpp>
#include "rendering/model_renderer.hpp"

#include "core/engine.hpp"
#include "rendering/shader_db.hpp"
#include "rendering/shader.hpp"
#include "platform/opengl/uniforms_gl.hpp"
#include "wind/wind.hpp"
#include <resources/image/image_gl.hpp>

#include "rendering/render.hpp"
#include "resources/resource_manager.hpp"
#include "resources/material/material.hpp"
#include "resources/mesh/mesh_gl.hpp"
#include "platform/opengl/gl_uniform.hpp"

class bee::ModelRenderer::Impl
{
public:
    void RenderCurrentInstances(std::shared_ptr<Mesh> mesh, int instances);

    Uniform<TransformsUBO> m_instancedTransformsUBO;
    Material::IBL m_ibl;
    uint32_t m_envCubemap = 0;
    uint32_t m_diffuseIBL = 0;
    uint32_t m_specularIBL = 0;
    uint32_t m_lutIBL = 0;
};

const bee::Material::IBL& bee::ModelRenderer::GetIBL() const
{
    return m_impl->m_ibl;
}

bee::ModelRenderer::ModelRenderer(const DebugData& debugFlags, uint32_t iblSpecularMipCount) : m_debugFlags(debugFlags), m_iblSpecularMipCount(iblSpecularMipCount)
{
    m_impl = std::make_unique<Impl>();

    m_toonData.toonImage = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/FToon.png", ImageFormat::RGBA8);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_toonData.doToonShading = false;
    m_toonData.toonPaletteId = 1;

    m_impl->m_instancedTransformsUBO.SetName("Transforms UBO (size:" + std::to_string(sizeof(TransformsUBO)) + ")");
    glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORMS_UBO_LOCATION, m_impl->m_instancedTransformsUBO.buffer); 

    m_impl->m_ibl.diffuse = std::make_shared<Image>(0, 0, 0, 0);
    m_impl->m_ibl.specular = std::make_shared<Image>(0, 0, 0, 0);
    m_impl->m_ibl.LUT = std::make_shared<Image>(0, 0, 0, 0);

    glGenTextures(1, &m_impl->m_envCubemap);
    glGenTextures(1, &m_impl->m_ibl.diffuse->handle);
    glGenTextures(1, &m_impl->m_ibl.specular->handle);
    glGenTextures(1, &m_impl->m_ibl.LUT->handle);
}

bee::ModelRenderer::~ModelRenderer() = default;

void* bee::ModelRenderer::InstancedTransformBuffer()
{
    return &m_impl->m_instancedTransformsUBO;
}

void bee::ModelRenderer::Render(const std::vector<Renderer::ObjectInfo>& objectsToDraw, const std::vector<Renderer::LightInfo>& lightsToDraw)
{
    PushDebugGL("Model pass");
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->Activate();
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_is_ditherable")->SetValue(true);

    glUniform1i(Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_windNoise")->GetLocation(), WIND_SAMPLER_LOCATION);
    glActiveTexture(GL_TEXTURE0 + WIND_SAMPLER_LOCATION);
    glBindTexture(GL_TEXTURE_3D, Engine.GetWindMap().GetWindImage().Retrieve()->handle);

    if (m_iblSpecularMipCount != -1)
        Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_ibl_specular_mip_count")->SetValue(m_iblSpecularMipCount);

    //Toon shading
    glActiveTexture(GL_TEXTURE0 + TOON_SAMPLER_LOCATION);
    glBindTexture(GL_TEXTURE_2D, m_toonData.toonImage.Retrieve()->handle);
    glUniform1i(TOON_SAMPLER_LOCATION, TOON_SAMPLER_LOCATION);
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_do_toon_shading")->SetValue(m_toonData.doToonShading);
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_toon_palette_index")->SetValue(m_toonData.toonPaletteId);
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_dither_distance")->SetValue(Engine.Renderer().GetDitherDistance());

    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_SSS_strength")->SetValue(m_subsurfaceData.strength/10.f);
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_SSS_distortion")->SetValue(m_subsurfaceData.distortion);
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->GetParameter("u_SSS_power")->SetValue(m_subsurfaceData.power);

    //Traverse the list, collecting transforms and instancing all meshes
    size_t drawPtr = 0;
    while (drawPtr < objectsToDraw.size())
    {
        auto batchMaterial = objectsToDraw.at(drawPtr).material;
        auto batchMesh = objectsToDraw.at(drawPtr).mesh;

        size_t instanceCount = 0;
        for (size_t lookPtr = drawPtr; lookPtr < objectsToDraw.size() && instanceCount < MAX_TRANSFORM_INSTANCES; ++lookPtr)
        {
            auto& nextElement = objectsToDraw.at(lookPtr);

            if (nextElement.material != batchMaterial || nextElement.mesh != batchMesh) break;

            m_impl->m_instancedTransformsUBO->bee_transforms[instanceCount].world = nextElement.transform;
            instanceCount++;
        }

        glEnable(GL_CULL_FACE);
        if (batchMaterial->DoubleSided)
        {
            glDisable(GL_CULL_FACE);
        }

        Material::Apply(batchMaterial, Engine.ShaderDB()[ShaderDB::Type::FORWARD], m_debugFlags, m_impl->m_ibl, m_iblSpecularMipCount);
        m_impl->RenderCurrentInstances(batchMesh, static_cast<int>(instanceCount));

        drawPtr += instanceCount;
        glEnable(GL_CULL_FACE);
    }
    PopDebugGL();
}

void bee::ModelRenderer::Impl::RenderCurrentInstances(std::shared_ptr<Mesh> mesh, int instances)
{
    m_instancedTransformsUBO.Patch();
    glBindVertexArray(mesh->vao_handle);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, mesh->index_format, nullptr, instances);
}