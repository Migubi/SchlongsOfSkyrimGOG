#include "skse64/PluginAPI.h"
#include "skse64/GameData.h"
#include "skse64/GameRTTI.h"
#include "skse64/GameThreads.h"
#include "skse64/NiNodes.h"
#include "skse64/GameFormComponents.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64_common/Utilities.h"
#include <set>
#include <vector>

#include "Data.h"
#include "Papyrus.h"
#include "Util.h"

namespace Armors{


	bool IsRevealing(StaticFunctionTag*, TESObjectARMO * pArmor){
		if (!pArmor){
			return false;
		}

		if (HasKeywordString(pArmor, "SOS_Revealing")){
			_DMESSAGE("armor %08X is revealing because keyword", pArmor->formID);
			return true;
		}

		if (Data::FindRevealingArmor(pArmor) != -1){
			_DMESSAGE("armor %08X is revealing because list", pArmor->formID);
			return true;
		}

		_DMESSAGE("armor %08X is not revealing", pArmor->formID);

		return false;
	}

	/*
	* Checks if the armor is a custom armor If the armor is custom armor to SOS (it has no slot 52, and it has no revealing keyword)
	*   - add the slot 52
	*   - store the armor
	* returns true if any the armor has been modified
	*/
	bool FixCustomArmor(StaticFunctionTag*, TESObjectARMO * pArmor){
		bool result = false;

		if (!pArmor){
			return false;
		}

		if (IsRevealing(NULL, pArmor)){
			return false;
		}

		UInt32 bodyAndPelvis = BGSBipedObjectForm::kPart_Body + BGSBipedObjectForm::kPart_Unnamed22;

		BGSBipedObjectForm* pBip = DYNAMIC_CAST(pArmor, TESObjectARMO, BGSBipedObjectForm);
		if (pBip)
		{
			// is a body armor addon with no slot 52 ?
			if ((pBip->GetSlotMask() & bodyAndPelvis) == BGSBipedObjectForm::kPart_Body){

				_MESSAGE("FixCustomArmor: adding slot 52 to Armor %08X with slotMask %08X",
					pArmor->formID, pBip->GetSlotMask());

				// add the slot 52 to the body AA
				pBip->AddSlotToMask(BGSBipedObjectForm::kPart_Unnamed22);

				// store the armor
				Data::AddConcealingArmor(pArmor);

				result = true;
			}
		}

		return result;
	}
	
	void AddRevealingArmor(StaticFunctionTag*, TESObjectARMO * pArmor){
		Data::AddRevealingArmor(pArmor);
	}

	void RemoveConcealingArmor(StaticFunctionTag*, TESObjectARMO * pArmor){
		Data::RemoveConcealingArmor(pArmor);
	}

	bool IsInConcealingList(StaticFunctionTag*, TESObjectARMO * pArmor){
		return Data::FindConcealingArmor(pArmor) != -1;
	}

	bool SetConcealing(StaticFunctionTag*, TESObjectARMO * pArmor){
		if (!pArmor){
			_ERROR("SetConcealing ERROR: passed Armor is null");
			return false;
		}

		if (HasKeywordString(pArmor, "SOS_Revealing")){
			// revealing armors can't be made concealing
			_ERROR("SetConcealing ERROR: Armor %08X has keyword SOS_Revealing", pArmor->formID);
			return false;
		}

		BGSBipedObjectForm* pBip = DYNAMIC_CAST(pArmor, TESObjectARMO, BGSBipedObjectForm);
		if (pBip && (pBip->GetSlotMask() & BGSBipedObjectForm::kPart_Body) != BGSBipedObjectForm::kPart_Body){
			// only armors that use the body biped slot
			_ERROR("SetConcealing ERROR: Armor %08X does not use biped slot body", pArmor->formID);
			return false;
		}

		Data::RemoveRevealingArmor(pArmor);

		// this could store a vanilla armor addon as concealing
		// which is not needed because vanilla gear already gets the slot 52
		// instead of checking now if it's vanilla the next schlongification process will remove it from the list
		Data::AddConcealingArmor(pArmor);

		pBip->AddSlotToMask(BGSBipedObjectForm::kPart_Unnamed22);

		_MESSAGE("SetConcealing: Armor %08X set to concealing", pArmor->formID);

		return true;
	}

	bool SetRevealing(StaticFunctionTag*, TESObjectARMO * pArmor){
		if (!pArmor){
			_ERROR("SetConcealing ERROR: passed Armor is null");
			return false;
		}

		BGSBipedObjectForm* pBip = DYNAMIC_CAST(pArmor, TESObjectARMO, BGSBipedObjectForm);
		if (pBip && (pBip->GetSlotMask() & BGSBipedObjectForm::kPart_Body) != BGSBipedObjectForm::kPart_Body){
			// only armors that use the body biped slot
			_ERROR("SetConcealing ERROR: Armor %08X does not use biped slot body", pArmor->formID);
			return false;
		}

		Data::RemoveConcealingArmor(pArmor);

		Data::AddRevealingArmor(pArmor);

		pBip->RemoveSlotFromMask(BGSBipedObjectForm::kPart_Unnamed22);

		_MESSAGE("SetRevealing: Armor %08X set to revealing", pArmor->formID);

		return true;
	}

	bool RegisterFuncs(VMClassRegistry * registry){

		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, TESObjectARMO*>("FixCustomArmor", "SOS_SKSE", FixCustomArmor, registry));
			
		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, TESObjectARMO*>("IsRevealing", "SOS_SKSE", IsRevealing, registry));
		
		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, TESObjectARMO*>("IsInConcealingList", "SOS_SKSE", IsInConcealingList, registry));
		/*
		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, TESObjectARMO*>("SetConcealing", "SOS_SKSE", SetConcealing, registry));

		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, TESObjectARMO*>("SetRevealing", "SOS_SKSE", SetRevealing, registry));
			*/
		return true;
	};

}
