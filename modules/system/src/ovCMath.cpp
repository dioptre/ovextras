
/**
 *
 * @fixme This class could benefit from a serious overhaul, e.g. using randomness from some established library or C11.
 *
 * - Here Linear Congruential Generator is re-implemented to avoid third-party dependencies messing the up the rand() state.
 *   This happened before when we used the global srand() / rand(). It made hard to make repeatable experiments on some
 *   platforms. The generated randomness from the introduced home-made class is not super but it should be sufficient for 
 *   OpenViBE's present use-cases.
 *
 * Other notes
 *
 * - The randomFloat32() and randomFloat64() functions might not be working as expected (verify)
 * - Due to generative process, values generated above l_ui32RandMax may not be dense (verify) 
 * - randomUInteger32WithCeiling() may not be dense either
 *
 */
#include "ovCMath.h"
#include <cstdlib>
#include <cstring>

using namespace System;

class RandomGenerator
{
private:
	uint32 l_ui32NextValue;

public:
	static const uint32 l_ui32RandMax = 0x7FFFFFFF; // (2^32)/2-1 == 2147483647

	RandomGenerator(uint32 seed = 1) : l_ui32NextValue( seed ) {}

	int32 rand(void)
	{
		// Pretty much C99 convention and parameters for a Linear Congruential Generator
		l_ui32NextValue = (l_ui32NextValue * 1103515245 + 12345) & l_ui32RandMax;
		return static_cast<int32>(l_ui32NextValue);
	}

	void setSeed(uint32 seed)
	{
		l_ui32NextValue = seed;
	}

	uint32 getSeed(void) const
	{
		return l_ui32NextValue;
	}
};

// Should be only accessed via Math:: calls defined below
static RandomGenerator g_oRandomGenerator;

boolean Math::initializeRandomMachine(const uint64 ui64RandomSeed)
{
	g_oRandomGenerator.setSeed(static_cast<uint32>(ui64RandomSeed));

	// For safety, we install also the C++ basic random engine (it might be useg by dependencies, old code, etc)
	srand(static_cast<unsigned int>(ui64RandomSeed));

	return true;
}

uint8 Math::randomUInteger8(void)
{
	return static_cast<uint8>(randomUInteger64());
}

uint16 Math::randomUInteger16(void)
{
	return static_cast<uint16>(randomUInteger64());
}

uint32 Math::randomUInteger32(void)
{
	return static_cast<uint32>(randomUInteger64());
}

uint64 Math::randomUInteger64(void)
{
	const uint64 r1=g_oRandomGenerator.rand();
	const uint64 r2=g_oRandomGenerator.rand();
	const uint64 r3=g_oRandomGenerator.rand();
	const uint64 r4=g_oRandomGenerator.rand();
	return (r1<<24)^(r2<<16)^(r3<<8)^(r4);
}

uint32 Math::randomUInteger32WithCeiling(uint32 ui32upperLimit)
{
	// float in range [0,1]
	const float64 l_f64Temp = g_oRandomGenerator.rand() / static_cast<float64>(g_oRandomGenerator.l_ui32RandMax);

	// static_cast is effectively floor(), so below we get output range [0,upperLimit-1], without explicit subtraction of 1
	const uint32 l_ui32ReturnValue = static_cast<uint32>( ui32upperLimit * l_f64Temp );

	return l_ui32ReturnValue;
}

int8 Math::randomSInterger8(void)
{
	return static_cast<int8>(randomUInteger64());
}

int16 Math::randomSInterger16(void)
{
	return static_cast<int16>(randomUInteger64());
}

int32 Math::randomSInterger32(void)
{
	return static_cast<int32>(randomUInteger64());
}

int64 Math::randomSInterger64(void)
{
	return static_cast<int64>(randomUInteger64());
}

float32 Math::randomFloat32(void)
{
	const uint32 r=randomUInteger32();
	float32 fr;
	::memcpy(&fr, &r, sizeof(fr));
	return fr;
}

float32 Math::randomFloat32BetweenZeroAndOne(void) {
	const float32 fr = static_cast<float32>(g_oRandomGenerator.rand()) / static_cast<float32>(g_oRandomGenerator.l_ui32RandMax);
	return fr;
}

float64 Math::randomFloat64(void)
{
	const uint64 r=randomUInteger64();
	float64 fr;
	::memcpy(&fr, &r, sizeof(fr));
	return fr;
}


