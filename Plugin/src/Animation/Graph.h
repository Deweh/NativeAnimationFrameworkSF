#pragma once
#include "Generator.h"
#include "Settings/SkeletonDescriptor.h"
#include "Easing.h"
#include "Node.h"

namespace Animation
{
	class Graph
	{
	public:
		enum STATE : uint8_t
		{
			kIdle = 0,
			kTransition = 1,
			kGenerator = 2
		};

		enum FLAGS : uint16_t
		{
			kNoFlags = 0,
			kTemporary = 1u << 0,
			kUnloaded3D = 1u << 1,
			kNoActiveIKChains = 1u << 2
		};

		enum TRANSITION_TYPE : uint8_t
		{
			kNoTransition = 0,
			kGameToGraph = 1,
			kGraphToGame = 2,
			kGraphSnapshotToGame = 3,
			kGeneratorToGenerator = 4
		};

		std::mutex lock;
		SFSE::stl::enumeration<FLAGS, uint16_t> flags = kNoFlags;
		STATE state = kIdle;
		TRANSITION_TYPE transitionType = kNoTransition;
		RE::NiPointer<RE::TESObjectREFR> target;

		std::shared_ptr<const Settings::SkeletonDescriptor> skeleton;
		std::vector<std::unique_ptr<Node>> nodes;
		RE::BGSFadeNode* rootNode = nullptr;
		Transform rootTransform;
		RE::NiQuaternion rootOrientation;

		std::unique_ptr<Generator> generator = nullptr;
		std::vector<Transform> transitionOutput;
		std::vector<Transform> transitionSnapshot;
		float transitionDuration = 0.0f;
		float transitionLocalTime = 0.0f;
		QuaternionLinearInterpolator transitionRotInterp;
		Point3LinearInterpolator transitionPosInterp;
		CubicInOutEase<float> transitionEase;

		Graph();

		void SetSkeleton(std::shared_ptr<const Settings::SkeletonDescriptor> a_descriptor);
		void GetSkeletonNodes(RE::BGSFadeNode* a_rootNode);
		Transform GetCurrentTransform(size_t nodeIdx);
		void Update(float a_deltaTime);
		void StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime);
		void UpdateTransition(float a_deltaTime, STATE a_endState, const std::function<std::pair<Transform, Transform>(size_t)>& a_transformsFunc);
		void PushOutput(const std::vector<Transform>& a_output);
		void SnapshotTransformsForTransition();
		void ResetRootTransform();
		void ResetRootOrientation();
		XYZTransform GetRootXYZ();
	};
}