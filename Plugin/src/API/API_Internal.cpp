#include "API_Internal.h"
#include "API_External.h"
#include "Animation/GraphManager.h"
#include "Settings/Settings.h"
#include "Serialization/GLTFImport.h"
namespace
{
	class NAFAPI_UserGenerator : public Animation::Generator
	{
	public:
		void* userData;
		NAFAPI_CustomGeneratorFunction generateFunc = nullptr;
		NAFAPI_CustomGeneratorFunction onDestroyFunc = nullptr;

		virtual void Generate(float deltaTime)
		{
			generateFunc(userData, deltaTime, &output[0], paused, localTime, duration);
		}

		virtual ~NAFAPI_UserGenerator()
		{
			if (onDestroyFunc != nullptr)
				onDestroyFunc(userData, 0.0f, &output[0], paused, localTime, duration);
		}
	};

	struct NAFAPI_StringArray
	{
		std::vector<std::string> strings;
		std::vector<const char*> charPtrs;

		NAFAPI_StringArray() {}

		NAFAPI_StringArray(const std::vector<std::string>& strs)
		{
			*this = strs;
		}

		void operator=(const std::vector<std::string>& rhs)
		{
			strings = rhs;
			charPtrs.clear();
			charPtrs.reserve(strings.size());
			for (auto& s : strings) {
				charPtrs.push_back(s.c_str());
			}
		}

		operator NAFAPI_Array<const char*>()
		{
			NAFAPI_Array<const char*> result;
			result.data = &charPtrs[0];
			result.size = charPtrs.size();
			return result;
		}
	};

	class NAFAPI_SharedObject
	{
	public:
		virtual ~NAFAPI_SharedObject() {}
	};

	template <typename T, class... Args>
	class NAFAPI_Shared : public NAFAPI_SharedObject
	{
	public:
		T data;

		NAFAPI_Shared(Args... _args) :
			data(_args...) {}

		void operator=(const T& rhs)
		{
			data = rhs;
		}

		virtual ~NAFAPI_Shared() {}
	};

	std::mutex apiLock;
	uint64_t nextApiHandle = 1;
	std::unordered_map<uint64_t, std::unique_ptr<NAFAPI_SharedObject>> apiManagedObjects;

	uint64_t MakeObjectManaged(std::unique_ptr<NAFAPI_SharedObject> obj) {
		std::unique_lock l{ apiLock };
		if (nextApiHandle == UINT64_MAX)
			nextApiHandle = 1;

		uint64_t hndl = nextApiHandle++;
		apiManagedObjects[hndl] = std::move(obj);

		return hndl;
	}
}

void NAFAPI_ReleaseHandle(
	uint64_t hndl)
{
	if (hndl != 0) {
		std::unique_lock l{ apiLock };
		apiManagedObjects.erase(hndl);
	}
}

uint16_t NAFAPI_PlayAnimationFromGLTF(
	RE::Actor* a_actor,
	float a_transitionTime,
	const char* a_fileName,
	const NAFAPI_AnimationIdentifer& a_id)
{
	Serialization::GLTFImport::AnimationInfo info{ .targetActor = a_actor, .fileName = a_fileName };
	info.id.type = static_cast<Serialization::GLTFImport::AnimationIdentifer::Type>(a_id.type);
	info.id.index = a_id.index;
	if (a_id.name != nullptr) {
		info.id.name = a_id.name;
	}

	Serialization::GLTFImport::LoadAnimation(info);

	if (info.result.error) {
		return info.result.error;
	}

	Animation::GraphManager::GetSingleton()->AttachGenerator(a_actor, std::move(info.result.generator), a_transitionTime);
	return 0;
}

NAFAPI_Handle<NAFAPI_Array<const char*>> NAFAPI_GetSkeletonNodes(
	const char* a_raceEditorId)
{
	NAFAPI_Handle<NAFAPI_Array<const char*>> result;
	auto skeleton = Settings::GetSkeleton(a_raceEditorId);
	if (Settings::IsDefaultSkeleton(skeleton))
		return result;

	auto obj = std::make_unique<NAFAPI_Shared<NAFAPI_StringArray>>();
	obj->data = skeleton->nodeNames;
	result.data = obj->data;
	result.handle = MakeObjectManaged(std::move(obj));
	return result;
}

void NAFAPI_AttachClipGenerator(
	RE::Actor* a_actor,
	NAFAPI_TimelineData* a_timelines,
	uint64_t a_timelinesSize,
	float a_transitionTime,
	int a_generatorType)
{
	if (!a_actor || a_timelinesSize < 1)
		return;

	std::unique_ptr<Animation::LinearClipGenerator> gen = std::make_unique<Animation::LinearClipGenerator>();
	gen->duration = 0.001f;
	gen->SetSize(a_timelinesSize);

	for (size_t i = 0; i < a_timelinesSize; i++) {
		auto& tl = a_timelines[i];
		auto& p = gen->position[i];
		auto& r = gen->rotation[i];

		for (size_t j = 0; j < tl.positionsSize; j++) {
			auto& t = tl.positionTimes[j];
			if (t > gen->duration)
				gen->duration = t;
			p.keys.emplace(t, tl.positions[j]);
		}

		for (size_t j = 0; j < tl.rotationsSize; j++) {
			auto& t = tl.rotationTimes[j];
			if (t > gen->duration)
				gen->duration = t;
			r.keys.emplace(t, tl.rotations[j]);
		}
	}

	gen->InitTimelines();
	Animation::GraphManager::GetSingleton()->AttachGenerator(a_actor, std::move(gen), a_transitionTime);
}

void NAFAPI_AttachCustomGenerator(
	RE::Actor* a_actor,
	uint64_t a_outputSize,
	NAFAPI_CustomGeneratorFunction a_generatorFunc,
	NAFAPI_CustomGeneratorFunction a_onDestroyFunc,
	void* a_userData,
	float a_transitionTime)
{
	if (!a_actor || a_outputSize < 1)
		return;

	std::unique_ptr<NAFAPI_UserGenerator> gen = std::make_unique<NAFAPI_UserGenerator>();
	gen->generateFunc = a_generatorFunc;
	gen->onDestroyFunc = a_onDestroyFunc;
	gen->userData = a_userData;
	gen->output.resize(a_outputSize);

	Animation::GraphManager::GetSingleton()->AttachGenerator(a_actor, std::move(gen), a_transitionTime);
}

bool NAFAPI_DetachGenerator(
	RE::Actor* a_actor,
	float a_transitionTime)
{
	return Animation::GraphManager::GetSingleton()->DetachGenerator(a_actor, a_transitionTime);
}