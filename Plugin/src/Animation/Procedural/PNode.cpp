#include "PNode.h"

namespace Animation::Procedural
{
	std::unordered_map<std::string_view, PNode::Registration*>& GetRegisteredNodeTypes()
	{
		static std::unordered_map<std::string_view, PNode::Registration*> instance;
		return instance;
	}

	PNode::Registration::Registration(const char* a_typeName,
		const std::vector<std::pair<const char*, size_t>>& a_inputs,
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
}