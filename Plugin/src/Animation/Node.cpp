#include "Node.h"

namespace Animation
{
	Node::~Node() {}

	GameNode::GameNode(RE::NiAVObject* n) :
		n(n) {}

	Transform GameNode::GetLocal()
	{
		return n->local;
	}

	Transform GameNode::GetWorld()
	{
		return n->world;
	}

	void GameNode::SetLocal(const Transform& t)
	{
		t.ToReal(n->local);
	}

	void GameNode::SetWorld(const Transform& t)
	{
		t.ToReal(n->world);
	}

	void GameNode::SetLocalReal(const RE::NiMatrix3& rot, const RE::NiPoint3& pos)
	{
		n->local.rotate = rot;
		n->local.translate = pos;
	}

	const char* GameNode::GetName()
	{
		return n->name.c_str();
	}

	GameNode::~GameNode() {}

	Transform NullNode::GetLocal() { return Transform(); }
	Transform NullNode::GetWorld() { return Transform(); }
	void NullNode::SetLocal(const Transform&) {}
	void NullNode::SetWorld(const Transform&) {}
	void NullNode::SetLocalReal(const RE::NiMatrix3&, const RE::NiPoint3&) {}
	const char* NullNode::GetName() { return ""; }
	NullNode::~NullNode() {}
}