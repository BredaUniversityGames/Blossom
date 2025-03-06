#include <precompiled/game_precompiled.hpp>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "game/blossom.hpp"

using namespace bee;

//Exports for selecting dedicated GPU
#ifdef BEE_PLATFORM_PC
#include <windows/dgpu_exports.hpp>
#endif

int main(int argc, char* argv[])
{
    Mode mode =
#if defined(BEE_EDITOR)
        Mode::WINDOW;
#else
        Mode::FULLSCREEN;
#endif

    bee::Engine.Initialize(mode);

    {
        BlossomGame game = BlossomGame();

        bee::Engine.Run([&](float dt) {
            game.Update(dt);
        });
    }

    bee::Engine.Shutdown();
}
