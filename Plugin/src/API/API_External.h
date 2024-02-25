#pragma once

//This header is intended to be copied into other plugins so that they can utilize the API.
namespace NAFAPI
{
	//API Invocation Templates
	//These should not be used directly. See the API Functions section for proper function wrappers.

	template <typename FT, typename... Args>
	void invoke(const std::string_view& funcName, Args... _args)
	{
		const auto hndl = GetModuleHandleA("NativeAnimationFrameworkSF.dll");
		if (hndl != NULL) {
			const auto addr = GetProcAddress(hndl, funcName.data());
			if (addr != NULL) {
				(reinterpret_cast<FT>(addr))(_args...);
			}
		}
	}

	template <typename FT, typename R, typename... Args>
	R invokeWithReturn(const std::string_view& funcName, R defaultReturn, Args... _args)
	{
		const auto hndl = GetModuleHandleA("NativeAnimationFrameworkSF.dll");
		if (hndl != NULL) {
			const auto addr = GetProcAddress(hndl, funcName.data());
			if (addr != NULL) {
				return (reinterpret_cast<FT>(addr))(_args...);
			}
		}
		return defaultReturn;
	}

	//API Structs

	struct AnimationIdentifer
	{
		enum Type : uint8_t
		{
			kIndex = 0,
			kName = 1
		};

		Type type = kIndex;
		const char* name = nullptr;
		uint64_t index = 0;
	};

	template <typename T>
	struct Array
	{
		T* data = nullptr;
		uint64_t size = 0;
	};

	template <typename K, typename V>
	struct Map
	{
		K* keys = nullptr;
		V* values = nullptr;
		uint64_t size = 0;
	};

	template <typename K, typename V>
	struct VectorMap
	{
		std::vector<K> keys;
		std::vector<V> values;

		Map<K, V> data()
		{
			if (keys.size() != values.size())
			{
				throw std::exception("Keys size must match values size.");
			}

			return Map<K, V>{ keys.data(), values.data(), keys.size() };
		}
	};

	struct Timeline
	{
		VectorMap<float, RE::NiPoint3> positions;
		VectorMap<float, RE::NiQuaternion> rotations;

		struct Data
		{
			Map<float, RE::NiPoint3> positions;
			Map<float, RE::NiQuaternion> rotations;
		};

		Data data()
		{
			return {
				positions.data(),
				rotations.data()
			};
		}
	};

	struct Transform
	{
		RE::NiQuaternion rotate;
		RE::NiPoint3 translate;
	};

	//Similar to a std::unique_ptr.
	//Once a Handle goes out of scope, the underlying pointer(s) will no longer be valid.
	//Ownership of a Handle can be transferred across scopes with std::move().
	template <typename T>
	struct Handle
	{
		typedef void (*ReleaseHandle_Def)(uint64_t a_handle);

		T data;
		uint64_t handle = 0;

		Handle() {}

		Handle(Handle<T>&) = delete;
		Handle(const Handle<T>&) = delete;

		Handle(Handle<T>&& rhs) {
			data = rhs.data;
			handle = rhs.handle;
			rhs.data = T();
			rhs.handle = 0;
		}

		T& operator->()
		{
			return data;
		}

		T& operator*()
		{
			return data;
		}

		~Handle() {
			invoke<ReleaseHandle_Def>("NAFAPI_ReleaseHandle", handle);
		}
	};

	struct Generator
	{
		bool rootResetRequired = false;
		bool paused = false;
		float localTime = 0.0f;
		float duration = 0.0f;
		Transform localRootTransform;
	};

	enum GeneratorType : int
	{
		kLinear = 0
	};

	enum GLTFErrorCode : uint16_t
	{
		kSuccess = 0,
		kNoSkeleton = 1,
		kFailedToLoad = 2,
		kFailedToMakeClip = 3,
		kInvalidAnimationIdentifier = 4
	};

	enum APIFunction : uint16_t
	{
		kIsInstalled,
		kPlayAnimationFromGLTF,
		kGetSkeletonNodes,
		kAttachClipGenerator,
		kAttachCustomGenerator,
		kDetachGenerator
	};

	//API Types

	/*
	* a_data - The a_userData pointer passed into the AttachCustomGenerator function.
	* a_generator - A pointer to the actual generator instance within NAF that this function is attached to.
	* a_deltaTime - The time since the last call, in seconds.
	* a_output - An array of transforms that will be applied to the actor after the call. The size of the array is equal to the a_outputSize value passed into the AttachCustomGenerator function.
	*/
	typedef void (*CustomGeneratorFunction)(void* a_data, Generator* a_generator, float a_deltaTime, Array<Transform> a_output);

	typedef uint16_t (*GetFeatureLevel_Def)();
	typedef GLTFErrorCode (*PlayAnimationFromGLTF_Def)(RE::Actor* a_actor, float a_transitionTime, const char* a_fileName, const AnimationIdentifer& a_id);
	typedef Handle<Array<const char*>> (*GetSkeletonNodes_Def)(const char* a_raceEditorId);
	typedef void (*AttachClipGenerator_Def)(RE::Actor* a_actor, Array<Timeline::Data>* a_timelines, float a_transitionTime, int a_generatorType);
	typedef void (*AttachCustomGenerator_Def)(RE::Actor* a_actor, CustomGeneratorFunction a_generatorFunc, CustomGeneratorFunction a_onDestroyFunc, void* a_userData, float a_transitionTime);
	typedef bool (*DetachGenerator_Def)(RE::Actor* a_actor, float a_transitionTime);

