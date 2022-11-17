#include "skse64/GameRTTI.h"
#include "skse64/GameData.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64/PluginAPI.h"
#include <sstream>
#include <algorithm>
#include <set>
#include <vector>
#include <tuple>

#include "json/json.h"
#include "Storage.h"
#include "Data.h"
#include "Util.h"

namespace Data{

	static const BSFixedString CONCEALING_LIST_KEY = BSFixedString("SOS_ConcealingArmors");
	static const BSFixedString REVEALING_LIST_KEY = BSFixedString("SOS_RevealingArmors");

	UInt32 FindBlackListed(TESForm * actor){
		return i_FormListFind(NULL, NULL, BSFixedString("SOS_Blacklist"), actor);
	}
	UInt32 FindKnownRace(TESForm * race){
		return i_FormListFind(NULL, NULL, BSFixedString("SOS_KnownCompatibleRaces"), race);
	}
	UInt32 CountAddons(){
		return i_FormListCount(NULL, NULL, BSFixedString("SOS_Addons"));
	}
	TESForm * GetAddon(int i){
		return i_FormListGet(NULL, NULL, BSFixedString("SOS_addons"), i);
	}
	UInt32 FindEnabledRace(TESForm * addon, TESForm * race){
		return i_FormListFind(NULL, addon, BSFixedString("SOS_EnabledRaces"), race);
	}
	UInt32 FindCompatibleRace(TESForm * addon, TESForm * race){
		return i_FormListFind(NULL, addon, BSFixedString("SOS_CompatibleRaces"), race);
	}
	float GetRaceProbability(TESForm * addon, int i){
		return i_FloatListGet(NULL, addon, BSFixedString("SOS_RaceProbabilities"), i);
	}
	float GetBone(TESForm * addon, int i){
		return i_FloatListGet(NULL, addon, BSFixedString("SOS_Bones"), i);
	}
	UInt32 GetGender(TESForm * addon){
		return i_GetIntValue(NULL, addon, BSFixedString("SOS_Genders"), 0);
	}

	///////////////////////////////////////////////////

	UInt32 AddConcealingArmor(TESForm * formArmorAddon){
		return i_FormListAdd(NULL, NULL, CONCEALING_LIST_KEY, formArmorAddon, false);
	}
	UInt32 CountConcealingArmors(){
		return i_FormListCount(NULL, NULL, CONCEALING_LIST_KEY);
	}
	UInt32 FindConcealingArmor(TESForm * formArmorAddon){
		return i_FormListFind(NULL, NULL, CONCEALING_LIST_KEY, formArmorAddon);
	}
	TESForm* GetConcealingArmor(int i){
		return i_FormListGet(NULL, NULL, CONCEALING_LIST_KEY, i);
	}
	UInt32 RemoveConcealingArmor(TESForm * formArmorAddon){
		return i_FormListRemove(NULL, NULL, CONCEALING_LIST_KEY, formArmorAddon, true);
	}

	///////////////////////////////////////////////////

	UInt32 AddRevealingArmor(TESForm * formArmorAddon){
		return i_FormListAdd(NULL, NULL, REVEALING_LIST_KEY, formArmorAddon, false);
	}
	UInt32 CountRevealingArmors(){
		return i_FormListCount(NULL, NULL, REVEALING_LIST_KEY);
	}
	UInt32 FindRevealingArmor(TESForm * formArmorAddon){
		return i_FormListFind(NULL, NULL, REVEALING_LIST_KEY, formArmorAddon);
	}
	TESForm* GetRevealingArmor(int i){
		return i_FormListGet(NULL, NULL, REVEALING_LIST_KEY, i);
	}
	UInt32 RemoveRevealingArmor(TESForm * formArmorAddon){
		return i_FormListRemove(NULL, NULL, REVEALING_LIST_KEY, formArmorAddon, true);
	}

	///////////////////////////////////////////////////
}