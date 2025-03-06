#pragma once
#define WIN32_LEAN_AND_MEAN
#include <wtypes.h>
#ifdef __cplusplus
extern "C"
{
#endif

    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif