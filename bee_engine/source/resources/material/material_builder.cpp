#include <precompiled/engine_precompiled.hpp>
#include <resources/material/material_builder.hpp>
#include <resources/material/material.hpp>

#include <core/engine.hpp>
#include <resources/resource_manager.hpp>
#include <resources/image/image_loader.hpp>

bee::ResourceHandle<bee::Material> bee::MaterialBuilder::Build() const {

	auto new_material = std::make_shared<Material>();

	if (images[TextureSlotIndex::BASE_COLOR].Valid()) {
		new_material->BaseColorTexture = images[TextureSlotIndex::BASE_COLOR];
		new_material->UseBaseTexture = true;
	}

	if (images[TextureSlotIndex::EMISSIVE].Valid()) {
		new_material->EmissiveTexture = images[TextureSlotIndex::EMISSIVE];
		new_material->UseEmissiveTexture = true;
	}

	if (images[TextureSlotIndex::OCCLUSION].Valid()) {
		new_material->OcclusionTexture = images[TextureSlotIndex::OCCLUSION];
		new_material->UseOcclusionTexture = true;
	}

	if (images[TextureSlotIndex::METALLIC_ROUGHNESS].Valid()) {
		new_material->MetallicRoughnessTexture = images[TextureSlotIndex::METALLIC_ROUGHNESS];
		new_material->UseMetallicRoughnessTexture = true;
	}

	if (images[TextureSlotIndex::NORMAL_MAP].Valid()) {
		new_material->NormalTexture = images[TextureSlotIndex::NORMAL_MAP];
		new_material->UseNormalTexture = true;
	}

	if (images[TextureSlotIndex::SUBSURFACE_OCCLUSION].Valid()) {
		new_material->SubsurfaceOcclusionTexture = images[TextureSlotIndex::SUBSURFACE_OCCLUSION];
		new_material->UseSubsurfaceTexture = true;
	}

	new_material->BaseColorSampler = samplers[TextureSlotIndex::BASE_COLOR];
	new_material->EmissiveSampler = samplers[TextureSlotIndex::EMISSIVE];
	new_material->OcclusionSampler = samplers[TextureSlotIndex::OCCLUSION];
	new_material->MetallicSampler = samplers[TextureSlotIndex::METALLIC_ROUGHNESS];
	new_material->NormalSampler = samplers[TextureSlotIndex::NORMAL_MAP];

	new_material->BaseColorFactor = factors[TextureSlotIndex::BASE_COLOR];
	new_material->EmissiveFactor = factors[TextureSlotIndex::EMISSIVE];
	new_material->NormalTextureScale = factors[TextureSlotIndex::NORMAL_MAP].x;
	new_material->OcclusionTextureStrength = factors[TextureSlotIndex::OCCLUSION].x;

	new_material->MetallicFactor = factors[TextureSlotIndex::METALLIC_ROUGHNESS].x;
	new_material->RoughnessFactor = factors[TextureSlotIndex::METALLIC_ROUGHNESS].y;
	new_material->SubsurfaceFactor = factors[TextureSlotIndex::SUBSURFACE_OCCLUSION].x;

	new_material->DoubleSided = doubleSided;
	new_material->IsDitherable = dithering;

	auto new_entry = std::make_shared<ResourceEntry<Material>>();
	new_entry->resource = new_material;
	new_entry->origin_path = material_name;

	return ResourceHandle<Material>(new_entry);

}
