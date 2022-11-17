#include "skse64/PluginAPI.h"
#include "skse64/GameData.h"
#include "skse64/GameRTTI.h"
#include "skse64/GameThreads.h"
#include "skse64/NiNodes.h"
#include "skse64/GameFormComponents.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64_common/Utilities.h"
#include "skse64/papyrusNetImmerse.h"
#include <set>
#include <vector>
#include <tuple>

#include "Data.h"
#include "Papyrus.h"

std::vector<std::string> sNiNodes = {
	"NPC GenitalsBase [GenBase]",
	"NPC GenitalsScrotum [GenScrot]",
	"NPC Genitals01 [Gen01]",
	"NPC Genitals02 [Gen02]",
	"NPC Genitals03 [Gen03]",
	"NPC Genitals04 [Gen04]",
	"NPC Genitals05 [Gen05]",
	"NPC Genitals06 [Gen06]",
};

namespace Papyrus{

	// returns the probability configured in MCM for this shclong and race
	// returns 0.0 if the race is not enabled
	float GetRaceProbabilityIfEnabled(TESForm * addon, TESRace * race){
		// check MCM race configuration
		if (Data::FindEnabledRace(addon, race) != -1){

			int raceIndex = Data::FindCompatibleRace(addon, race);
			if (raceIndex != -1){
				return Data::GetRaceProbability(addon, raceIndex);
			}
		}
		return 0.0f;
	}

	bool IsSchlongGenderAllowed(TESForm * addon, UInt32 gender){
		int enabledGender = Data::GetGender(addon); // 0=Male, 1=Female, 2=Both, 10=Both but disabled for females, 11=Both but disabled for males
		return enabledGender == 2 || enabledGender == gender || enabledGender == gender + 10;
	}

	UInt32 GetSex(Actor * actor){
		UInt32 sex = 0;
		TESNPC * actorBase = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if (actorBase) {
			sex = CALL_MEMBER_FN(actorBase, GetSex)();
		}
		return sex;
	}

	// returns a list of the available schlongs for the race and actor's gender, and their race probability
	float GetCompatibleAddons(Actor *actor, TESRace * race, std::vector<std::tuple<TESForm*, float>>& probs){

		float probabilitySum = 0.0f;
		UInt32 gender = GetSex(actor);

		int count = Data::CountAddons();
		for (int i = 0; i < count; i++){
			TESForm *addon = Data::GetAddon(i);

			float addonProbability = GetRaceProbabilityIfEnabled(addon, race);
			if (addonProbability > 0.0f && IsSchlongGenderAllowed(addon, gender))
			{
				probabilitySum += addonProbability;
				probs.push_back(std::make_tuple(addon, addonProbability));
			}
		}

		return probabilitySum;
	}

	// returns a random schlong of the given list
	TESForm * GetRandomAddon(float probabilitySum, std::vector<std::tuple<TESForm*, float>> addonProbs){
		float p = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 100.0f));
		float lBound = 0.0f;
		int i = 0;
		for (auto it = addonProbs.begin(); it != addonProbs.end(); it++, i++){
			float probability = std::get<1>(*it);
			float uBound = lBound + 100 * probability / probabilitySum;
			if (p <= uBound){
				_DMESSAGE("Schlong %08X won with probability of %.2f resulting in an interval of %.2f | %.2f via random value of %.2f",
					std::get<0>(*it)->formID, probability, lBound, uBound, p);
				return std::get<0>(*it);
			}
			lBound = uBound;
		}

		return NULL;
	}

	// returns a random schlong for the actor, based on the MCM settings
	TESForm * DetermineSchlongType(StaticFunctionTag*, Actor * actor){
		if (!actor){
			return NULL;
		}
		
		TESForm * actorForm = DYNAMIC_CAST(actor, Actor, TESForm);
		if (!actorForm || Data::FindBlackListed(actorForm) != -1){
			return NULL;
		}

		TESRace * race = actor->race;
		if (Data::FindKnownRace(race) == -1){
			_MESSAGE("unknown race - actor %08X race %08X", actor->formID, race->formID);
			return NULL;
		}

		std::vector<std::tuple<TESForm*, float>> addonProbs;

		float probabilitySum = GetCompatibleAddons(actor, race, addonProbs);

		if (probabilitySum > 0.0f){
			return GetRandomAddon(probabilitySum, addonProbs);
		}
		else{
			_MESSAGE("absolutely nothing available for actor %08X race %08X", actor->formID, race->formID);
			return NULL;
		}

	}

	float Rescale(float scale, float factor){
		return 1 + (scale - 1) * factor;
	}

	void ScaleSchlongBones(StaticFunctionTag*, TESForm * addon, Actor * actor, UInt32 rank, float factor){
		if (!addon || !actor || rank < 0 || factor < -50.0f){
			return;
		}
		
		float size = rank / 20.0f;
		TESObjectREFR * obj = DYNAMIC_CAST(actor, Actor, TESObjectREFR);
		if (!obj){
			return;
		}

		//_DMESSAGE("ScaleSchlongBones: setting schlong scale for %08X to rank %i", obj->formID, rank);

		if (rank == 0){
			papyrusNetImmerse::SetNodeScale(NULL, obj, BSFixedString(sNiNodes.at(0).c_str()), 0.0001, false);
			//Notify(akActor.GetLeveledActorBase().GetName() + " was trolled!")
		}
		else{
			int iBone = 0;
			float fScale = 1.0f;
			float boost = (100 + factor) / 100;
			
			for (int i = 0; i < 8; i++){
				fScale = Data::GetBone(addon, i);
				//_DMESSAGE("ScaleSchlongBones: setting schlong scale for %08X, bone %i = %.2f", obj->formID, i, fScale);
				if (i == 0 && fScale > 1.0f){
					// apply boost on base bone
					fScale = Rescale(fScale, boost);
				}

				fScale = Rescale(fScale, size);
				//_DMESSAGE("ScaleSchlongBones: setting schlong scale for %08X to scale %.2f", obj->formID, fScale);
				papyrusNetImmerse::SetNodeScale(NULL, obj, BSFixedString(sNiNodes.at(i).c_str()), fScale, false);
			}

		}
	}

	/*
	bool CheckSkeleton(StaticFunctionTag*, Actor * actor){

		if (!actor){
			return false;
		}

		NiAVObject * skeleton = actor->GetNiNode();
		if (!skeleton){
			return false;
		}

		BSFixedString pelvis = BSFixedString("NPC Pelvis [Pelv]");
		skeleton = skeleton->GetObjectByName(&pelvis.data);
		if (!skeleton){
			return false;
		}

		BSFixedString genBase = BSFixedString("NPC GenitalsBase [GenBase]");
		skeleton = skeleton->GetObjectByName(&genBase.data);
		if (!skeleton){
			return false;
		}

		return true;

	}
	*/

	bool RegisterFuncs(VMClassRegistry * registry){

		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, TESForm*, Actor*>("DetermineSchlongType", "SOS_SKSE", DetermineSchlongType, registry));

		registry->RegisterFunction(
			new NativeFunction4 <StaticFunctionTag, void, TESForm*, Actor*, UInt32, float>("ScaleSchlongBones", "SOS_SKSE", ScaleSchlongBones, registry));
		registry->SetFunctionFlags("SOS_SKSE", "ScaleSchlongBones", VMClassRegistry::kFunctionFlag_NoWait);

		/*
		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, bool, Actor*>("CheckSkeleton", "SOS_SKSE", CheckSkeleton, registry));
		*/
		
		return true;
	};

}
