#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate

#include "ISerializableForm.h"  // ISerializableForm
#include "PlayerUtil.h"  // InventoryChangesVisitor

#include "RE/Skyrim.h"


namespace Shield
{
	class Shield : public ISerializableForm
	{
	public:
		static Shield* GetSingleton();

		RE::TESObjectARMO* GetForm();

	protected:
		Shield() = default;
		Shield(const Shield&) = delete;
		Shield(Shield&&) = delete;
		~Shield() = default;

		Shield& operator=(const Shield&) = delete;
		Shield& operator=(Shield&&) = delete;
	};


	class ShieldTaskDelegate : public TaskDelegate
	{
	public:
		class ShieldEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};


		class ShieldUnEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};


		ShieldTaskDelegate(bool a_equip);
		virtual ~ShieldTaskDelegate() = default;

		virtual void Run() override;
		virtual void Dispose() override;

	private:
		bool _equip;
	};


	class AnimGraphSinkDelegate : public TaskDelegate
	{
	public:
		virtual void Run() override;
		virtual void Dispose() override;
	};


	class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static TESEquipEventHandler* GetSingleton();
		virtual EventResult ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;

	protected:
		TESEquipEventHandler() = default;
		TESEquipEventHandler(const TESEquipEventHandler&) = delete;
		TESEquipEventHandler(TESEquipEventHandler&&) = delete;
		virtual ~TESEquipEventHandler() = default;

		TESEquipEventHandler& operator=(const TESEquipEventHandler&) = delete;
		TESEquipEventHandler& operator=(TESEquipEventHandler&&) = delete;
	};


	class BSAnimationGraphEventHandler : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static BSAnimationGraphEventHandler* GetSingleton();
		virtual EventResult ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;

	protected:
		BSAnimationGraphEventHandler() = default;
		BSAnimationGraphEventHandler(const BSAnimationGraphEventHandler&) = delete;
		BSAnimationGraphEventHandler(BSAnimationGraphEventHandler&&) = delete;
		virtual ~BSAnimationGraphEventHandler() = default;

		BSAnimationGraphEventHandler& operator=(const BSAnimationGraphEventHandler&) = delete;
		BSAnimationGraphEventHandler& operator=(BSAnimationGraphEventHandler&&) = delete;
	};


	void InstallHooks();


	namespace
	{
		bool g_skipAnim = false;
	}
}
