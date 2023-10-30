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

	GameNode::~GameNode() {}

	Transform NullNode::GetLocal() { return Transform(); }
	Transform NullNode::GetWorld() { return Transform(); }
	void NullNode::SetLocal(const Transform&) {}
	void NullNode::SetWorld(const Transform&) {}
	NullNode::~NullNode() {}
}