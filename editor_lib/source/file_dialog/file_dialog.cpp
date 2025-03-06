#include <precompiled/editor_precompiled.hpp>
#include <file_dialog/windows_file_dialog.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>

using Microsoft::WRL::ComPtr;

std::optional<std::filesystem::path> bee::WindowsFileDialog::Input()
{
    //With help from ChatGPT
    ComPtr<IFileOpenDialog> input;

    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        NULL,
        CLSCTX_ALL,
        IID_PPV_ARGS(&input)
    );

    std::filesystem::path result{};

    if (SUCCEEDED(hr))
    {

        ComPtr<IShellItem> pDefaultFolder;

        auto projectRoot = std::filesystem::current_path();
        projectRoot /= m_defaultDirectory;

        SHCreateItemFromParsingName(projectRoot.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolder));
        input->SetFolder(pDefaultFolder.Get());

        std::vector<COMDLG_FILTERSPEC> filter_ptrs;
        for (auto&& [name, filter] : m_filters) filter_ptrs.push_back({ name.c_str(), filter.c_str()});

        input->SetFileTypes(static_cast<uint32_t>(filter_ptrs.size()), filter_ptrs.data());

        // Show the file picker dialog
        hr = input->Show(NULL);

        if (SUCCEEDED(hr))
        {
            // Process the selected file(s)
            IShellItem* pSelectedItem {};
            hr = input->GetResult(&pSelectedItem);

            if (SUCCEEDED(hr))
            {
                PWSTR path_ptr{};
                pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &path_ptr);
                if (path_ptr) result = path_ptr;
            }
            pSelectedItem->Release();
        }
    }
    
    if (result.empty()) return std::nullopt;
    return result;
}

std::optional<std::filesystem::path> bee::WindowsFileDialog::Output(const std::wstring& filename)
{
    //With help from ChatGPT
    ComPtr<IFileSaveDialog> input;

    HRESULT hr = CoCreateInstance(
        CLSID_FileSaveDialog,
        NULL,
        CLSCTX_ALL,
        IID_PPV_ARGS(&input)
    );

    std::filesystem::path result{};

    if (SUCCEEDED(hr))
    {

        ComPtr<IShellItem> pDefaultFolder;

        auto projectRoot = std::filesystem::current_path();
        projectRoot /= m_defaultDirectory;

        SHCreateItemFromParsingName(projectRoot.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolder));
        input->SetFolder(pDefaultFolder.Get());

        std::vector<COMDLG_FILTERSPEC> filter_ptrs;
        for (auto&& [name, filter] : m_filters) filter_ptrs.push_back({ name.c_str(), filter.c_str() });

        input->SetFileTypes(static_cast<uint32_t>(filter_ptrs.size()), filter_ptrs.data());
        input->SetFileName(filename.c_str());

        // Show the file picker dialog
        hr = input->Show(NULL);

        if (SUCCEEDED(hr))
        {
            // Process the selected file(s)
            IShellItem* pSelectedItem{};
            hr = input->GetResult(&pSelectedItem);

            if (SUCCEEDED(hr))
            {
                PWSTR path_ptr{};
                pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &path_ptr);
                if (path_ptr) result = path_ptr;
            }
            pSelectedItem->Release();
        }
    }

    if (result.empty()) return std::nullopt;
    return result;
}
