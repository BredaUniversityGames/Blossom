#pragma once
#include <memory>

namespace bee
{

class Shader;


class PostProcess
{
public:
	PostProcess();

	enum class Type
	{
		Vignette,
		Bloom,
		DepthOfField,
		None // always keep this one as the last entry
	};

	struct RenderTextureCollection
	{
		void* sourceTexture = nullptr;
		void* brightnessTexture = nullptr;

		void* worldSpacePositionTexture = nullptr;
		void* normalTexture = nullptr;
	};

protected:
	int m_width = -1;
	int m_height = -1;
	Type m_type = Type::None;

	virtual void Draw(void* sourceTexture, void* finalFramebuffer, void* brightnessTexture = nullptr) { }
	virtual void Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer) { }

	friend class PostProcessManager;
};

class Vignette : public PostProcess
{
private:
	struct VignetteData
	{
		float m_vignette;
	};

	VignetteData m_vignetteData;
	std::shared_ptr<Shader> m_vignetteShader = nullptr;

	void Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer);
	// TODO: Remove legacy
	void Draw(void* sourceTexture, void* finalFramebuffer, void* brightnessTexture = nullptr);
public:
	Vignette(float vignette = 0.0f);
	~Vignette();

	VignetteData& GetVignetteData() { return m_vignetteData; }
};

class Bloom : public PostProcess
{
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

	struct BloomData
	{
		float m_bloomWeight;
		unsigned int m_blurAmount;
	};

	BloomData m_bloomData;
	std::shared_ptr<Shader> m_screenQuad = nullptr;
	std::shared_ptr<Shader> m_threshold = nullptr; 
	std::shared_ptr<Shader> m_gaussianBlur = nullptr;
	std::shared_ptr<Shader> m_bloomFinal = nullptr;

	void Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer);
public:
	Bloom(float weight = 0.3f, unsigned int blur = 4);
	~Bloom();

	BloomData& GetBloomData() { return m_bloomData; }
};

class DepthOfField : public PostProcess
{
public:
	struct DepthOfFieldData
	{
		float FocusDistance;
		float FocusFalloffDistance;

		float BlurStrength;
	};

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

	DepthOfFieldData m_dofData = {};
	std::shared_ptr<Shader> m_screenQuad = nullptr;

	void Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer);

public:

	DepthOfField();
	~DepthOfField();

	DepthOfFieldData& GetDOFData() { return m_dofData; }
};

}