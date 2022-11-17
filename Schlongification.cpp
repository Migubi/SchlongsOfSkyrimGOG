#include "skse64/PluginAPI.h"
#include "skse64/GameData.h"
#include "skse64/GameRTTI.h"
#include "skse64/PapyrusNativeFunctions.h"
#include <set>

#include "Util.h"
#include "Data.h"
#include "Schlongification.h"

std::set<UInt32> vanillaArmors = {
	0x0006F393, 0x0010594B, 0x0010594D, 0x0010594F, 0x0004B28B, 0x0005D009, 0x000CAE15, 0x0001396B, 0x00013966, 0x0001393E, 0x00018388,
	0x001019CC, 0x0001394D, 0x00013961, 0x000896A3, 0x0001392A, 0x00105F12, 0x000B83CB, 0x00013939, 0x0002151E, 0x00021520, 0x00021508,
	0x00021507, 0x000C7F5B, 0x00021525, 0x0002150D, 0x00021522, 0x00013911, 0x000136D5, 0x00013ED9, 0x00013ED8, 0x00013948, 0x00012E49,
	0x0003619E, 0x00108544, 0x00107611, 0x000483C2, 0x0005DB86, 0x000FCC0E, 0x000FCC0F, 0x00013957, 0x000D3EA0, 0x0001B3A3, 0x0001B3A4,
	0x00013952, 0x000F6F22, 0x0001395C, 0x0008697E, 0x000A6D7B, 0x000AD5A0, 0x0001B3A2, 0x00108540, 0x00036584, 0x000D3AC3, 0x000D3ACC,
	0x000E35E8, 0x000E35D7, 0x001092B5, 0x000E35DB, 0x000C0165, 0x0005B6A1, 0x0006FF38, 0x00013105, 0x0005B69F, 0x0006FF37, 0x0001BC82,
	0x0006D92C, 0x000467BF, 0x000467C0, 0x000D3DE9, 0x0010D691, 0x0010D6A4, 0x000D3DEA, 0x0010D6A5, 0x00015516, 0x000CF8B3, 0x0001BE1A,
	0x000209A6, 0x000261C0, 0x00017695, 0x0003452E, 0x0006C1D9, 0x0006C1DA, 0x0006C1D8, 0x00086991, 0x000F8713, 0x000CEE80, 0x000F8715,
	0x000F1229, 0x0005DB7B, 0x0008698C, 0x000CEE76, 0x000EAD49, 0x000E84C4, 0x000E84C6, 0x0004223C, 0x0006FF45, 0x0007C932, 0x0010F570,
	0x00080697, 0x0006FF43, 0x000BACF3, 0x0010CFEF, 0x0010CFE4, 0x0010CFF0, 0x0010CFEC, 0x0010CFF1, 0x0010CFEA, 0x0010CFF2, 0x0010CFEB,
	0x00107106, 0x000646A7, 0x000B144D, 0x00109C11, 0x000C36EA, 0x0010710D, 0x00088952, 0x0008F19A, 0x0003C9FE, 0x00065B94, 0x000E9EB5,
	0x0007BC19, 0x00106661, 0x00107108, 0x0010CEE5, 0x00065BBF, 0x0010C698, 0x00062303, 0x000C5D11, 0x0010710C, 0x00088956, 0x000D191F,
	0x00052794, 0x0002AC61, 0x000E739B, 0x000D2844, 0x0005ABC3, 0x000E1F15, 0x00105966, 0x0010EB5B, 0x000CEE6E, 0x0006492C, 0x000E0DD0,
	0x000CF8A0, 0x000F1ABE, 0x00016FFE, 0x0010B2FD, 0x000D8D50, 0x0005CBFE, 0x000E40DF, 0x000EAFD0, 0x000C33BD, 0x00106392, 0x00106390,
	0x000C7CBB, 0x0006B46B, 0x0010710B, 0x0007E010, 0x00107109, 0x0010CEE3, 0x0010CEE4, 0x0010D662, 0x0010D664, 0x0010D2B4
};

std::set<UInt32> dawnguardArmors = {
	0x00019ADF, 0x0000F3F7, 0x0000F3FA, 0x0000F3FB, 0x0000F402, 0x0000F404, 0x000142C7, 0x000191F2, 0x000191F3, 0x0000B5DB, 0x000194C5,
	0x0001570C, 0x0000C816, 0x0000CAD3, 0x00017F76
};

std::set<UInt32> dragonbornArmors = {
	0x00037564, 0x00037563, 0x0001CD93, 0x0003AB26, 0x0001CD8A, 0x0001CD87, 0x0001CD97, 0x0001CD9F, 0x0001CDA2, 0x00037B8A, 0x0002B0F7,
	0x0001CDA6, 0x000376DA, 0x0002401B, 0x00037FEF, 0x00039151, 0x00039D1E, 0x00039D20, 0x00039D22, 0x000292AC, 0x0001A574, 0x0003C819,
	0x0001CDA9, 0x0001CDAB, 0x0002AD33
};

// adds biped slot 52 to some armors

