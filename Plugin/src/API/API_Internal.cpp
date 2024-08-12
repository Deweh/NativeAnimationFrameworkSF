#include "API_Internal.h"
#include "API_External.h"
#include "Animation/GraphManager.h"
#include "Settings/Settings.h"
#include "Serialization/GLTFImport.h"
#include "Animation/Ozz.h"
#include "Animation/Transform.h"
namespace
{
	class NAFAPI_UserGenerator : public Animation::Generator
	{
	public:
		void* userData;
		std::vector<Animation::Transform> userOutput;
		NAFAPI_CustomGeneratorFunction generateFunc = nullptr;
		NAFAPI_CustomGeneratorFunction onDestroyFunc = nullptr;
		bool detaching = false;

		virtual void Generate(Animation::PoseCache& cache) override
		{
			if (detaching) [[unlikely]]
				return;

			generateFunc(userData, this, 0.0f, { userOutput.data(), userOutput.size() });
			Animation::Transform::StoreSoaTransforms(output, [&](size_t i) {
				return userOutput[i];
			});
		}

		virtual void SetOutput(const ozz::span<ozz::math::SoaTransform>& span) override
		{
			Animation::Generator::SetOutput(span);
			userOutput.resize(span.size() * 4);
		}

		virtual void OnDetaching() override
		{
			detaching = true;
			if (onDestroyFunc != nullptr)
				onDestroyFunc(userData, this, 0.0f, { userOutput.data(), userOutput.size() });
		}

		virtual ~NAFAPI_UserGenerator()
		{
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

uint16_t NAFAPI_GetFeatureLevel() {
	return 2;
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

	auto sharedAnim = std::make_shared<Animation::OzzAnimation>();
	sharedAnim->data = std::move(info.result.anim);

	Animation::GraphManager::GetSingleton()->AttachGenerator(
		a_actor,
		std::make_unique<Animation::LinearClipGenerator>(sharedAnim),
		a_transitionTime);
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
	std::vector<std::string> namesArr;
	for (auto& n : skeleton->data->joint_names()) {
		namesArr.push_back(n);
	}
	obj->data = namesArr;
	result.data = obj->data;
	result.handle = MakeObjectManaged(std::move(obj));
	return result;
}

void NAFAPI_AttachClipGenerator(
	RE::Actor* a_actor,
	NAFAPI_Array<NAFAPI_TimelineData>* a_timelines,
	float a_transitionTime,
	int a_generatorType)
{
	if (!a_actor || a_timelines->size < 1)
		return;

	ozz::animation::offline::RawAnimation rawAnim;
	rawAnim.tracks.resize(a_timelines->size);
	rawAnim.duration = 0.001f;

	ozz::animation::offline::RawAnimation::TranslationKey tKey;
	ozz::animation::offline::RawAnimation::RotationKey rKey;

	for (size_t i = 0; i < a_timelines->size; i++) {
		auto& tl = a_timelines->data[i];
		auto& track = rawAnim.tracks[i];

		for (size_t j = 0; j < tl.positions.size; j++) {
			tKey.time = tl.positions.keys[j];
			if (tKey.time > rawAnim.duration)
				rawAnim.duration = tKey.time;

			auto& tVal = tl.positions.values[j];
			tKey.value.x = tVal.x;
			tKey.value.y = tVal.y;
			tKey.value.z = tVal.z;
			
			track.translations.push_back(tKey);
		}

		for (size_t j = 0; j < tl.rotations.size; j++) {
			rKey.time = tl.rotations.keys[j];
			if (rKey.time > rawAnim.duration)
				rawAnim.duration = rKey.time;

			auto& rVal = tl.rotations.values[j];
			rKey.value.x = rVal.x;
			rKey.value.y = rVal.y;
			rKey.value.z = rVal.z;
			rKey.value.w = rVal.w;
		}
	}

	ozz::animation::offline::AnimationBuilder builder;
	auto result = builder(rawAnim);

	if (!result)
		return;

	auto sharedAnim = std::make_shared<Animation::OzzAnimation>();
	sharedAnim->data = std::move(result);

	Animation::GraphManager::GetSingleton()->AttachGenerator(
		a_actor,
		std::make_unique<Animation::LinearClipGenerator>(sharedAnim),
		a_transitionTime);
}

void NAFAPI_AttachCustomGenerator(
	RE::Actor* a_actor,
	NAFAPI_CustomGeneratorFunction a_generatorFunc,
	NAFAPI_CustomGeneratorFunction a_onDestroyFunc,
	void* a_userData,
	float a_transitionTime)
{
	if (!a_actor)
		return;

	std::unique_ptr<NAFAPI_UserGenerator> gen = std::make_unique<NAFAPI_UserGenerator>();
	gen->generateFunc = a_generatorFunc;
	gen->onDestroyFunc = a_onDestroyFunc;
	gen->userData = a_userData;

	Animation::GraphManager::GetSingleton()->AttachGenerator(a_actor, std::move(gen), a_transitionTime);
}

void NAFAPI_VisitGraph(
	RE::Actor* a_actor,
	NAFAPI_VisitGraphFunction a_visitFunc,
	void* a_userData)
{
	if (!a_actor)
		return;

	NAFAPI_GraphData data;
	std::vector<Animation::Node*> nodePtrs;
	Animation::GraphManager::GetSingleton()->VisitGraph(a_actor, [&](Animation::Graph* g) {
		nodePtrs.reserve(g->nodes.size());
		for (auto& n : g->nodes) {
			nodePtrs.push_back(n.get());
		}
		data.flags = &g->flags;
		data.nodes.data = nodePtrs.data();
		data.nodes.size = nodePtrs.size();
		if (g->loadedData) {
			data.rootNode = g->loadedData->rootNode;
		}
		data.rootTransform = &g->rootTransform;
		a_visitFunc(a_userData, &data);
		return true;
	}, true);
}

bool NAFAPI_DetachGenerator(
	RE::Actor* a_actor,
	float a_transitionTime)
{
	return Animation::GraphManager::GetSingleton()->DetachGenerator(a_actor, a_transitionTime);
}