#include <precompiled/engine_precompiled.hpp>
#include "tools/pcg_rand.hpp"

namespace pcg
{

RNGState g_rng;

void SeedGlobal(uint64_t state, uint64_t inc)
{
    g_rng.state = state;
    g_rng.inc = inc;
}

uint32_t rand(RNGState& rng)
{
	uint64_t oldState = rng.state;

	rng.state = oldState * 6364136223846793005ULL + (rng.inc|1);

	uint32_t xorshifted = uint32_t(((oldState >> 18u) ^ oldState) >> 27u);
	int32_t rot = uint32_t(oldState >> 59u);

	return (xorshifted >> rot) | (xorshifted << (uint32_t)((-rot) & 31));
}

uint32_t rand()
{
    return rand(g_rng);
}

float rand0_1(RNGState& rng)
{
    float randNum = (float)rand(rng);
    uint32_t uintMax = 0xffffffff;
    float max = (float)uintMax;

    return randNum / max;
}

float rand0_1()
{
    return rand0_1(g_rng);
}

}