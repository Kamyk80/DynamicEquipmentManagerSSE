#include "Ammo.h"

#include "Forms.h"  // WeapTypeBoundArrow
#include "PlayerUtil.h"  // PlayerIsBeastRace

#include "SKSE/API.h"
#include "RE/Skyrim.h"


namespace Ammo
{
	Ammo* Ammo::GetSingleton()
	{
		static Ammo singleton;
		return &singleton;
	}


	RE::TESAmmo* Ammo::GetForm()
	{
		return static_cast<RE::TESAmmo*>(ISerializableForm::GetForm());
	}


	void DelayedWeaponTaskDelegate::Run()
	{
		if (g_equippedWeaponFormID != kInvalid) {
			auto weap = RE::TESForm::LookupByID<RE::TESObjectWEAP>(g_equippedWeaponFormID);

			if ((!weap->IsBow() && !weap->IsCrossbow()) || weap->IsBound()) {
				g_equippedWeaponFormID = kInvalid;
				return;
			}

			Visitor visitor;
			VisitPlayerInventoryChanges(&visitor);

			auto ammo = Ammo::GetSingleton()->GetForm();
			if (ammo) {
				auto equipManager = RE::ActorEquipManager::GetSingleton();
				auto player = RE::PlayerCharacter::GetSingleton();
				equipManager->EquipItem(player, ammo, 0, visitor.Count(), 0, true, false, false);

				auto ui = RE::UI::GetSingleton();
				auto intStrings = RE::InterfaceStrings::GetSingleton();
				auto invMenu = ui->GetMenu<RE::InventoryMenu>(intStrings->inventoryMenu);
				if (invMenu && invMenu->itemList) {
					invMenu->itemList->Update(player);
				}
			}

			g_equippedWeaponFormID = kInvalid;
		}
	}


	void DelayedWeaponTaskDelegate::Dispose()
	{
		delete this;
	}


	DelayedWeaponTaskDelegate::Visitor::Visitor() :
		_count(0)
	{}


	bool DelayedWeaponTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->object->IsAmmo()) {
			if (a_entry->object->formID == Ammo::GetSingleton()->GetFormID()) {
				_count = a_count;
				return false;
			}
		}
		return true;
	}


	SInt32 DelayedWeaponTaskDelegate::Visitor::Count() const
	{
		return _count;
	}


	bool DelayedAmmoTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->object->formID == g_equippedAmmoFormID && a_entry->extraLists) {
			for (auto& xList : *a_entry->extraLists) {
				if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
					auto equipManager = RE::ActorEquipManager::GetSingleton();
					auto player = RE::PlayerCharacter::GetSingleton();
					equipManager->UnequipItem(player, a_entry->object, xList, a_count, 0, true, false);

					auto ui = RE::UI::GetSingleton();
					auto intStrings = RE::InterfaceStrings::GetSingleton();
					auto invMenu = ui->GetMenu<RE::InventoryMenu>(intStrings->inventoryMenu);
					if (invMenu && invMenu->itemList) {
						invMenu->itemList->Update(player);
					}

					return false;
				}
			}
		}
		return true;
	}


	void DelayedAmmoTaskDelegate::Run()
	{
		if (g_equippedAmmoFormID != kInvalid) {
			RE::TESAmmo* ammo = RE::TESForm::LookupByID<RE::TESAmmo>(g_equippedAmmoFormID);
			if (!ammo->HasKeyword(WeapTypeBoundArrow)) {
				if (g_equippedWeaponFormID == kInvalid) {
					Ammo::GetSingleton()->SetForm(g_equippedAmmoFormID);
				} else {
					// Ammo was force equipped
					Visitor visitor;
					VisitPlayerInventoryChanges(&visitor);
				}
			}
			g_equippedAmmoFormID = kInvalid;
		}
	}


	void DelayedAmmoTaskDelegate::Dispose()
	{
		delete this;
	}


	bool TESEquipEventHandler::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->object->formID == Ammo::GetSingleton()->GetFormID() && a_entry->extraLists) {
			auto equipManager = RE::ActorEquipManager::GetSingleton();
			auto player = RE::PlayerCharacter::GetSingleton();
			auto xList = a_entry->extraLists->empty() ? 0 : a_entry->extraLists->front();
			equipManager->UnequipItem(player, a_entry->object, xList, a_count, 0, true, false);
			return false;
		}
		return true;
	}


	TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
	{
		static TESEquipEventHandler singleton;
		return &singleton;
	}


	auto TESEquipEventHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
		-> EventResult
	{
		if (!a_event || !a_event->hActor || !a_event->hActor->IsPlayerRef() || PlayerIsBeastRace()) {
			return EventResult::kContinue;
		}

		auto form = RE::TESForm::LookupByID(a_event->baseObject);
		if (!form) {
			return EventResult::kContinue;
		}

		auto task = SKSE::GetTaskInterface();
		switch (form->formType) {
		case RE::FormType::Weapon:
			if (a_event->equipped) {
				g_equippedWeaponFormID = form->formID;
				task->AddTask(new DelayedWeaponTaskDelegate());
			} else {
				Visitor visitor;
				VisitPlayerInventoryChanges(&visitor);
			}
			break;
		case RE::FormType::Ammo:
			if (a_event->equipped) {
				g_equippedAmmoFormID = form->formID;
				task->AddTask(new DelayedAmmoTaskDelegate());
			}
			break;
		}
		return EventResult::kContinue;
	}
}
