#include "skse64/GameRTTI.h"
#include "skse64/GameData.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64/PluginAPI.h"

#include "Util.h"

bool HasKeywordString(TESForm * form, const char * strKeyword){
	if (!form){
		return false;
	}

	BGSKeywordForm* pKeywords = DYNAMIC_CAST(form, TESForm, BGSKeywordForm);
	if (!pKeywords){
		return false;
	}

	for (int i = pKeywords->numKeywords; i--> 0;){
		if (!strcmp(pKeywords->keywords[i]->keyword.Get(), strKeyword)){
			return true;
		}
	}

	return false;
}