	//API Functions

	/*
	* Returns true if NativeAnimationFrameworkSF.dll is installed & loaded.
	*/
	bool IsInstalled()
	{
		return GetModuleHandleA("NativeAnimationFrameworkSF.dll") != NULL;
	}

	/*
	* Returns a set of all available API functions based on NAF's reported feature level.
	* This depends on the version of NAF the user has installed.
	*/
	std::set<APIFunction> GetAvailableFunctions()
	{
		uint16_t featureLevel = invokeWithReturn<GetFeatureLevel_Def>("NAFAPI_GetFeatureLevel", 0ui16);
		std::set<APIFunction> result;
		switch (featureLevel) {
		default:
		case 0:
			result.insert(APIFunction::kIsInstalled);
			result.insert(APIFunction::kPlayAnimationFromGLTF);
			result.insert(APIFunction::kGetSkeletonNodes);
			result.insert(APIFunction::kAttachClipGenerator);
			result.insert(APIFunction::kAttachCustomGenerator);
			result.insert(APIFunction::kDetachGenerator);
		}
		return result;
	}

	/*
	* Creates a ClipGenerator from a GLTF animation and attaches it to an actor.
	* 
	* a_actor - The actor to attach the ClipGenerator to.
	* a_transitionTime - The amount of time that the transition to the new generator will take, in seconds.
	* a_fileName - The file path to the .gltf/.glb file, starting from the Data/NAF folder.
	* a_id - An identifier to tell NAF which animation to play within the the .gltf/.glb file. Leave as default to use the first animation found.
	*/
	GLTFErrorCode PlayAnimationFromGLTF(RE::Actor* a_actor, float a_transitionTime, const char* a_fileName, const AnimationIdentifer& a_id = {})
	{
		return invokeWithReturn<PlayAnimationFromGLTF_Def>("NAFAPI_PlayAnimationFromGLTF", GLTFErrorCode::kFailedToLoad, a_actor, a_transitionTime, a_fileName, a_id);
	}

	/*
	* Returns a string array of the skeleton nodes for a specific race. Can be used to determine the output size for a custom generator.
	* Will return an empty array if no skeleton .json file is installed for the race.
	* 
	* a_raceEditorId - The editor ID of the race to get the skeleton for.
	*/
	Handle<Array<const char*>> GetSkeletonNodes(const char* a_raceEditorId)
	{
		return invokeWithReturn<GetSkeletonNodes_Def>("NAFAPI_GetSkeletonNodes", Handle<Array<const char*>>(), a_raceEditorId);
	}

	/*
	* Attaches an animation generator to an actor using the provided position & rotation data.
	* 
	* a_actor - The target actor.
	* a_timelines - The position & rotation data.
	* a_transitionTime - The amount of time that the transition to the new generator will take, in seconds.
	* a_generatorType - The interpolation mode for the genderator. Currently only Linear is available.
	*/
	void AttachClipGenerator(RE::Actor* a_actor, std::vector<Timeline>& a_timelines, float a_transitionTime, GeneratorType a_generatorType = GeneratorType::kLinear)
	{
		std::vector<Timeline::Data> data;
		data.reserve(a_timelines.size());
		for (auto& tl : a_timelines) {
			data.push_back(tl.data());
		}

		Array<Timeline::Data> arr{ data.data(), data.size() };
		invoke<AttachClipGenerator_Def>("NAFAPI_AttachClipGenerator", a_actor, &arr, a_transitionTime, static_cast<int>(a_generatorType));
	}

	/*
	* Attaches a custom animation generator to an actor, which will be called every frame update.
	* 
	* a_actor - The target actor.
	* a_outputSize - The size of the output Transforms array. This should be sized large enough for the skeleton of the target actor.
	* a_generatorFunc - The main function for the generator which will be called every frame update.
	* a_onDestroyFunc - (optional) The function that will be called when the generator is destroyed.
	* a_userData - A pointer that will be passed as the first param of the a_generatorFunc function with each call.
	* a_transitionTime - The amount of time that the transition to the new generator will take, in seconds.
	*/
	void AttachCustomGenerator(RE::Actor* a_actor, CustomGeneratorFunction a_generatorFunc, CustomGeneratorFunction a_onDestroyFunc, void* a_userData, float a_transitionTime)
	{
		invoke<AttachCustomGenerator_Def>("NAFAPI_AttachCustomGenerator", a_actor, a_generatorFunc, a_onDestroyFunc, a_userData, a_transitionTime);
	}

	/*
	* Detaches any generator currently attached to an actor.
	* Returns true if there was a generator attached to the provided actor.
	* 
	* a_actor - The actor to detach any generators from.
	* a_transitionTime - The amount of time that the transition back to the game's animation system will take, in seconds.
	*/
	bool DetachGenerator(RE::Actor* a_actor, float a_transitionTime)
	{
		return invokeWithReturn<DetachGenerator_Def>("NAFAPI_DetachGenerator", false, a_actor, a_transitionTime);
	}
}