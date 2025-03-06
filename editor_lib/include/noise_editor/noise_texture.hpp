#pragma once
#include <string>
#include <thread>
#include <noise_editor/multithread_queues.h>

#include <glm/glm.hpp>

#include "resources/image/image_loader.hpp"
#include <FastNoise2/include/FastNoise/FastNoise.h>
#include <FastNoise2/include/FastNoise/Metadata.h>

namespace bee
{
    class NoiseTexture
    {
    public:
        enum GenType
        {
            GenType_2D,
            GenType_2DTiled,
            GenType_Count
        };

        inline static const char* GenTypeStrings =
            "2D\0"
            "2D Tiled\0";

        NoiseTexture();
        ~NoiseTexture();

        void Draw();
        void ReGenerate(FastNoise::SmartNodeArg<> generator);
        void UpdateSelected(const ResourceHandle<Image> img, const FastNoise::SmartNodeArg<> generator);

        float GetFrequency();
        int GetSeed();
        void SetFrequency(float freq);
        void SetSeed(int seed);

        ResourceHandle<Image> GetNoiseTextureHandle();

    private:
        struct BuildData
        {
            FastNoise::SmartNode<const FastNoise::Generator> generator;
            glm::ivec2 size;
            glm::vec4 offset;
            float frequency;
            int32_t seed;
            uint64_t iteration;
            GenType generationType;
        };

        struct TextureData
        {
            TextureData() = default;

            TextureData(uint64_t iter, glm::ivec2 s, FastNoise::OutputMinMax mm, const std::vector<float>& v)
                : minMax(mm), size(s), iteration(iter)
            {
                if (v.empty())
                {
                    return;
                }

                float* texDataPtr = new float[v.size()];

                std::memcpy(texDataPtr, v.data(), v.size() * sizeof(float));

                textureData = { texDataPtr };
            }

            void Free()
            {
                delete[] textureData;

                textureData = nullptr;
            }

            float* textureData;
            FastNoise::OutputMinMax minMax;
            glm::ivec2 size;
            uint64_t iteration;
        };

        static TextureData BuildTexture(const BuildData& buildData);
        static void GenerateLoopThread(GenerateQueue<BuildData>& generateQueue, CompleteQueue<TextureData>& completeQueue);

        void DoExport();
        void SetupSettingsHandlers();

        ResourceHandle<Image> mNoiseTexture;
        uint64_t mCurrentIteration = 0;

        uint64_t blank = 0;
        BuildData mBuildData;
        BuildData mExportBuildData;
        FastNoise::OutputMinMax mMinMax;

        std::thread mExportThread;
        std::vector<std::thread> mThreads;
        bool mKillThreads;
        GenerateQueue<BuildData> mGenerateQueue;
        CompleteQueue<TextureData> mCompleteQueue;

        glm::vec2 previewSize = { 448, 448 };

        static constexpr int cPreviewSize = 448;
    };
}  // namespace bee