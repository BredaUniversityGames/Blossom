#include <precompiled/editor_precompiled.hpp>

#include <imgui/imgui_internal.h>

#include <array>
#include "tinygltf/stb_image_write.h"
#include "core/engine.hpp"
#include "resources/resource_manager.hpp"
#include "resources/image/image_gl.hpp"
#include "tools/tools.hpp"
#include <noise_editor/noise_texture.hpp>

using namespace bee;

NoiseTexture::NoiseTexture()
{
    mBuildData.iteration = 0;
    mBuildData.frequency = 0.02f;
    mBuildData.seed = 1337;
    mBuildData.size = { -1, -1 };
    mBuildData.offset = {};
    mBuildData.generationType = GenType_2D;

    mExportBuildData.size = { 224, 224 };

    for (size_t i = 0; i < 2; i++)
    {
        mThreads.emplace_back(GenerateLoopThread, std::ref(mGenerateQueue), std::ref(mCompleteQueue));
    }

    SetupSettingsHandlers();
}

NoiseTexture::~NoiseTexture()
{
    for (auto& thread : mThreads)
    {
        mGenerateQueue.KillThreads();
        thread.join();
    }

    if (mExportThread.joinable())
    {
        mExportThread.join();
    }
}

void NoiseTexture::Draw()
{
    TextureData texData;
    if (mCompleteQueue.Pop(texData))
    {
        if (mCurrentIteration < texData.iteration)
        {
            mCurrentIteration = texData.iteration;
            mNoiseTexture =
                Engine.Resources().Images().FromRawData(texData.textureData,
                    ImageFormat::RGBA8, texData.size.x, texData.size.y);
        }
        texData.Free();
    }

    ImGui::SetNextWindowSize(ImVec2(768, 768), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(1143, 305), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Noise Preview", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::PushItemWidth(82.0f);
        bool edited = false;

        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        ImGui::SameLine();

        // random button
        if (mBuildData.generator && (edited |= ImGui::Button(u8"\U0000f11b Randomize")))
        {
            mBuildData.seed = static_cast<uint32_t>(GetRandomNumber(0.0f, 100000.0f, 0));
        }
        ImGui::SameLine();

        edited |= ImGui::DragInt("Seed", &mBuildData.seed);
        ImGui::SameLine();

        if (ImGui::DragInt2("Size", &mExportBuildData.size.x, 2, 4, 1024))
        {
            if (ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->SettingsDirtyTimer <= 0.0f)
            {
                ImGui::GetCurrentContext()->SettingsDirtyTimer = ImGui::GetIO().IniSavingRate;
            }

            float minSize = static_cast<float>(glm::min(mExportBuildData.size.x, mExportBuildData.size.y));
            previewSize = { ((float)mExportBuildData.size.x / minSize) * (float)cPreviewSize, ((float)mExportBuildData.size.y / minSize) * (float)cPreviewSize };
        }
        ImGui::SameLine();

        edited |= ImGui::DragFloat("Frequency", &mBuildData.frequency, 0.001f);

        if (mBuildData.generator)
        {
            ImGui::SameLine();
            if (ImGui::Button("Export"))
            {
                mExportBuildData = mBuildData;
                ImGui::OpenPopup("Export noisemap");
            }
        }

        ImGui::PopItemWidth();

        if (contentSize.x >= 1 && contentSize.y >= 1 &&
            (edited || mBuildData.size.x != mExportBuildData.size.x || mBuildData.size.y != mExportBuildData.size.y))
        {
            glm::ivec2 newSize = mExportBuildData.size;

            mBuildData.offset.x -= (newSize.x - mBuildData.size.x) / 2;
            mBuildData.offset.y -= (newSize.y - mBuildData.size.y) / 2;
            mBuildData.size = newSize;
            ReGenerate(mBuildData.generator);
        }

        if (edited)
        {
            if (ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->SettingsDirtyTimer <= 0.0f)
            {
                ImGui::GetCurrentContext()->SettingsDirtyTimer = ImGui::GetIO().IniSavingRate;
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, 0);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
        if (mNoiseTexture.Retrieve() != nullptr)
        {
            const auto lm = static_cast<unsigned long long>(mNoiseTexture.Retrieve()->handle);
            const ImTextureID id = reinterpret_cast<void*>(lm);
            ImGui::ImageButton(id, ImVec2(previewSize.x, previewSize.y));
        }
        ImGui::PopStyleColor(3);

        DoExport();
    }
    ImGui::End();
}

float NoiseTexture::GetFrequency()
{
    return mBuildData.frequency;
}

int NoiseTexture::GetSeed()
{
    return mBuildData.seed;
}

void NoiseTexture::SetFrequency(float freq)
{
    mBuildData.frequency = freq;
}

void NoiseTexture::SetSeed(int seed)
{
    mBuildData.seed = seed;
}

void NoiseTexture::DoExport()
{
    if (ImGui::BeginPopupModal("Export noisemap", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::PushItemWidth(82.0f);

        // text box to enter file name
        static std::string map_name = "type here";
        ImGui::InputText("Name", &map_name);

        // drop-down to select image format
        const char* items[] = { ".bmp", ".png" };
        static int item_current_idx = 0;
        const char* combo_preview_value = items[item_current_idx];
        if (ImGui::BeginCombo("Format", combo_preview_value))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(items[n], is_selected))
                {
                    item_current_idx = n;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // export file
        if (ImGui::Button(u8"\U0000f019 Export"))
        {
            ImGui::CloseCurrentPopup();

            // async exporting
            if (mExportThread.joinable())
            {
                mExportThread.join();
            }
            mExportThread = std::thread(
                [buildData = mExportBuildData, items]()
                {
                    auto data = BuildTexture(buildData);
                    int width = buildData.size.x;
                    int height = buildData.size.y;
                    std::string path = "/textures/noise/";
                    std::string name = map_name;

                    if (items[item_current_idx] == ".bmp")
                    {
                        name += ".bmp";
                        // adjust name if it already exists
                        for (int i = 1; i < 1024; i++)
                        {
                            if (!Engine.FileIO().Exists(FileIO::Directory::Asset, path + name))
                            {
                                break;
                            }
                            name = map_name;
                            name += '_' + std::to_string(i) + ".bmp";
                        }
                        path += name;

                        stbi_write_bmp(Engine.FileIO().GetPath(FileIO::Directory::Asset, path).c_str(), width, height, 4, data.textureData);
                    }
                    else if (items[item_current_idx] == ".png")
                    {
                        name += ".png";
                        // adjust name if it already exists
                        for (int i = 1; i < 1024; i++)
                        {
                            if (!Engine.FileIO().Exists(FileIO::Directory::Asset, path + name))
                            {
                                break;
                            }
                            name = map_name;
                            name += '_' + std::to_string(i) + ".png";
                        }
                        path += name;

                        stbi_write_png(Engine.FileIO().GetPath(FileIO::Directory::Asset, path).c_str(), width, height, 4, data.textureData, 4 * width);
                    }
                }
            );
        }

        // close button
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }
}

void NoiseTexture::ReGenerate(FastNoise::SmartNodeArg<> generator)
{
    mBuildData.generator = generator;
    mBuildData.iteration++;

    // clear the queue
    mGenerateQueue.Clear();

    // not visible
    if (mBuildData.size.x <= 0 || mBuildData.size.y <= 0)
    {
        return;
    }

    // add existing data to queue
    if (generator)
    {
        mGenerateQueue.Push(mBuildData);
        return;
    }

    // set blank texture as preview when no generator
    std::array<float, 16 * 16> blankTex = { };
    mNoiseTexture = Engine.Resources().Images().FromRawData(blankTex.data(), ImageFormat::R32F, 16, 16);
    blank++;
    mCurrentIteration = mBuildData.iteration;
}

void NoiseTexture::UpdateSelected(const ResourceHandle<Image> img, const FastNoise::SmartNodeArg<> generator)
{
    mBuildData.generator = generator;
    mBuildData.iteration++;
    mNoiseTexture = img;
}

NoiseTexture::TextureData NoiseTexture::BuildTexture(const BuildData& buildData)
{
    static thread_local std::vector<float> noiseData;
    noiseData.resize((size_t)buildData.size.x * buildData.size.y);

    auto gen = FastNoise::New<FastNoise::ConvertRGBA8>(buildData.generator->GetSIMDLevel());
    gen->SetSource(buildData.generator);

    FastNoise::OutputMinMax minMax;

    switch (buildData.generationType)
    {
    case GenType_2D:
        minMax = gen->GenUniformGrid2D(noiseData.data(),
            (int)buildData.offset.x, (int)buildData.offset.y,
            buildData.size.x, buildData.size.y,
            buildData.frequency, buildData.seed);
        break;

    case GenType_2DTiled:
        minMax = gen->GenTileable2D(noiseData.data(),
            buildData.size.x, buildData.size.y,
            buildData.frequency, buildData.seed);
        break;

    case GenType_Count:
        break;
    }

    return TextureData(buildData.iteration, buildData.size, minMax, noiseData);
}

void NoiseTexture::GenerateLoopThread(GenerateQueue<BuildData>& generateQueue, CompleteQueue<TextureData>& completeQueue)
{
    while (true)
    {
        BuildData buildData = generateQueue.Pop();

        if (generateQueue.ShouldKillThread())
        {
            return;
        }

        TextureData texData = BuildTexture(buildData);

        if (!completeQueue.Push(texData))
        {
            texData.Free();
        }
    }
}

void NoiseTexture::SetupSettingsHandlers()
{
    ImGuiSettingsHandler editorSettings;
    editorSettings.TypeName = "NoiseToolNoiseTexture";
    editorSettings.TypeHash = ImHashStr(editorSettings.TypeName);
    editorSettings.UserData = this;
    editorSettings.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* outBuf)
        {
            auto* noiseTexture = (NoiseTexture*)handler->UserData;
            outBuf->appendf("\n[%s][Settings]\n", handler->TypeName);

            outBuf->appendf("frequency=%f\n", noiseTexture->mBuildData.frequency);
            outBuf->appendf("seed=%d\n", noiseTexture->mBuildData.seed);
            outBuf->appendf("gen_type=%d\n", (int)noiseTexture->mBuildData.generationType);
            outBuf->appendf("export_size=%d:%d\n", noiseTexture->mExportBuildData.size.x,
                noiseTexture->mExportBuildData.size.y);
        };
    editorSettings.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void*
        {
            if (strcmp(name, "Settings") == 0)
            {
                return handler->UserData;
            }

            return nullptr;
        };
    editorSettings.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
        {
            auto* noiseTexture = (NoiseTexture*)handler->UserData;

            /*sscanf(line, "frequency=%f", &noiseTexture->mBuildData.frequency);
            sscanf(line, "seed=%d", &noiseTexture->mBuildData.seed);
            sscanf(line, "gen_type=%d", (int*)&noiseTexture->mBuildData.generationType);
            sscanf(line, "export_size=%d:%d", &noiseTexture->mExportBuildData.size.x, &noiseTexture->mExportBuildData.size.y);*/
        };

    // ImGuiExtra::AddOrReplaceSettingsHandler(editorSettings);
}

ResourceHandle<Image> NoiseTexture::GetNoiseTextureHandle()
{
    return mNoiseTexture;
}