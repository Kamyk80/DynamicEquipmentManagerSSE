#include "Helmet.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "Settings.h"  // Settings
#include "version.h"  // MAKE_STR

#include "RE/BaseExtraList.h"  // BaseExtraList
#include "RE/BShkbAnimationGraph.h"  // BShkbAnimationGraph
#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/EnchantmentItem.h"  // EnchantmentItem
#include "RE/EquipManager.h"  // EquipManager
#include "RE/ExtraDataTypes.h"  // ExtraDataType
#include "RE/ExtraEnchantment.h"  // ExtraEnchantment
#include "RE/FormTypes.h"  // FormType
#include "RE/InventoryEntryData.h"  // InventoryEntryData
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/TESObjectARMO.h"  // TESObjectARMO


namespace Helmet
{
	Helmet::Helmet() :
		ISerializableForm()
	{}


	Helmet::~Helmet()
	{}


	const char* Helmet::ClassName() const
	{
		return MAKE_STR(Helmet);
	}


	void Helmet::Clear()
	{
		ISerializableForm::Clear();
		_enchantment.Clear();
	}


	bool Helmet::Save(json& a_save)
	{
		try {
			json helmetSave;
			if (!ISerializableForm::Save(helmetSave)) {
				return false;
			}

			json enchSave;
			if (!_enchantment.Save(enchSave)) {
				return false;
			}

			a_save = {
				{ ClassName(), helmetSave },
				{ _enchantment.ClassName(), enchSave }
			};
		} catch (std::exception& e) {
			_ERROR("[ERROR] %s\n", e.what());
			return false;
		}

		return true;
	}


	bool Helmet::Load(json& a_load)
	{
		try {
			auto& it = a_load.find(ClassName());
			if (it == a_load.end() || !ISerializableForm::Load(*it)) {
				return false;
			}

			it = a_load.find(_enchantment.ClassName());
			if (it == a_load.end() || !_enchantment.Load(*it)) {
				return false;
			}
		} catch (std::exception& e) {
			_ERROR("[ERROR] %s\n", e.what());
			return false;
		}

		return true;
	}


	void Helmet::SetEnchantmentForm(UInt32 a_formID)
	{
		_enchantment.SetForm(a_formID);
	}


	UInt32 Helmet::GetLoadedEnchantmentFormID()
	{
		return _enchantment.GetLoadedFormID();
	}