void ModifyVanillaArmors(UInt8 modIndex, std::set<UInt32> armors){

	int modified = 0;

	// fix vanilla and dlc armors
	// adds the biped slot 52 to all vanilla and dlc body armors unless they are flagged as revealing

	for (auto armor : armors) {
		UInt32 formID = (modIndex << 24) | armor;
		TESForm * formArmor = LookupFormByID(formID);
		
		if (formArmor)
		{
			BGSBipedObjectForm* pBip = DYNAMIC_CAST(formArmor, TESForm, BGSBipedObjectForm);
			if (pBip)
			{
				bool isRevealing = HasKeywordString(formArmor, "SOS_Revealing");

				if (!isRevealing){
					// armor is not flagged as revealing, add the slot 52 to it
					// this will unequip the schlong when equip this armor
					_DMESSAGE("    adding slot 52 to armor %08X and slotmask %08X", formArmor->formID, pBip->GetSlotMask());
					pBip->AddSlotToMask(BGSBipedObjectForm::kPart_Unnamed22);

					modified++;
				}
			}
		}
		
	}

	_VMESSAGE("%i armors processed", modified);

}


UInt8 IsModActive(char * modName){
	const ModInfo * modInfo = DataHandler::GetSingleton()->LookupModByName(modName);
	if (modInfo != NULL){
		return modInfo->modIndex;
	}
	else{
		return NULL;
	}
}

namespace Schlongification{

	void Init(){

		_VMESSAGE("\nSCHLONGIFICATION");

		if (!IsModActive("Schlongs Of Skyrim.esp")){
			_WARNING("Warning: Schlongs Of Skyrim.esp is not active.");
			return;
		}

		UInt8 modIndex = 0;
		_VMESSAGE("adding slot 52 to vanilla armors");
		ModifyVanillaArmors(modIndex, vanillaArmors);

		modIndex = IsModActive("Dawnguard.esm");
		if (modIndex){
			_VMESSAGE("adding slot 52 to Dawnguard armors");
			ModifyVanillaArmors(modIndex, dawnguardArmors);
		}

		modIndex = IsModActive("Dragonborn.esm");
		if (modIndex){
			_VMESSAGE("adding slot 52 to Dragonborn armors");
			ModifyVanillaArmors(modIndex, dragonbornArmors);
		}
		
		// not needed anymore
		vanillaArmors.clear();
		dawnguardArmors.clear();
		dragonbornArmors.clear();
	}

	// fix custom armors
	// adds the biped slot 52 to all stored custom body armors
	void ModifyCustomArmors()
	{
		int count = Data::CountConcealingArmors();
		int modified = 0;

		_MESSAGE("custom armors: adding slot 52 to %i stored armors", count);

		for (int i = count; i --> 0;) // loop backwards because we could need to delete entries
		{

			TESForm * formArmor = Data::GetConcealingArmor(i);
			if (!formArmor){
				_WARNING("  no concealing armor found at position %i", i);
			}
			else{
				
				TESObjectARMO * armor = DYNAMIC_CAST(formArmor, TESForm, TESObjectARMO);
				if (!armor){
					_WARNING("  DYNAMIC_CAST TESForm --> TESObjectARMO failed for form %08X", armor->formID);
				}
				else{
					_VMESSAGE("  processing armorAddon %08X", armor->formID);
					
					// Check if vanilla or not
					std::set<UInt32>::iterator it = vanillaArmors.find(armor->formID);
					if (it == vanillaArmors.end())
					{
						// add the slot 52 to this stored custom armor addon
						// this will hide the schlong when equip this armor
						
						BGSBipedObjectForm* pBip = DYNAMIC_CAST(armor, TESObjectARMO, BGSBipedObjectForm);
						if (pBip){
							pBip->AddSlotToMask(BGSBipedObjectForm::kPart_Unnamed22);
						}
					}
					else
					{
						// The armor addon is vanilla
						// that means the armor has been already processed and either:
						// - has the slot 52
						// - it doesn't because it's revealing by keyword, in this case it shouldn't have the slot 52, or hole-in-hip issue while using revealin armor mods
						// This can happen if a vanilla armor was made revealing and then concealing (that stores it in the concealing list)
						_WARNING("  armor %08X removed from concealing list because it is vanilla", armor->formID);
						Data::RemoveConcealingArmor(armor);
					}

					modified++;
				}
			}
		}

		_MESSAGE("%i armor addons processed\n", modified);

	}

	// removes slot 52 from the vanilla armors that have been set to revealing through MCM

	void ModifyRevealingArmors(){

		BSFixedString revealingListKey = BSFixedString("SOS_RevealingArmors");

		int count = Data::CountRevealingArmors();
		int modified = 0;

		_MESSAGE("revealing armors: removing slot 52 from %i stored armor addons", count);

		// loop stored revealing armors
		for (int i = 0; i < count; i++){

			TESForm * form = Data::GetRevealingArmor(i);
			if (form){

				TESObjectARMO * armor = DYNAMIC_CAST(form, TESForm, TESObjectARMO);
				if (armor){

					// if vanilla, remove the slot 52 that was added during reschlongification
					// has no effect on custom armors
					_VMESSAGE("  processing armor %08X", armor->formID);

					BGSBipedObjectForm* pBip = DYNAMIC_CAST(armor, TESObjectARMO, BGSBipedObjectForm);
					if (pBip){

						pBip->RemoveSlotFromMask(BGSBipedObjectForm::kPart_Unnamed22);
						modified++;
					}
				}
			}
		}

		_MESSAGE("%i armor addons processed", modified);
	}

}