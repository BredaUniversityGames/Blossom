#pragma once
#include <code_utils/bee_utils.hpp>
#include <filesystem>
#include <menu_interface.hpp>

namespace bee {

class AssetBrowser : public IMenu {
public:
	AssetBrowser() = default;
	~AssetBrowser() = default;

	virtual void Show(BlossomGame& game) override;

	NON_COPYABLE(AssetBrowser);
	NON_MOVABLE(AssetBrowser);

private:

	void DrawNavigator();
	void DrawFileView();

	std::filesystem::path m_currentDragFile;
	std::filesystem::path m_rootDir{ ".\\assets" };
	std::vector<std::string> m_currentDirStack;
};

}