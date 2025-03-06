#include "precompiled/engine_precompiled.hpp"

#include <platform/opengl/open_gl.hpp>

#include "platform/opengl/shader_gl.hpp"
#include "rendering/render.hpp"
#include "resources/image/image_gl.hpp"
#include "resources/material/material.hpp"
#include "platform/opengl/uniforms_gl.hpp"

namespace bee::internal
{
void SetTexture(std::shared_ptr<Image> texture, Sampler sampler, int location);
int32_t SamplerTypeToGL(Sampler::Filter filter);
int32_t SamplerTypeToGL(Sampler::Wrap wrap);
}

void bee::Material::Apply(std::shared_ptr<Material> material, std::shared_ptr<Shader> shader, const DebugData& debugFlags, const IBL& ibl, int iblSpecularMipCount)
{
    // if (material != m_currentMaterial)
    {
        if (material->UseBaseTexture)
            internal::SetTexture(material->BaseColorTexture.Retrieve(), material->BaseColorSampler, BASE_COLOR_SAMPLER_LOCATION);

        if (material->UseNormalTexture)
            internal::SetTexture(material->NormalTexture.Retrieve(), material->NormalSampler, NORMAL_SAMPLER_LOCATION);         

        if (material->UseMetallicRoughnessTexture)
            internal::SetTexture(material->MetallicRoughnessTexture.Retrieve(), material->MetallicSampler, ORM_SAMPLER_LOCATION);

        if (material->UseOcclusionTexture)
            internal::SetTexture(material->OcclusionTexture.Retrieve(), material->OcclusionSampler, OCCLUSION_SAMPLER_LOCATION);

        if (material->UseEmissiveTexture)
            internal::SetTexture(material->EmissiveTexture.Retrieve(), material->EmissiveSampler, EMISSIVE_SAMPLER_LOCATION);

        if (material->UseSubsurfaceTexture)
            internal::SetTexture(material->SubsurfaceOcclusionTexture.Retrieve(), material->SubsurfaceSampler, SUBSURFACE_SAMPLER_LOCATION);

        glActiveTexture(GL_TEXTURE0 + SPECULAR_SAMPER_LOCATION);
        glBindTexture(GL_TEXTURE_CUBE_MAP,  ibl.specular->handle);
        glUniform1i(SPECULAR_SAMPER_LOCATION, SPECULAR_SAMPER_LOCATION);

        glActiveTexture(GL_TEXTURE0 + DIFFUSE_SAMPER_LOCATION);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.diffuse->handle);
        glUniform1i(DIFFUSE_SAMPER_LOCATION, DIFFUSE_SAMPER_LOCATION);

        glActiveTexture(GL_TEXTURE0 + LUT_SAMPER_LOCATION);
        glBindTexture(GL_TEXTURE_2D, ibl.LUT->handle);
        glUniform1i(LUT_SAMPER_LOCATION, LUT_SAMPER_LOCATION);
    }

    shader->GetParameter("base_color_factor")->SetValue(material->BaseColorFactor);
    shader->GetParameter("use_base_texture")->SetValue(material->UseBaseTexture);
    shader->GetParameter("use_occlusion_texture")->SetValue(material->UseBaseTexture);
    shader->GetParameter("use_metallic_roughness_texture")->SetValue(material->UseMetallicRoughnessTexture);
    shader->GetParameter("use_normal_texture")->SetValue(material->UseNormalTexture);
    shader->GetParameter("metallic_factor")->SetValue(material->MetallicFactor);
    shader->GetParameter("roughness_factor")->SetValue(material->RoughnessFactor);
    shader->GetParameter("use_emissive_texture")->SetValue(material->UseEmissiveTexture);
    shader->GetParameter("is_unlit")->SetValue(material->IsUnlit);
    shader->GetParameter("u_receive_shadows")->SetValue(material->ReceiveShadows);
    shader->GetParameter("u_ibl_specular_mip_count")->SetValue(iblSpecularMipCount);
    shader->GetParameter("u_is_ditherable")->SetValue(material->IsDitherable);
    shader->GetParameter("u_double_sided")->SetValue(material->DoubleSided);
    shader->GetParameter("subsurface_factor")->SetValue(material->SubsurfaceFactor);

#ifdef BEE_DEBUG
    shader->GetParameter("debug_base_color")->SetValue(debugFlags.BaseColor);
    shader->GetParameter("debug_normals")->SetValue(debugFlags.Normals);
    shader->GetParameter("debug_normal_map")->SetValue(debugFlags.NormalMap);
    shader->GetParameter("debug_metallic")->SetValue(debugFlags.Metallic);
    shader->GetParameter("debug_roughness")->SetValue(debugFlags.Roughness);
    shader->GetParameter("debug_emissive")->SetValue(debugFlags.Emissive);
    shader->GetParameter("debug_occlusion")->SetValue(debugFlags.Occlusion);
    shader->GetParameter("debug_displacement_pivot")->SetValue(debugFlags.DisplacementPivot);
    shader->GetParameter("debug_wind_mask")->SetValue(debugFlags.WindMask);
#endif
}

void bee::Material::ApplyAlbedo(std::shared_ptr<Material> material)
{
    if(material->UseBaseTexture)
        internal::SetTexture(material->BaseColorTexture.Retrieve(), material->BaseColorSampler, BASE_COLOR_SAMPLER_LOCATION);
}

void bee::internal::SetTexture(std::shared_ptr<Image> texture, Sampler sampler, int location)
{
    glActiveTexture(GL_TEXTURE0 + location);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glUniform1i(location, location);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SamplerTypeToGL(sampler.MinFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, SamplerTypeToGL(sampler.MagFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, SamplerTypeToGL(sampler.WrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, SamplerTypeToGL(sampler.WrapT));
}

int bee::internal::SamplerTypeToGL(Sampler::Filter filter)
{
    switch (filter)
    {
    case Sampler::Filter::Nearest:
        return GL_NEAREST;
    case Sampler::Filter::Linear:
        return GL_LINEAR;
    case Sampler::Filter::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case Sampler::Filter::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case Sampler::Filter::NearestMipmapLinear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case Sampler::Filter::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    }
    return 0;
}

int bee::internal::SamplerTypeToGL(bee::Sampler::Wrap wrap)
{
    switch (wrap)
    {
    case Sampler::Wrap::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case Sampler::Wrap::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case Sampler::Wrap::Repeat:
        return GL_REPEAT;
    }
    return 0;
}