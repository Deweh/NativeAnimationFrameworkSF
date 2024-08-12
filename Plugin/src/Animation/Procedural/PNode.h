#pragma once
#include "Animation/PoseCache.h"
#include "Animation/Ozz.h"
#include "Util/General.h"

namespace Animation::Procedural
{
	class PNode;
	using PEvaluationResult = std::variant<float, PoseCache::Handle, PNode*, std::string>;

	template <typename T>
	inline constexpr std::size_t PEvaluationType = variant_index<T, PEvaluationResult>::value;

	struct PNodeInstanceData
	{
		virtual ~PNodeInstanceData() = default;
	};

	class PNode
	{
	public:
		std::vector<PNode*> inputs;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData(const OzzSkeleton* a_skeleton);
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) = 0;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime);
		virtual void SetCustomValues(const std::span<PEvaluationResult>& a_values);

		template <typename T>
		static std::unique_ptr<PNode> CreateNodeOfType()
		{
			return std::make_unique<T>();
		}

		struct Registration
		{
			typedef std::unique_ptr<PNode> (*CreationFunctor)();

			Registration(const char* a_typeName,
				const std::vector<std::pair<const char*, size_t>>& a_inputs,
				const std::vector<std::pair<const char*, size_t>>& a_customValues,
				size_t a_output,
				CreationFunctor a_createFunctor);

			const char* typeName;
			std::vector<std::pair<const char*, size_t>> inputs;
			std::vector<std::pair<const char*, size_t>> customValues;
			size_t output;
			CreationFunctor createFunctor;
		};
	};

	std::unordered_map<std::string_view, PNode::Registration*>& GetRegisteredNodeTypes();
}