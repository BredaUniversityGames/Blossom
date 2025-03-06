#pragma once

#include <resources/resource_handle.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <game/constants.hpp>
#include <systems/playerHUDhelpers.hpp>

namespace bee
{

class Level;

#if defined(BEE_EDITOR)
class Editor;
#endif

class BlossomGame
{
public:
    BlossomGame();
    ~BlossomGame();

    void Update(float dt);
   
    //Opens level based on filename, except if filename is empty, where a new level (default) is created
    //Return true if the level was loaded successfully, false otherwise.
    bool OpenLevel(const std::string& file = {});

    std::shared_ptr<Level> GetLevel() const { return m_currentLevel; };

    enum State
    {
        MAIN_MENU,
        PLAYING
    };

    void SetState(State newState) { m_state = newState; }
    bool SetFreeCam(bool val) { m_freeCamEnabled = val; }

private:

    //Constants
    const GameConstants constants;

    //Main Menu UI
    ResourceHandle<Mesh> m_UIQuad;
    ResourceHandle<Image> m_playButton;
    ResourceHandle<Image> m_playButtonHovered;
    ResourceHandle<Image> m_resetButton;
    ResourceHandle<Image> m_resetButtonHovered;
    ResourceHandle<Image> m_exitButton;
    ResourceHandle<Image> m_exitButtonHovered;

    void ShowMainMenu();
    void CheckWinCondition();
    bool victory = false;

    // in-game HUD

    entt::entity m_currentPlayer = entt::null;

    DynamicBar playerBar{};
    DynamicBar hiveBar{};

    void CreateHUDResources();
    void UpdateHUD(float dt);

    std::shared_ptr<Level> m_currentLevel;
    State m_state{};
    bool m_freeCamEnabled = false;

    int m_hiveAudioChannelID;

#if defined (BEE_EDITOR)
    static constexpr bool IN_EDITOR = true;
    std::unique_ptr<Editor> m_editor;
#else
    static constexpr bool IN_EDITOR = false;
#endif

};

}  // namespace bee