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
		auto updated = d->ownerUpdatedThisFrame;
		auto owner = d->ownerHandle.lock();
		if (owner == nullptr) {
			return;
		}
		d.unlock();

		std::unique_lock l{ owner->lock };
		a_visitFunc(owner.get(), updated);
	}

	Graph* SyncInstance::GetOwner()
	{
		return data.lock()->owner;
	}

	void SyncInstance::SetOwner(Graph* a_grph)
	{
		auto d = data.lock();
		d->owner = a_grph;
		if (a_grph) {
			d->ownerHandle = std::static_pointer_cast<Graph>(a_grph->shared_from_this());
		} else {
			d->ownerHandle.reset();
		}
	}
}