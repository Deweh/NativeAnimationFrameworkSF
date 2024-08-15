#pragma once
#include "Transform.h"
#include "Util/General.h"

namespace Animation
{
	class Graph;

	// Class to synchronize generator times & root transforms between any number of graphs.
	// When a SyncInstance is attached to a graph, that graph will rely on the owner graph's time & root transform information.
	class SyncInstance
	{
	public:

		// Due to how the game's threading works, it's possible for a reliant graph to update before the owner graph has updated for a given frame.
		// This would cause the reliant graph to lag behind the owner graph.
		//
		// To mitigate this, the sync instance keeps track of which graphs have reported an update. If a graph which is already in the updatedGraphs
		// vector reports an update, the vector is cleared, as this indicates that a new frame has begun. With this information, we can tell each graph
		// whether the owner graph has updated this frame yet or not. If the owner has not updated, the graph adds its frame delta to the stored time.

		struct InstData
		{
			Graph* owner = nullptr;
			std::vector<Graph*> updatedGraphs;
			bool ownerUpdatedThisFrame = false;
		};

		Util::Guarded<InstData> data;
		
		void NotifyGraphUpdateFinished(Graph* a_grph);
		void VisitOwner(std::function<void(Graph*, bool)> a_visitFunc);
		Graph* GetOwner();
		void SetOwner(Graph* a_grph);
	};
}