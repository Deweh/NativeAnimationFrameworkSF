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

		std::vector<PNode*> inputs;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData();
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) = 0;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime);
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton);
		virtual Registration* GetTypeInfo();
		virtual ~PNode() = default;

		template <typename T>
		static std::unique_ptr<PNode> CreateNodeOfType()
		{
			return std::make_unique<T>();
		}
	};

	std::unordered_map<std::string_view, PNode::Registration*>& GetRegisteredNodeTypes();

	template <typename T>
	class PNodeT : public PNode
	{
	public:
		virtual Registration* GetTypeInfo() override
		{
			return &T::_reg;
		}
	};

	template <typename T>
	bool IsNodeOfType(PNode* a_node)
	{
		return a_node->GetTypeInfo() == &T::_reg;
	}
}