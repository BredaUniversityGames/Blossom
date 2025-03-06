#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace bee {

//TODO: Add mode for Output files (only Input dialog is supported)

class WindowsFileDialog {
public:
	WindowsFileDialog() = default;

	WindowsFileDialog& WithFilter(const std::wstring& name, const std::wstring& filter) {
		m_filters.emplace_back(name, filter);
		return *this;
	}

	WindowsFileDialog& WithDefaultDir(const std::filesystem::path& path) {
		m_defaultDirectory = path;
		return *this;
	}

	//Creates an Input File Dialog
	std::optional<std::filesystem::path> Input();

	//Creates an Output File Dialog
	std::optional<std::filesystem::path> Output(const std::wstring& filename);

private:
	std::filesystem::path m_defaultDirectory;
	std::vector<std::pair<std::wstring, std::wstring>> m_filters;
};

}
