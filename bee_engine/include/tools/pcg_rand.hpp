#pragma once

#include <cstdint>

namespace pcg
{

struct RNGState
{
	uint64_t state;
	uint64_t inc;
};

void SeedGlobal(uint64_t state, uint64_t inc);

uint32_t rand(RNGState& rng);
uint32_t rand();

float rand0_1(RNGState& rng);
float rand0_1();

extern RNGState g_rng;

}