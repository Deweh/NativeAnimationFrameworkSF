#include "SyncInstance.h"
#include "Graph.h"

namespace Animation
{
	void SyncInstance::NotifyGraphUpdateFinished(Graph* a_grph)
	{
		auto d = data.lock();
		for (auto& g : d->updatedGraphs) {
			if (g == a_grph) {
				d->ownerUpdatedThisFrame = false;
				d->updatedGraphs.clear();
				break;
			}
		}
		d->updatedGraphs.push_back(a_grph);
		if (a_grph == d->owner) {
			d->ownerUpdatedThisFrame = true;
		}
	}

	void SyncInstance::VisitOwner(std::function<void(Graph*, bool)> a_visitFunc)
	{
		auto d = data.lock();
		auto& owner = d->owner;
		if (owner == nullptr) {
			return;
		}

		std::unique_lock l{ owner->lock };
		a_visitFunc(owner, d->ownerUpdatedThisFrame);
	}

	Graph* SyncInstance::GetOwner()
	{
		return data.lock()->owner;
	}

	void SyncInstance::SetOwner(Graph* a_grph)
	{
		auto d = data.lock();
		d->owner = a_grph;
	}
}