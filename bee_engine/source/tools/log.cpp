#include <precompiled/engine_precompiled.hpp>
#include "tools/log.hpp"

                                                                                          

void bee::Log::Initialize()
{
#if 0  // Samples from Log for testing
    Log::Warn("Easy padding in numbers like {:08d}", 12);
    Log::Critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    Log::Info("Support for floats {:03.2f}", 1.23456);
    Log::Info("Positional args are {1} {0}..", "too", "supported");
    Log::Info("{:>8} aligned, {:<8} aligned", "right", "left");
    Log::Error("ERR!?");
#endif
    Log::Info("   _______      .---.          ,-----.        .-'''-.     .-'''-.      ,-----.     ,---.    ,---. ");
    Log::Info("  \\  ____  \\    | ,_|        .'  .-,  '.     / _     \\   / _     \\   .'  .-,  '.   |    \\  /    | ");
    Log::Info("  | |    \\ |  ,-./  )       / ,-.|  \\ _ \\   (`' )/`--'  (`' )/`--'  / ,-.|  \\ _ \\  |  ,  \\/  ,  | ");
    Log::Info("  | |____/ /  \\  '_ '`)    ;  \\  '_ /  | : (_ o _).    (_ o _).    ;  \\  '_ /  | : |  |\\_   /|  | ");
    Log::Info("  |   _ _ '.   > (_)  )    |  _`,/ \\ _/  |  (_,_). '.   (_,_). '.  |  _`,/ \\ _/  | |  _( )_/ |  | ");
    Log::Info("  |  ( ' )  \\ (  .  .-'    : (  '\\_/ \\   ; .---.  \\  : .---.  \\  : : (  '\\_/ \\   ; | (_ o _) |  | ");
    Log::Info("  | (_(;)_) |  `-'`-'|___   \\ `'/  \\  ) /  \\    `-'  | \\    `-'  |  \\ `'/  \\  ) /  |  (_,_)  |  | ");
    Log::Info("  |  (_,_)  /   |        \\   '. \\_/``'.'    \\       /   \\       /    '. \\_/``'.'   |  |      |  | ");
    Log::Info("  /_______.'    `--------`     '-----'       `-...-'     `-...-'       '-----'     '--'      '--' ");
    Log::Info("");

    Log::Info("Blossom Engine - Nature Team");
}