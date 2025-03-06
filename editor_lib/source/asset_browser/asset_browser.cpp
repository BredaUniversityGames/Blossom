#include <precompiled/editor_precompiled.hpp>
#include <asset_browser/asset_browser.hpp>
#include <asset_browser/asset_types.hpp>

void bee::AssetBrowser::Show(BlossomGame& game)
{
	ImGui::Begin("Asset Browser");

	DrawNavigator();
	DrawFileView();

	ImGui::End();
}

void bee::AssetBrowser::DrawNavigator()
{
	if (ImGui::Button("Root")) {
		m_currentDirStack.clear();
	}

	for (size_t target = 0; target < m_currentDirStack.size(); target++)
	{
		ImGui::SameLine();

		if (ImGui::Button(m_currentDirStack[target].c_str())) 
		{
			while (target < m_currentDirStack.size() - 1)
				m_currentDirStack.pop_back();
		}
	}
}

void bee::AssetBrowser::DrawFileView()
{
	std::filesystem::path currentPath = m_rootDir;
	for (auto& dir : m_currentDirStack) currentPath += "\\" + dir;

	//Folder iteration
	for (auto& directory_elem : std::filesystem::directory_iterator{ currentPath }) {

		if (directory_elem.is_directory()) {

			std::string folderName = directory_elem.path().stem().string();
			std::string displayName = ICON_FA_FOLDER + std::string(" ") + folderName;

			bool selected = false;
			ImGui::Selectable(displayName.c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick);
			if (selected && ImGui::IsMouseDoubleClicked(0))
			{
				m_currentDirStack.push_back(folderName);
			}
		}
	}

	//File iteration
	for (auto& directory_elem : std::filesystem::directory_iterator{ currentPath }) {
		
		if (directory_elem.is_regular_file()) {

			auto extension = directory_elem.path().extension().string();
			auto filename = directory_elem.path().filename().string();

			auto icon = ICON_FA_FILE;

			if (auto it = EXTENSIONS_TO_ICON.find(extension); it != EXTENSIONS_TO_ICON.end()) {
				icon = it->second;
			}

			std::string display_name = icon + std::string(" ") + filename;

			ImGui::Selectable(display_name.c_str());
			
			if (auto found = EXTENSIONS_TO_TYPE.find(extension); found != EXTENSIONS_TO_TYPE.end()) {
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

					m_currentDragFile = directory_elem.path().lexically_relative(m_rootDir);

					std::string payload_key = "FILE" + std::to_string(found->second.hash_code());
					ImGui::SetDragDropPayload(payload_key.c_str(), &m_currentDragFile, sizeof(std::filesystem::path));
					ImGui::Text(display_name.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}
	}
}

