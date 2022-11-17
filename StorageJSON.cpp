#include "skse64/PluginAPI.h"
#include "skse64/GameData.h"
#include "skse64/GameRTTI.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "common/ICriticalSection.h"
#include <sstream>
#include <algorithm>
#include <set>
#include <vector>
#include <tuple>
#include <fstream>
#include "json/json.h"

#include "Storage.h"

// papyrus functions to store temporary json data

void SetInt(StaticFunctionTag * base, BSFixedString key, UInt32 value){
	i_SetIntValue(base, NULL, _jsonbs(key), value);
}
UInt32 GetInt(StaticFunctionTag * base, BSFixedString key){
	UInt32 value = i_GetIntValue(base, NULL, _jsonbs(key), 0);
	i_UnsetIntValue(base, NULL, _jsonbs(key));
	return value;
}
void SetFloat(StaticFunctionTag * base, BSFixedString key, float value){
	i_SetFloatValue(base, NULL, _jsonbs(key), value);
}
float GetFloat(StaticFunctionTag * base, BSFixedString key){
	float value = i_GetFloatValue(base, NULL, _jsonbs(key), 0.0f);
	i_UnsetFloatValue(base, NULL, _jsonbs(key));
	return value;
}
void SetIntList(StaticFunctionTag * base, BSFixedString key, VMArray<UInt32> value){
	i_IntListClear(base, NULL, _jsonbs(key));
	for (int i = 0; i < value.Length(); i++){
		UInt32 elem;
		value.Get(&elem, i);
		i_IntListAdd(base, NULL, _jsonbs(key), elem, true);
	}
}
VMResultArray<SInt32> GetIntList(StaticFunctionTag * base, BSFixedString key){
	int count = i_IntListCount(base, NULL, _jsonbs(key));
	VMResultArray<SInt32> list;
	list.resize(count, 0);
	for (int i = 0; i < count; i++){
		SInt32 value = (SInt32)i_IntListGet(base, NULL, _jsonbs(key), i);
		list[i] = value;
	}
	i_IntListClear(base, NULL, _jsonbs(key));
	return list;
}
void SetFloatList(StaticFunctionTag * base, BSFixedString key, VMArray<float> value){
	i_FloatListClear(base, NULL, _jsonbs(key));
	for (int i = 0; i < value.Length(); i++){
		float elem;
		value.Get(&elem, i);
		i_FloatListAdd(base, NULL, _jsonbs(key), elem, true);
	}
}
VMResultArray<float> GetFloatList(StaticFunctionTag * base, BSFixedString key){
	int count = i_FloatListCount(base, NULL, _jsonbs(key));
	VMResultArray<float> list;
	list.resize(count, 0.0f);
	for (int i = 0; i < count; i++){
		float value = i_FloatListGet(base, NULL, _jsonbs(key), i);
		list[i] = value;
	}
	i_FloatListClear(base, NULL, _jsonbs(key));
	return list;
}
void SetStringList(StaticFunctionTag * base, BSFixedString key, VMArray<BSFixedString> value){
	i_StringListClear(base, NULL, _jsonbs(key));
	for (int i = 0; i < value.Length(); i++){
		BSFixedString elem;
		value.Get(&elem, i);
		i_StringListAdd(base, NULL, _jsonbs(key), elem, true);
	}
}
VMResultArray<BSFixedString> GetStringList(StaticFunctionTag * base, BSFixedString key){
	int count = i_StringListCount(base, NULL, _jsonbs(key));
	VMResultArray<BSFixedString> list;
	list.resize(count, 0);
	for (int i = 0; i < count; i++){
		BSFixedString value = i_StringListGet(base, NULL, _jsonbs(key), i);
		list[i] = value;
	}
	i_StringListClear(base, NULL, _jsonbs(key));
	return list;
}

namespace StorageJSON{
	bool RegisterFuncs(VMClassRegistry * registry){

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, UInt32>("SetInt", "SOS_IO", SetInt, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, BSFixedString>("GetInt", "SOS_IO", GetInt, registry));

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, float>("SetFloat", "SOS_IO", SetFloat, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, float, BSFixedString>("GetFloat", "SOS_IO", GetFloat, registry));

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, VMArray<UInt32>>("SetIntList", "SOS_IO", SetIntList, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<SInt32>, BSFixedString>("GetIntList", "SOS_IO", GetIntList, registry));

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, VMArray<float>>("SetFloatList", "SOS_IO", SetFloatList, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<float>, BSFixedString>("GetFloatList", "SOS_IO", GetFloatList, registry));

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, VMArray<BSFixedString>>("SetStringList", "SOS_IO", SetStringList, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<BSFixedString>, BSFixedString>("GetStringList", "SOS_IO", GetStringList, registry));

		return true;
	};
}