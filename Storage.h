#pragma once

typedef unsigned long long int 	uint64_t;

void LoadModInfo();
uint64_t GetFormKey(TESForm * form);
int GameGetForm(int formId);
UInt8 GetNewOrder(UInt8 order);
UInt32 GetNewId(UInt32 oldId);
uint64_t GetNewKey(uint64_t key);
void EncodeString(std::string& a);
void DecodeString(std::string& a);
bool IsValidObject(TESForm * obj, uint64_t formId);
UInt32 i_Cleanup(StaticFunctionTag * base);

UInt32 i_FormListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value, bool allowDuplicate);
UInt32 i_FormListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_FormListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_FormListFind(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value);
TESForm* i_FormListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index);
UInt32 i_FormListRemove(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value, bool allInstances);
TESForm* i_FormListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, TESForm* value);

UInt32 i_IntListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value, bool allowDuplicate);
UInt32 i_IntListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_IntListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_IntListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index);
UInt32 i_IntListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, UInt32 value);

UInt32 i_FloatListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value, bool allowDuplicate);
UInt32 i_FloatListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_FloatListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
float i_FloatListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index);
float i_FloatListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, float value);

UInt32 i_StringListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, BSFixedString value, bool allowDuplicate);
UInt32 i_StringListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
UInt32 i_StringListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key);
BSFixedString i_StringListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index);

UInt32 i_GetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 missing);
UInt32 i_SetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value);
bool i_UnsetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key);

float i_SetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value);
float i_GetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float missing);
bool i_UnsetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key);

template <typename T>
int RemoveDuplicates(std::map<uint64_t, std::map<std::string, std::vector<T>>> *map, char * keyList);
int RemoveFormDuplicates(char * key);

BSFixedString _jsonbs(BSFixedString key);
BSFixedString _jsonc(const char * key);

namespace Storage{

	void RemoveJSON();
	Json::Value GetJSON();
	void SetJSON(Json::Value root);

	void Init();
	void LoadModInfo();
	int Load(SKSESerializationInterface * intfc);
	void Revert(SKSESerializationInterface * intfc);
	void Save(SKSESerializationInterface * intfc, int);

	bool RegisterFuncs(VMClassRegistry * registry);


}
