#include "SyncInstance.h"

namespace Animation
{
	SyncInstance::UpdateData SyncInstance::NotifyGraphUpdate(Graph* a_grph)
	{
		bool ownerUpdated = false;
		auto d = data.lock();
		for (auto& g : d->updatedGraphs) {
			if (g == a_grph) {
				d->updatedGraphs.clear();
				ownerUpdated = false;
				break;
			} else if (g == d->owner) {
				ownerUpdated = true;
			}
		}
		d->updatedGraphs.push_back(a_grph);

		return SyncInstance::UpdateData {
			.rootTransform = d->rootTransform,
			.time = d->time,
			.hasOwnerUpdated = ownerUpdated,
			.sequencePhase = d->sequencePhase,
			.syncEnabled = d->syncEnabled
		};
	}

	void SyncInstance::NotifyOwnerUpdate(float a_time, const Transform& a_rootTransform, bool a_syncEnabled, size_t a_sequencePhase)
	{
		auto d = data.lock();
		for (auto& g : d->updatedGraphs) {
			if (g == d->owner) {
				d->updatedGraphs.clear();
				break;
			}
		}
		d->updatedGraphs.push_back(d->owner);
		d->time = a_time;
		d->rootTransform = a_rootTransform;
		d->sequencePhase = a_sequencePhase;
		d->syncEnabled = a_syncEnabled;
	}

	Graph* SyncInstance::GetOwner()
	{
		return data.lock()->owner;
	}

	void SyncInstance::SetOwner(Graph* a_grph)
	{
		auto d = data.lock();
		d->sequencePhase = UINT64_MAX;
		d->syncEnabled = true;
		d->owner = a_grph;
	}
}