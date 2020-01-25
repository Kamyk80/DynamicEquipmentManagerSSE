﻿#include "skse64_common/skse_version.h"  // RUNTIME_VERSION

#include <string>  // string

#include "Ammo.h"  // Ammo, TESEquipEventHandler
#include "Helmet.h"  // Helmet, BSAnimationGraphEventHandler
#include "PlayerUtil.h"  // SinkAnimationGraphEventHandler
#include "Settings.h"  // Settings
#include "Shield.h"  // Shield, BSAnimationGraphEventHandler
#include "version.h"  // VERSION_VERSTRING, VERSION_MAJOR

#include "SKSE/API.h"
#include "RE/Skyrim.h"


namespace
{
	enum
	{
		kSerializationVersion = 3,
		kDynamicEquipmentManager = 'DNEM',
		kAmmo = 'AMMO',
		kHelmet = 'HELM',
		kShield = 'SHLD'
	};


	std::string DecodeTypeCode(UInt32 a_typeCode)
	{
		constexpr std::size_t SIZE = sizeof(UInt32);

		std::string sig;
		sig.resize(SIZE);
		char* iter = reinterpret_cast<char*>(&a_typeCode);
		for (std::size_t i = 0, j = SIZE - 2; i < SIZE - 1; ++i, --j) {
			sig[j] = iter[i];
		}
		return sig;
	}


	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		auto ammo = Ammo::Ammo::GetSingleton();
		if (!ammo->Save(a_intfc, kAmmo, kSerializationVersion)) {
			_ERROR("Failed to save ammo!\n");
			ammo->Clear();
		}

		auto helmet = Helmet::Helmet::GetSingleton();
		if (!helmet->Save(a_intfc, kHelmet, kSerializationVersion)) {
			_ERROR("Failed to save helmet!\n");
			helmet->Clear();
		}

		auto shield = Shield::Shield::GetSingleton();
		if (!shield->Save(a_intfc, kShield, kSerializationVersion)) {
			_ERROR("Failed to save shield!\n");
			shield->Clear();
		}

		_MESSAGE("Finished saving data");
	}


	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		auto ammo = Ammo::Ammo::GetSingleton();
		ammo->Clear();
		auto helmet = Helmet::Helmet::GetSingleton();
		helmet->Clear();
		auto shield = Shield::Shield::GetSingleton();
		shield->Clear();

		UInt32 type;
		UInt32 version;
		UInt32 length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version != kSerializationVersion) {
				_ERROR("Loaded data is out of date! Read (%u), expected (%u) for type code (%s)", version, kSerializationVersion, DecodeTypeCode(type).c_str());
				continue;
			}

			switch (type) {
			case kAmmo:
				if (!ammo->Load(a_intfc)) {
					_ERROR("Failed to load ammo!\n");
					ammo->Clear();
				}
				break;
			case kHelmet:
				if (!helmet->Load(a_intfc)) {
					_ERROR("Failed to load helmet!\n");
					helmet->Clear();
				}
				break;
			case kShield:
				if (!shield->Load(a_intfc)) {
					_ERROR("Failed to load shield!\n");
					shield->Clear();
				}
				break;
			default:
				_ERROR("Unrecognized record type (%s)!", DecodeTypeCode(type).c_str());
				break;
			}
		}

		_MESSAGE("Finished loading data");
	}


	class TESObjectLoadedEventHandler : public RE::BSTEventSink<RE::TESObjectLoadedEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;


		static TESObjectLoadedEventHandler* GetSingleton()
		{
			static TESObjectLoadedEventHandler singleton;
			return &singleton;
		}


		virtual EventResult ProcessEvent(const RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_eventSource) override
		{
			if (!a_event) {
				return EventResult::kContinue;
			}

			auto player = RE::PlayerCharacter::GetSingleton();
			if (a_event->formID == player->formID) {
				if (Settings::manageHelmet) {
					if (SinkAnimationGraphEventHandler(Helmet::BSAnimationGraphEventHandler::GetSingleton())) {
						_MESSAGE("Registered helmet player animation event handler");
					}
				}
				if (Settings::manageShield) {
					if (SinkAnimationGraphEventHandler(Shield::BSAnimationGraphEventHandler::GetSingleton())) {
						_MESSAGE("Registered shield player animation event handler");
					}
				}
			}

			return EventResult::kContinue;
		}

	protected:
		TESObjectLoadedEventHandler() = default;
		TESObjectLoadedEventHandler(const TESObjectLoadedEventHandler&) = delete;
		TESObjectLoadedEventHandler(TESObjectLoadedEventHandler&&) = delete;
		virtual ~TESObjectLoadedEventHandler() = default;

		TESObjectLoadedEventHandler& operator=(const TESObjectLoadedEventHandler&) = delete;
		TESObjectLoadedEventHandler& operator=(TESObjectLoadedEventHandler&&) = delete;
	};


	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
				sourceHolder->AddEventSink(TESObjectLoadedEventHandler::GetSingleton());
				_MESSAGE("Registered object loaded event handler");

				if (Settings::manageAmmo) {
					sourceHolder->AddEventSink(Ammo::TESEquipEventHandler::GetSingleton());
					_MESSAGE("Registered ammo equip event handler");
				}

				if (Settings::manageHelmet) {
					sourceHolder->AddEventSink(Helmet::TESEquipEventHandler::GetSingleton());
					_MESSAGE("Registered helmet equip event handler");
				}

				if (Settings::manageShield) {
					sourceHolder->AddEventSink(Shield::TESEquipEventHandler::GetSingleton());
					_MESSAGE("Registered shield equip event handler");
				}
			}
			break;
		}
	}
}


extern "C" {
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\DynamicEquipmentManagerSSE.log");
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::UseLogStamp(true);

		_MESSAGE("DynamicEquipmentManagerSSE v%s", DNEM_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "DynamicEquipmentManagerSSE";
		a_info->version = DNEM_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!\n");
			return false;
		}

		switch (a_skse->RuntimeVersion()) {
		case RUNTIME_VERSION_1_5_97:
			break;
		default:
			_FATALERROR("Unsupported runtime version %08X!\n", a_skse->RuntimeVersion());
			return false;
		}

		return true;
	}


	bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		_MESSAGE("DynamicEquipmentManagerSSE loaded");

		if (!SKSE::Init(a_skse)) {
			return false;
		}

		if (Settings::loadSettings()) {
			_MESSAGE("Settings loaded successfully");
#if _DEBUG
			Settings::dump();
#endif
		} else {
			_FATALERROR("Failed to load settings!\n");
			return false;
		}

		auto messaging = SKSE::GetMessagingInterface();
		if (messaging->RegisterListener("SKSE", MessageHandler)) {
			_MESSAGE("Messaging interface registration successful");
		} else {
			_FATALERROR("Messaging interface registration failed!\n");
			return false;
		}

		auto serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID(kDynamicEquipmentManager);
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);

		if (Settings::manageShield) {
			Shield::InstallHooks();
			_MESSAGE("Installed hooks for shield");
		}

		return true;
	}
};
