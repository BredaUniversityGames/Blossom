#pragma once
#include <vector>
#include <memory>

#include "post_process_effects.hpp"

namespace bee
{

class PostProcessManager
{
private:
    std::unordered_map<PostProcess::Type, std::shared_ptr<PostProcess>> m_processes;

    static const int s_maxProcesses = static_cast<int>(PostProcess::Type::None);
    PostProcess::Type m_orderOfExecution[s_maxProcesses] = {};

public:
    PostProcessManager();
    ~PostProcessManager();

    void InsertProcess(PostProcess::Type type);
    std::shared_ptr<PostProcess> GetProcess(PostProcess::Type type);
    template<typename T>
    std::shared_ptr<T> GetProcess(PostProcess::Type type) { return std::dynamic_pointer_cast<T>(GetProcess(type)); }

    void Reorder(const std::initializer_list<PostProcess::Type>& order);

    void Draw(const PostProcess::RenderTextureCollection& rtCollection, void* dstFramebuffer);
};

}  // namespace bee