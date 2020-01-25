#include "Helmet.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "Animations.h"  // Anim, HashAnimation
#include "Forms.h"  // WerewolfBeastRace, DLC1VampireBeastRace

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace Helmet
{
	Helmet* Helmet::GetSingleton()
	{
		static Helmet singleton;
		return &singleton;
	}


	void Helmet::Clear()
	{
		ISerializableForm::Clear();
		_enchantment.Clear();
	}


	bool Helmet::Save(SKSE::SerializationInterface* a_intfc, UInt32 a_type, UInt32 a_version)
	{
		if (!ISerializableForm::Save(a_intfc, a_type, a_version)) {
			return false;
		} else if (!_enchantment.Save(a_intfc)) {
			return false;
		} else {
			return true;
		}
	}


	bool Helmet::Load(SKSE::SerializationInterface* a_intfc)
	{
		if (!ISerializableForm::Load(a_intfc)) {
			return false;
		} else if (!_enchantment.Load(a_intfc)) {
			return false;
		} else {
			return true;
		}
	}


	RE::TESObjectARMO* Helmet::GetForm()
	{
		return static_cast<RE::TESObjectARMO*>(ISerializableForm::GetForm());
	}


	void Helmet::SetEnchantmentForm(UInt32 a_formID)
	{
		_enchantment.SetForm(a_formID);
	}


	RE::EnchantmentItem* Helmet::GetEnchantmentForm()
	{
		return _enchantment.GetForm();
	}


	UInt32 Helmet::GetEnchantmentFormID()
	{
		return _enchantment.GetFormID();
	}


	RE::EnchantmentItem* Helmet::Enchantment::GetForm()
	{
		return static_cast<RE::EnchantmentItem*>(ISerializableForm::GetForm());
	}


	void HelmetTaskDelegate::Run()
	{
		if (_equip) {
			HelmetEquipVisitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		} else {
			HelmetUnEquipVisitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		}
	}


	void HelmetTaskDelegate::Dispose()
	{
		delete this;
	}


	DelayedHelmetLocator::Visitor::Visitor(UInt32 a_formID) :
		_formID(a_formID)
	{}


	bool HelmetTaskDelegate::HelmetEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		auto helmet = Helmet::GetSingleton();
		if (a_entry->object->formID == helmet->GetFormID()) {
			auto armor = static_cast<RE::TESObjectARMO*>(a_entry->object);
			auto enchantment = helmet->GetEnchantmentForm();
			if (enchantment) {
				if (!a_entry->extraLists) {
					return true;
				}
				bool found = false;
				for (auto& xList : *a_entry->extraLists) {
					auto xEnch = xList->GetByType<RE::ExtraEnchantment>();
					if (xEnch && xEnch->enchantment && xEnch->enchantment->formID == enchantment->formID) {
						found = true;
						break;
					}
				}
				if (!found) {
					return true;
				}
			}
			auto equipManager = RE::ActorEquipManager::GetSingleton();
			auto player = RE::PlayerCharacter::GetSingleton();
			auto xList = (a_entry->extraLists && !a_entry->extraLists->empty()) ? a_entry->extraLists->front() : 0;
			equipManager->EquipItem(player, armor, xList, 1, armor->equipSlot, true, false, false);
			return false;
		}
		return true;
	}


	bool HelmetTaskDelegate::HelmetUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		using FirstPersonFlag = RE::BIPED_MODEL::BipedObjectSlot;

		if (a_entry->object && a_entry->object->Is(RE::FormType::Armor) && a_entry->extraLists) {
			for (auto& xList : *a_entry->extraLists) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					auto armor = static_cast<RE::TESObjectARMO*>(a_entry->object);
					if (armor->HasPartOf(FirstPersonFlag::kHair) && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
						auto equipManager = RE::ActorEquipManager::GetSingleton();
						auto player = RE::PlayerCharacter::GetSingleton();
						equipManager->UnequipItem(player, armor, xList, 1, armor->equipSlot, true, false);
						return false;
					}
				}
			}
		}
		return true;
	}


	HelmetTaskDelegate::HelmetTaskDelegate(bool a_equip) :
		_equip(a_equip)
	{}


	DelayedHelmetLocator::DelayedHelmetLocator(UInt32 a_formID) :
		_formID(a_formID)
	{}


	void DelayedHelmetLocator::Run()
	{
		Visitor visitor(_formID);
		VisitPlayerInventoryChanges(&visitor);
	}


	void DelayedHelmetLocator::Dispose()
	{
		delete this;
	}


	bool DelayedHelmetLocator::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->object->formID == _formID && a_entry->extraLists) {
			for (auto& xList : *a_entry->extraLists) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					auto helmet = Helmet::GetSingleton();
					helmet->Clear();
					helmet->SetForm(_formID);
					auto armor = static_cast<RE::TESObjectARMO*>(a_entry->object);
					for (auto& xList : *a_entry->extraLists) {
						if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
							auto ench = xList->GetByType<RE::ExtraEnchantment>();
							if (ench && ench->enchantment) {
								helmet->SetEnchantmentForm(ench->enchantment->formID);
							}
						}
					}
					return false;
				}
			}
		}
		return true;
	}


	void AnimGraphSinkDelegate::Run()
	{
		SinkAnimationGraphEventHandler(BSAnimationGraphEventHandler::GetSingleton());
	}


	void AnimGraphSinkDelegate::Dispose()
	{
		delete this;
	}


	TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
	{
		static TESEquipEventHandler singleton;
		return &singleton;
	}


	auto TESEquipEventHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
		-> EventResult
	{
		using FirstPersonFlag = RE::BIPED_MODEL::BipedObjectSlot;

		if (!a_event || !a_event->hActor || !a_event->hActor->IsPlayerRef() || PlayerIsBeastRace()) {
			return EventResult::kContinue;
		}

		auto armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_event->baseObject);
		if (!armor) {
			return EventResult::kContinue;
		}

		if (armor->HasPartOf(FirstPersonFlag::kHead | FirstPersonFlag::kHair | FirstPersonFlag::kCirclet)) {
			auto helmet = Helmet::GetSingleton();
			if (armor->IsLightArmor() || armor->IsHeavyArmor()) {
				if (a_event->equipped) {
					SKSE::GetTaskInterface()->AddTask(new DelayedHelmetLocator(armor->formID));
				} else {
					auto player = RE::PlayerCharacter::GetSingleton();
					if (player->IsWeaponDrawn()) {
						helmet->Clear();
					}
				}
			} else {
				helmet->Clear();
			}
		}

		return EventResult::kContinue;
	}


	BSAnimationGraphEventHandler* BSAnimationGraphEventHandler::GetSingleton()
	{
		static BSAnimationGraphEventHandler singleton;
		return &singleton;
	}


	auto BSAnimationGraphEventHandler::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
		-> EventResult
	{
		if (!a_event || !a_event->holder || !a_event->holder->IsPlayerRef()) {
			return EventResult::kContinue;
		}

		auto task = SKSE::GetTaskInterface();
		switch (HashAnimation(a_event->tag)) {
		case Anim::kWeaponDraw:
			if (!PlayerIsBeastRace()) {
				task->AddTask(new HelmetTaskDelegate(true));
			}
			break;
		case Anim::kWeaponSheathe:
			if (!PlayerIsBeastRace()) {
				task->AddTask(new HelmetTaskDelegate(false));
			}
			break;
		case Anim::kGraphDeleting:
			task->AddTask(new AnimGraphSinkDelegate());
			break;
		}

		return EventResult::kContinue;
	}
}
