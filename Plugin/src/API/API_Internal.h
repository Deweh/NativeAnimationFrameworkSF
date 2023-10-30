#pragma once
#include "Animation/Transform.h"
#include "Animation/Generator.h"

typedef void (*NAFAPI_CustomGeneratorFunction)(void* a_data, float a_deltaTime, Animation::Transform* a_output, bool& a_paused, float& a_localTime, float& a_duration);

struct NAFAPI_AnimationIdentifer
{
	enum Type : uint8_t
	{
		kIndex = 0,
		kName = 1
	};

	Type type = kIndex;
	const char* name;
	uint64_t index = 0;
};

struct NAFAPI_TimelineData
{
	float* positionTimes;
	RE::NiPoint3* positions;
	uint64_t positionsSize;
	float* rotationTimes;
	RE::NiQuaternion* rotations;
	uint64_t rotationsSize;
};

template <typename T>
struct NAFAPI_Handle
{
	T data;
	uint64_t handle = 0;
};

template <typename T>
struct NAFAPI_Array
{
	T* data = nullptr;
	uint64_t size = 0;
};

extern "C" __declspec(dllexport) void NAFAPI_ReleaseHandle(
	uint64_t a_hndl);

extern "C" __declspec(dllexport) uint16_t NAFAPI_PlayAnimationFromGLTF(
	RE::Actor* a_actor,
	float a_transitionTime,
	const char* a_fileName,
	const NAFAPI_AnimationIdentifer& a_id);

extern "C" __declspec(dllexport) NAFAPI_Handle<NAFAPI_Array<const char*>> NAFAPI_GetSkeletonNodes(
	const char* a_raceEditorId);

extern "C" __declspec(dllexport) void NAFAPI_AttachClipGenerator(
	RE::Actor* a_actor,
	NAFAPI_TimelineData* a_timelines,
	uint64_t a_timelinesSize,
	float a_transitionTime,
	int a_generatorType);

extern "C" __declspec(dllexport) void NAFAPI_AttachCustomGenerator(
	RE::Actor* a_actor,
	uint64_t a_outputSize,
	NAFAPI_CustomGeneratorFunction a_generatorFunc,
	NAFAPI_CustomGeneratorFunction a_onDestroyFunc,
	void* a_userData,
	float a_transitionTime);

extern "C" __declspec(dllexport) bool NAFAPI_DetachGenerator(
	RE::Actor* a_actor,
	float a_transitionTime);