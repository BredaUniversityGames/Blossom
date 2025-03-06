#include "precompiled/engine_precompiled.hpp"

#include "platform/opengl/shader_gl.hpp"
#include "rendering/shader_db.hpp"

bee::ShaderDB::ShaderDB()
{
    m_shaders.emplace(Type::FORWARD, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "/shaders/uber.vert", 
                      "shaders/uber.frag"));
    m_shaders.emplace(Type::TERRAIN, 
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "shaders/tessellation/tess.vert",
                      "shaders/tessellation/tess.tesc",
                      "shaders/tessellation/tess.tese",
                      "shaders/uber.frag"));
    m_shaders.emplace(Type::TERRAIN_SHADOW, 
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "shaders/tessellation/tess.vert",
                      "shaders/tessellation/tess.tesc",
                      "shaders/tessellation/tess.tese",
                      "shaders/tessellation/tess_depth_only.frag"));
    m_shaders.emplace(Type::UI, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/quad_button.vert", 
                      "shaders/quad_button.frag"));
    m_shaders.emplace(Type::GRASS, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/grass/grass.vert", 
                      "shaders/grass/grass.frag"));
    m_shaders.emplace(Type::GRASS_COMPUTE, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/grass/grass_generation.comp"));
    m_shaders.emplace(Type::SKYBOX, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/skybox/skybox.vert", 
                      "shaders/skybox/skybox.frag"));
    m_shaders.emplace(Type::SHADOW, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/depth_only.vert", 
                      "shaders/depth_only.frag"));
    m_shaders.emplace(Type::TONEMAPPING, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "/shaders/postprocess/standard.vert", 
                      "/shaders/postprocess/tonemapping.frag"));
    m_shaders.emplace(Type::FILTER_IBL, 
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/quad.vert", 
                      "shaders/filter_ibl.frag"));
    m_shaders.emplace(Type::EQUIRECTANGULAR_TO_CUBEMAP, 
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "shaders/cubemap.vert",
                      "shaders/equirectangular_to_cubemap.frag"));

    m_shaders.emplace(Type::GAUSSIAN_9TAP_FILTER,
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "/shaders/postprocess/standard.vert",
                      "/shaders/postprocess/9tap_blur.frag"));

    m_shaders.emplace(Type::DOF_COMPOSITE,
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "/shaders/postprocess/standard.vert",
                      "/shaders/postprocess/dof_composite.frag"));

    m_shaders.emplace(Type::CLEAR_TEX,
                      std::make_shared<Shader>(FileIO::Directory::Asset, 
                      "shaders/util/clear_tex.comp"));
    m_shaders.emplace(Type::WRITE_DISPLACMENTS,
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "shaders/displacement/write_displacement.comp"));

    m_shaders.emplace(Type::POPULATE_3DTEX_PERLIN,
                      std::make_shared<Shader>(FileIO::Directory::Asset,
                      "shaders/noise/populate_3dtex_perlin.comp"));
}

bee::ShaderDB::~ShaderDB() = default;
