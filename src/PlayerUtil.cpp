#include "PlayerUtil.h"

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include <utility>  // pair, make_pair
#include <map>  // map
#include <vector>  // vector

#include "Forms.h"  // WerewolfBeastRace, DLC1VampireBeastRace

#include "RE/Skyrim.h"


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor)
{
	auto player = RE::PlayerCharacter::GetSingleton();
	auto changes = player->GetInventoryChanges();
	std::map<FormID, std::pair<RE::InventoryEntryData*, Count>> invMap;
	if (changes) {
		for (auto& entry : *changes->entryList) {
			if (entry && entry->object) {
				invMap.emplace(entry->object->formID, std::make_pair(entry, entry->countDelta));
			}
		}
	}

	auto container = player->GetContainer();
	std::vector<RE::InventoryEntryData*> heapList;
	if (container) {
		container->ForEachContainerObject([&](RE::ContainerObject* a_entry) -> bool
		{
			if (a_entry->obj) {
				auto& it = invMap.find(a_entry->obj->formID);
				if (it != invMap.end()) {
					if (!a_entry->obj->IsGold()) {
						it->second.second += a_entry->count;
					}
				} else {
					RE::InventoryEntryData* entryData = new RE::InventoryEntryData(a_entry->obj, a_entry->count);
					heapList.push_back(entryData);
					invMap.emplace(a_entry->obj->formID, std::make_pair(entryData, entryData->countDelta));
				}
			}
			return true;
		});
	}

	for (auto& item : invMap) {
		if (item.second.second > 0) {
			if (!a_visitor->Accept(item.second.first, item.second.second)) {
				break;
			}
		}
	}

	for (auto& entry : heapList) {
		delete entry;
	}
}


bool SinkAnimationGraphEventHandler(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink)
{
	auto player = RE::PlayerCharacter::GetSingleton();
	RE::BSAnimationGraphManagerPtr graphManager;
	player->GetAnimationGraphManager(graphManager);
	if (graphManager) {
		bool sinked = false;
		for (auto& animationGraph : graphManager->graphs) {
			if (sinked) {
				break;
			}
			RE::BSTEventSource<RE::BSAnimationGraphEvent>* eventSource = animationGraph->GetEventSource<RE::BSAnimationGraphEvent>();
			for (auto& sink : eventSource->sinks) {
				if (sink == a_sink) {
					sinked = true;
					break;
				}
			}
		}
		if (!sinked) {
			graphManager->graphs.front()->GetEventSource<RE::BSAnimationGraphEvent>()->AddEventSink(a_sink);
			return true;
		}
	}
	return false;
}


bool PlayerIsBeastRace()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	auto race = player->GetRace();
	return race == WerewolfBeastRace || race == DLC1VampireBeastRace;
}
