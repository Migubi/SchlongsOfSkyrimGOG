#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64/GameMenus.h"
#include "skse64/GameRTTI.h"
#include "skse64/GameData.h"
#include "skse64/PapyrusNativeFunctions.h"
#include <shlobj.h>
#include <set>
#include <vector>

#include "json/json.h"
#include "Schlongification.h"
#include "Storage.h"
#include "Papyrus.h"
#include "Armors.h"
#include "SosIO.h"
#include "StorageJSON.h"

static PluginHandle g_pluginHandle = kPluginHandle_Invalid;
SKSESerializationInterface * g_serialization = NULL;
static SKSEMessagingInterface * g_messaging = NULL;
static SKSEPapyrusInterface * g_papyrus = NULL;

bool initialized = false;
static const int pluginVersion = 300004;

void Serialization_Load(SKSESerializationInterface * intfc){
	_MESSAGE("Serialization_Load");
	int loadedVersion = Storage::Load(intfc);

	// once we have the data from the SKSE co-save:
	_MESSAGE("\nSCHLONGIFICATION");

	// remove slot 52 from vanilla armors that have been set to revealing through MCM
	Schlongification::ModifyRevealingArmors();

	// add the slot 52 to custom armors
	Schlongification::ModifyCustomArmors();

}

void Serialization_Revert(SKSESerializationInterface * intfc){
	_MESSAGE("Serialization_Revert");
	Storage::Revert(intfc);
}

void Serialization_Save(SKSESerializationInterface * intfc){
	_MESSAGE("Serialization_Save");
	Storage::Save(intfc, pluginVersion);
}

class MyMenuEventHandler : public BSTEventSink <MenuOpenCloseEvent>
{
public:
	EventResult MyMenuEventHandler::ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
	{
		// Still using this thing instead of SKSEMessagingInterface to run changes on loaded armors
		// There is no message of type kMessage_PreLoadGame or kMessage_NewGame
		// when starting the game with "coc whiterunbanneredmare" in the menu screen
		// it seems kMessage_InputLoaded is too soon because the mods are not yet loaded
		if (!initialized && evn->opening && evn->menuName == BSFixedString("HUD Menu")){
			initialized = true;
			Schlongification::Init();
		}
		return kEvent_Continue;
	}
};
MyMenuEventHandler menuEventHandler;

extern "C"
{

	__declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface* skse)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\SchlongsOfSkyrim.log");

		int iniSetting = GetPrivateProfileInt("General", "iLogLevel", (int)gLog.kLevel_Message, ".\\Data\\SKSE\\Plugins\\SchlongsOfSkyrim.ini");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(static_cast<IDebugLog::LogLevel>(iniSetting));

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor) {
			_FATALERROR("loaded in editor, marking as incompatible");
			return false;
		}

		g_serialization = (SKSESerializationInterface*)skse->QueryInterface(kInterface_Serialization);
		if (!g_serialization) {
			_FATALERROR("couldn't get serialization interface");
			return false;
		}
		if (g_serialization->version < SKSESerializationInterface::kVersion) {
			_FATALERROR("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);
			return false;
		}

		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
		if (!g_papyrus) {
			_FATALERROR("couldn't get papyrus interface");
			return false;
		}
		if (g_papyrus->interfaceVersion < SKSEPapyrusInterface::kInterfaceVersion) {
			_FATALERROR("papyrus interface too old (%d expected %d)", g_papyrus->interfaceVersion, SKSEPapyrusInterface::kInterfaceVersion);
			return false;
		}

		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		if (!g_messaging) {
			_FATALERROR("couldn't get messaging interface");
			return false;
		}
		if (g_messaging->interfaceVersion < SKSEMessagingInterface::kInterfaceVersion) {
			_FATALERROR("messaging interface too old (%d expected %d)", g_messaging->interfaceVersion, SKSEMessagingInterface::kInterfaceVersion);
			return false;
		}

		Storage::Init();

		g_papyrus->Register(Papyrus::RegisterFuncs);
		g_papyrus->Register(Armors::RegisterFuncs);
		g_papyrus->Register(Storage::RegisterFuncs);
		g_papyrus->Register(StorageJSON::RegisterFuncs);
		g_papyrus->Register(SosIO::RegisterFuncs);
		//g_papyrus->Register(Maintenance::RegisterFuncs);

		g_serialization->SetUniqueID(g_pluginHandle, 'SOS');
		g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
		g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
		g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

		MenuManager::GetSingleton()->MenuOpenCloseEventDispatcher()->AddEventSink(&menuEventHandler);

		return true;
	}

	__declspec(dllexport) SKSEPluginVersionData SKSEPlugin_Version =
	{
		SKSEPluginVersionData::kVersion,

		300004,
		"SchlongsOfSkyrim",

		"galgaroth, dll by Bigumi",
		"",

		0,	// not version independent (extended field)
		0,	// not version independent
		{ RUNTIME_VERSION_1_6_659_GOG, 0 },	// compatible with 1.6.659

		0,	// works with any version of the script extender. you probably do not need to put anything here
	};


};