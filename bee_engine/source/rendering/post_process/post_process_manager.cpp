#include <precompiled/engine_precompiled.hpp>
#include "rendering/post_process/post_process_manager.hpp"

#include "rendering/post_process/post_process_effects.hpp"

#include "core/device.hpp"
#include "core/engine.hpp"
#include <tools/log.hpp>

using namespace bee;
using namespace std;

PostProcessManager::PostProcessManager()
{
    for (int i = 0; i < s_maxProcesses; i++)
    {
        m_orderOfExecution[i] = static_cast<PostProcess::Type>(i); // set order to default to order of PostProcess::Type enum
    }
}

PostProcessManager::~PostProcessManager()
{

}

void PostProcessManager::InsertProcess(PostProcess::Type type)
{
    if (m_processes.find(type) != m_processes.end())
        return;

    std::shared_ptr<PostProcess> process = nullptr;
    switch (type)
    {
        case PostProcess::Type::Bloom:
        {
            process = std::make_shared<Bloom>();
            break;
        }
        case PostProcess::Type::Vignette:
        {
            process = std::make_shared<Vignette>();
            break;
        }
        case PostProcess::Type::DepthOfField:
        {
            process = std::make_shared<DepthOfField>();
            break;
        }
        default:
            return;
    }

    m_processes.insert({ type, process });
}

std::shared_ptr<PostProcess> PostProcessManager::GetProcess(PostProcess::Type type)
{
    if (auto it = m_processes.find(type); it != m_processes.end())
        return it->second;

    return nullptr;
}

void PostProcessManager::Reorder(const std::initializer_list<PostProcess::Type>& order)
{
    // Zero out all post process passes
    for (int i = 0; i < s_maxProcesses; i++)
        m_orderOfExecution[i] = PostProcess::Type::None;

    int i = 0;

    for (const auto& type : order)
    {
        if (i > static_cast<int>(order.size()))
            return;

        m_orderOfExecution[i] = type;
        i++;
    }
}

void PostProcessManager::Draw(const PostProcess::RenderTextureCollection& rtCollection, void* dstFramebuffer)
{
    for (int i = 0; i < s_maxProcesses; i++)
    {
        if (m_orderOfExecution[i] != PostProcess::Type::None)
        {
            auto it = m_processes.find(m_orderOfExecution[i]);
            if (it != m_processes.end())
            {
                it->second->Draw(rtCollection, dstFramebuffer);
            }
        }
    }
}