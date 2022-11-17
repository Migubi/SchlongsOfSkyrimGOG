#pragma once

bool HasKeywordString(TESForm * form, const char * strKeyword);
bool IsRevealing(TESObjectARMO * armor);
void ModifyVanillaArmors(UInt8 modIndex, std::set<UInt32> armors);

UInt8 IsModActive(char * modName);

namespace Schlongification{

	void Init();

	void ModifyCustomArmors();

	void ModifyRevealingArmors();

}