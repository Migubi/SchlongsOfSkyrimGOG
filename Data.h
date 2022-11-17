#pragma once

namespace Data{

	UInt32 FindBlackListed(TESForm * actor);
	UInt32 FindKnownRace(TESForm * race);
	UInt32 CountAddons();
	TESForm * GetAddon(int i);
	UInt32 FindEnabledRace(TESForm * addon, TESForm * race);
	UInt32 FindCompatibleRace(TESForm * addon, TESForm * race);
	float GetRaceProbability(TESForm * addon, int i);
	float GetBone(TESForm * addon, int i);
	UInt32 GetGender(TESForm * addon);

	UInt32 AddConcealingArmor(TESForm * formArmorAddon);
	UInt32 CountConcealingArmors();
	UInt32 FindConcealingArmor(TESForm * formArmorAddon);
	TESForm* GetConcealingArmor(int i);
	UInt32 RemoveConcealingArmor(TESForm * formArmorAddon);

	UInt32 AddRevealingArmor(TESForm * formArmorAddon);
	UInt32 CountRevealingArmors();
	UInt32 FindRevealingArmor(TESForm * formArmorAddon);
	TESForm* GetRevealingArmor(int i);
	UInt32 RemoveRevealingArmor(TESForm * formArmorAddon);
	
}