	RE::TESObjectARMO* Helmet::GetArmorForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::TESObjectARMO>(_loadedFormID);
		}
	}


	RE::EnchantmentItem* Helmet::GetEnchantmentForm()
	{
		return _enchantment.GetEnchantmentForm();
	}


	Helmet::Enchantment::Enchantment() :
		ISerializableForm()
	{}


	Helmet::Enchantment::~Enchantment()
	{}


	const char*	Helmet::Enchantment::ClassName() const
	{
		return MAKE_STR(Enchantment);
	}


	RE::EnchantmentItem* Helmet::Enchantment::GetEnchantmentForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::EnchantmentItem>(_loadedFormID);
		}
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
		if (this) {
			delete this;
		}
	}


	bool HelmetTaskDelegate::HelmetEquipVisitor::Accept(RE::InventoryEntryData* a_entry)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type && a_entry->type->formID == g_lastEquippedHelmet.GetLoadedFormID()) {
			RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
			RE::EnchantmentItem* enchantment = g_lastEquippedHelmet.GetEnchantmentForm();
			if (enchantment) {
				if (armor->objectEffect) {
					if (armor->objectEffect->formID != enchantment->formID) {
						return true;
					}
				} else {
					if (!a_entry->extraList) {
						return true;
					}
					bool found = false;
					for (auto& xList : *a_entry->extraList) {
						if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
							RE::ExtraEnchantment* ench = xList->GetByType<RE::ExtraEnchantment>();
							if (ench && ench->objectEffect && ench->objectEffect->formID == enchantment->formID) {
								found = true;
								break;
							}
						}
					}
					if (!found) {
						return true;
					}
				}
			}
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::BaseExtraList* xList = (a_entry->extraList && !a_entry->extraList->empty()) ? a_entry->extraList->front() : 0;
			equipManager->EquipItem(player, armor, xList, 1, armor->equipSlot, true, false, false, 0);
			return false;
		}
		return true;
	}


	bool HelmetTaskDelegate::HelmetUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type && a_entry->type->Is(RE::FormType::Armor) && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
					if (armor->HasPartOf(FirstPersonFlag::kHair) && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
						RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
						RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
						equipManager->UnEquipItem(player, armor, xList, 1, armor->equipSlot, true, false, true, false, 0);
						return false;
					}
				}
			}
		}
		return true;
	}


	void UnequipHelmet()
	{}


	void DelayedHelmetLocator::Run()
	{
		Visitor visitor(_formID);
		VisitPlayerInventoryChanges(&visitor);
	}


	void DelayedHelmetLocator::Dispose()
	{
		if (this) {
			delete this;
		}
	}


	bool DelayedHelmetLocator::Visitor::Accept(RE::InventoryEntryData* a_entry)
	{
		if (a_entry->type && a_entry->type->formID == _formID && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					g_lastEquippedHelmet.SetForm(_formID);
					RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
					if (armor->objectEffect) {
						g_lastEquippedHelmet.SetEnchantmentForm(armor->objectEffect->formID);
					} else {
						for (auto& xList : *a_entry->extraList) {
							if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
								RE::ExtraEnchantment* ench = xList->GetByType<RE::ExtraEnchantment>();
								if (ench && ench->objectEffect) {
									g_lastEquippedHelmet.SetEnchantmentForm(ench->objectEffect->formID);
								}
							}
						}
					}
					return false;
				}
			}
		}
		return true;
	}


	RE::EventResult TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
	{
		using RE::EventResult;
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!a_event || !a_event->akSource || a_event->akSource->formID != player->formID) {
			return EventResult::kContinue;
		}

		RE::TESForm* form = RE::TESForm::LookupByID(a_event->formID);
		if (!form) {
			return EventResult::kContinue;
		}

		switch (form->formType) {
		case RE::FormType::Armor:
		{
			if (a_event->isEquipping) {
				RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(form);
				if (armor->HasPartOf(FirstPersonFlag::kHair)) {
					if (armor->IsLightArmor() || armor->IsHeavyArmor()) {
						DelayedHelmetLocator* dlgt = new DelayedHelmetLocator(form->formID);
						g_task->AddTask(dlgt);
					} else {
						g_lastEquippedHelmet.Clear();
					}
				}
			}
			break;
		}
		}
		return EventResult::kContinue;
	}


	RE::EventResult BSAnimationGraphEventHandler::ReceiveEvent(RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		using RE::EventResult;

		constexpr char* BeginWeaponDraw = "BeginWeaponDraw";
		constexpr char* BeginWeaponSheathe = "BeginWeaponSheathe";

		if (!a_event || !a_event->akTarget) {
			return EventResult::kContinue;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->akTarget->formID != player->formID) {
			return EventResult::kContinue;
		}

		if (a_event->animName == BeginWeaponDraw) {
			TaskDelegate* dlgt = new HelmetTaskDelegate(true);
			g_task->AddTask(dlgt);
		} else if (a_event->animName == BeginWeaponSheathe) {
			TaskDelegate* dlgt = new HelmetTaskDelegate(false);
			g_task->AddTask(dlgt);
		}

		return EventResult::kContinue;
	}


	class IAnimationGraphManagerHolderEx : public RE::IAnimationGraphManagerHolder
	{
	public:
		typedef bool _ConstructBShkbAnimationGraph_t(RE::IAnimationGraphManagerHolder* a_this, RE::BShkbAnimationGraph*& a_out);
		static _ConstructBShkbAnimationGraph_t* orig_ConstructBShkbAnimationGraph;


		bool Hook_ConstructBShkbAnimationGraph(RE::BShkbAnimationGraph*& a_out)
		{
			bool result = orig_ConstructBShkbAnimationGraph(this, a_out);
			if (!_sinked) {
				a_out->GetBSAnimationGraphEventSource()->AddEventSink(&g_animationGraphEventSink);
				_sinked = true;
			}
			return result;
		}


		static void InstallHooks()
		{
			if (Settings::manageHelmet) {
				constexpr uintptr_t PLAYER_CHARACTER_I_ANIMATION_GRAPH_MANAGER_HOLDER_VTBL = 0x0167DFF0;
				RelocPtr<_ConstructBShkbAnimationGraph_t*> vtbl_ConstructBShkbAnimationGraph(PLAYER_CHARACTER_I_ANIMATION_GRAPH_MANAGER_HOLDER_VTBL + (0x5 * 0x8));
				orig_ConstructBShkbAnimationGraph = *vtbl_ConstructBShkbAnimationGraph;
				SafeWrite64(vtbl_ConstructBShkbAnimationGraph.GetUIntPtr(), GetFnAddr(&Hook_ConstructBShkbAnimationGraph));
				_DMESSAGE("Installed hook for (%s)", typeid(IAnimationGraphManagerHolderEx).name());
			}
		}

	private:
		static bool _sinked;
	};


	bool IAnimationGraphManagerHolderEx::_sinked = false;
	IAnimationGraphManagerHolderEx::_ConstructBShkbAnimationGraph_t* IAnimationGraphManagerHolderEx::orig_ConstructBShkbAnimationGraph;


	void InstallHooks()
	{
		IAnimationGraphManagerHolderEx::InstallHooks();
	}


	Helmet g_lastEquippedHelmet;
	TESEquipEventHandler g_equipEventSink;
}
