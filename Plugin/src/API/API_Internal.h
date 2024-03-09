#pragma once
#include "Animation/Transform.h"
#include "Animation/Generator.h"
#include "Animation/Graph.h"

template <typename T>
struct NAFAPI_Array
{
	T* data = nullptr;
	uint64_t size = 0;
};

template <typename K, typename V>
struct NAFAPI_Map
{
	K* keys = nullptr;
	V* values = nullptr;
	uint64_t size = 0;
};

template <typename T>
struct NAFAPI_Handle
{
	T data;
	uint64_t handle = 0;
};

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
	NAFAPI_Map<float, RE::NiPoint3> positions;
	NAFAPI_Map<float, RE::NiQuaternion> rotations;
};

struct NAFAPI_GraphData
{
	SFSE::stl::enumeration<Animation::Graph::FLAGS, uint16_t>* flags;
	NAFAPI_Array<Animation::Node*> nodes;
	RE::NiAVObject* rootNode;
	Animation::Transform* rootTransform;
};

typedef void (*NAFAPI_CustomGeneratorFunction)(void* a_data, Animation::Generator* a_generator, float a_deltaTime, NAFAPI_Array<Animation::Transform> a_output);
typedef void (*NAFAPI_VisitGraphFunction)(void*, NAFAPI_GraphData*);

extern "C" __declspec(dllexport) uint16_t NAFAPI_GetFeatureLevel();

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
	NAFAPI_Array<NAFAPI_TimelineData>* a_timelines,
	float a_transitionTime,
	int a_generatorType);

extern "C" __declspec(dllexport) void NAFAPI_AttachCustomGenerator(
	RE::Actor* a_actor,
	NAFAPI_CustomGeneratorFunction a_generatorFunc,
	NAFAPI_CustomGeneratorFunction a_onDestroyFunc,
	void* a_userData,
	float a_transitionTime);

extern "C" __declspec(dllexport) void NAFAPI_VisitGraph(
	RE::Actor* a_actor,
	NAFAPI_VisitGraphFunction a_visitFunc,
	void* a_userData);

extern "C" __declspec(dllexport) bool NAFAPI_DetachGenerator(
	RE::Actor* a_actor,
	float a_transitionTime);