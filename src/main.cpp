namespace MCM
{
	class Settings
	{
	public:
		class General
		{
		public:
			inline static REX::INI::Bool bAutoSaveMode{ "General", "bAutoSaveMode", false };
			inline static REX::INI::I32  iAutoSaveCount{ "General", "iAutoSaveCount", 3 };
		};

		static void Update()
		{
			Register();

			const auto ini = REX::INI::SettingStore::GetSingleton();
			ini->Init(
				"Data/MCM/Config/BakaQuickFullSaves/settings.ini",
				"Data/MCM/Settings/BakaQuickFullSaves.ini");
			ini->Load();

			UpdateGameSetting();
		}

	private:
		class EventHandler :
			public REX::Singleton<EventHandler>,
			public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_sink) override
			{
				if (a_event.menuName == "PauseMenu" && !a_event.opening)
				{
					MCM::Settings::Update();
				}

				return RE::BSEventNotifyControl::kContinue;
			}
		};

		static void Register()
		{
			if (bRegistered)
			{
				return;
			}

			if (auto UI = RE::UI::GetSingleton())
			{
				UI->RegisterSink<RE::MenuOpenCloseEvent>(EventHandler::GetSingleton());
				bRegistered = true;
			}
		}

		static void UpdateGameSetting()
		{
			if (auto INISettingCollection = RE::INISettingCollection::GetSingleton())
			{
				if (auto iAutoSaveCount = INISettingCollection->GetSetting("iAutoSaveCount:SaveGame"))
				{
					iAutoSaveCount->SetInt(MCM::Settings::General::iAutoSaveCount);
				}
			}
		}

		inline static bool bRegistered{ false };
	};
}

namespace Hooks
{
	class hkQuickLoad
	{
	private:
		static void QueueSaveLoadTask(RE::BGSSaveLoadManager* a_this, RE::BGSSaveLoadManager::QUEUED_TASK a_task)
		{
			if (a_task == RE::BGSSaveLoadManager::QUEUED_TASK::kQuickLoad)
			{
				return _QueueSaveLoadTask(a_this, RE::BGSSaveLoadManager::QUEUED_TASK::kLoadMostRecentSave);
			}

			return _QueueSaveLoadTask(a_this, a_task);
		}

		inline static REL::Hook _QueueSaveLoadTask{ REL::ID(2249427), 0xC3, QueueSaveLoadTask };
	};

	class hkQuickSave
	{
	private:
		static void GenerateSaveFileName(RE::BGSSaveLoadManager* a_this, char* a_saveFileName, bool a_displayOnly, RE::BGSSaveLoadManager::SAVEFILE_CATEGORY a_saveCategory)
		{
			if (MCM::Settings::General::bAutoSaveMode)
			{
				return _GenerateSaveFileName(a_this, a_saveFileName, a_displayOnly, RE::BGSSaveLoadManager::SAVEFILE_CATEGORY::kAuto);
			}

			return _GenerateSaveFileName(a_this, a_saveFileName, a_displayOnly, RE::BGSSaveLoadManager::SAVEFILE_CATEGORY::kUser);
		}

		inline static REL::Hook _GenerateSaveFileName{ REL::ID(2228083), 0x1ED, GenerateSaveFileName };
	};
}

namespace
{
	void MessageCallback(F4SE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case F4SE::MessagingInterface::kGameDataReady:
			if (static_cast<bool>(a_msg->data))
				MCM::Settings::Update();
			break;
		default:
			break;
		}
	}
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se, { .trampoline = true, .trampolineSize = 32 });
	F4SE::GetMessagingInterface()->RegisterListener(MessageCallback);
	return true;
}
