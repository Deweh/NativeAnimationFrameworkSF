#pragma once
#include "Transform.h"

namespace Animation
{
	class Node
	{
	public:
		ozz::math::Float4x4* localMatrix;

		virtual Transform GetLocal() = 0;
		virtual Transform GetWorld() = 0;
		virtual void SetLocal(const Transform& t) = 0;
		virtual void SetWorld(const Transform& t) = 0;
		virtual void SetLocalReal(const RE::NiMatrix3& rot, const RE::NiPoint3& pos) = 0;
		virtual const char* GetName() = 0;
		virtual ~Node();
	};

	class GameNode : public Node
	{
	public:
		RE::NiAVObject* n;

		GameNode(RE::NiAVObject* n);

		virtual Transform GetLocal();
		virtual Transform GetWorld();
		virtual void SetLocal(const Transform& t);
		virtual void SetWorld(const Transform& t);
		virtual void SetLocalReal(const RE::NiMatrix3& rot, const RE::NiPoint3& pos);
		virtual const char* GetName();
		virtual ~GameNode();
	};

	class NullNode : public Node
	{
	public:
		NullNode(ozz::math::Float4x4* a_localMat);

		virtual Transform GetLocal();
		virtual Transform GetWorld();
		virtual void SetLocal(const Transform& t);
		virtual void SetWorld(const Transform& t);
		virtual void SetLocalReal(const RE::NiMatrix3& rot, const RE::NiPoint3& pos);
		virtual const char* GetName();
		virtual ~NullNode();
	};
}