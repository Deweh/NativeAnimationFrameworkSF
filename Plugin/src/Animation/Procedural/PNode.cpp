#include "PNode.h"

namespace Animation::Procedural
{
	void PEvaluationContext::UpdateModelSpaceCache(const std::span<ozz::math::SoaTransform>& a_localPose, int a_from, int a_to)
	{
		ozz::animation::LocalToModelJob l2mJob;
		l2mJob.skeleton = skeleton;
		l2mJob.input = ozz::make_span(a_localPose);
		l2mJob.output = ozz::make_span(modelSpaceCache);
		l2mJob.from = a_from;
		l2mJob.to = a_to;
		l2mJob.Run();
	}

	std::unique_ptr<PNodeInstanceData> PNode::CreateInstanceData()
	{
		return nullptr;
	}

	void PNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
	}

	void PNode::Synchronize(PNodeInstanceData* a_instanceData, PNodeInstanceData* a_ownerInstance, float a_correctionDelta)
	{
	}

	bool PNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		return true;
	}

	PNode::Registration* PNode::GetTypeInfo()
	{
		return nullptr;
	}

	PNode::Registration::Registration(const char* a_typeName,
		const std::vector<InputConnection>& a_inputs,
		const std::vector<std::pair<const char*, size_t>>& a_customValues,
		size_t a_output,
		CreationFunctor a_createFunctor) :
		typeName(a_typeName),
		inputs(a_inputs),
		customValues(a_customValues),
		output(a_output),
		createFunctor(a_createFunctor)
	{
		GetRegisteredNodeTypes()[{ a_typeName }] = this;
	}

	std::unordered_map<std::string_view, PNode::Registration*>& GetRegisteredNodeTypes()
	{
		static std::unordered_map<std::string_view, PNode::Registration*> instance;
		return instance;
	}

	PNode::InputConnection::InputConnection(const char* a_name, size_t a_evalType, bool a_optional) :
		name(a_name), evalType(a_evalType), optional(a_optional)
	{
	}
}