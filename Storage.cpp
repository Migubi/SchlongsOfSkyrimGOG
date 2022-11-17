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
#include "skse64/GameForms.h"

#include "Storage.h"
#pragma warning(disable:4503)

std::map<uint64_t, std::map<std::string, std::vector<int>>> * savedFormList = NULL;
std::map<uint64_t, std::map<std::string, std::vector<int>>> * savedIntList = NULL;
std::map<uint64_t, std::map<std::string, std::vector<float>>> * savedFloatList = NULL;
std::map<uint64_t, std::map<std::string, std::vector<std::string>>> * savedStringList = NULL;
std::map<uint64_t, std::map<std::string, int>> * savedInt = NULL;
std::map<uint64_t, std::map<std::string, float>> * savedFloat = NULL;

int loadedModsCount = 0;

std::vector <std::string> CurrentMods = {};
std::vector <std::string> LoadedMods = {};

ICriticalSection	mutex;
bool loadedModInfo = false;

enum Action {
	ERASE, STORE, DEBUG
};

bool hasJSONPrefix(const std::string str) {
	std::string prefix("json__");
	return !str.compare(0, prefix.size(), prefix);
}

void ValueToJSON(Json::Value * root, std::string name, int value) {
	(*root)[name] = value;
	_DMESSAGE("stored 1 entry");
}
void ValueToJSON(Json::Value * root, std::string name, float value) {
	(*root)[name] = value;
	_DMESSAGE("stored 1 entry");
}
void ValueToJSON(Json::Value * root, std::string name, std::vector<int> value) {
	for (std::vector<int>::iterator itr = value.begin(); itr != value.end(); itr++) {
		(*root)[name].append(*itr);
	}
	_DMESSAGE("stored %d entries", (*root)[name].size());
}
void ValueToJSON(Json::Value * root, std::string name, std::vector<float> value) {
	for (std::vector<float>::iterator itr = value.begin(); itr != value.end(); itr++) {
		(*root)[name].append(*itr);
	}
	_DMESSAGE("stored %d entries", (*root)[name].size());
}
void ValueToJSON(Json::Value * root, std::string name, std::vector<std::string> value) {
	for (std::vector<std::string>::iterator itr = value.begin(); itr != value.end(); itr++) {
		(*root)[name].append(*itr);
	}
	_DMESSAGE("stored %d entries", (*root)[name].size());
}

void debug_entries(std::string name, int entry) {
	_DMESSAGE("  debug_entries for %s:", name.c_str());
	_DMESSAGE("    %d", entry);
}
void debug_entries(std::string name, float entry) {
	_DMESSAGE("  debug_entries for %s:", name.c_str());
	_DMESSAGE("    %f", entry);
}
void debug_entries(std::string name, std::vector<int> entries) {
	_DMESSAGE("  debug_entries for %s:", name.c_str());
	std::string str;
	for (std::vector<int>::iterator itr = entries.begin(); itr != entries.end(); itr++) {
		char value[200];
		memset(value, '\0', sizeof(value));
		sprintf_s(value, "%d", *itr);
		if (str.length() != 0) {
			str.append(", ");
		}
		str.append(value);
	}
	_DMESSAGE("    [%s]", str.c_str());
}
void debug_entries(std::string name, std::vector<float> entries) {
	_DMESSAGE("  debug_entries for %s:", name.c_str());
	std::string str;
	for (std::vector<float>::iterator itr = entries.begin(); itr != entries.end(); itr++) {
		char value[200];
		memset(value, '\0', sizeof(value));
		sprintf_s(value, "%f", *itr);
		if (str.length() != 0) {
			str.append(", ");
		}
		str.append(value);
	}
	_DMESSAGE("    [%s]", str.c_str());
}
void debug_entries(std::string name, std::vector<std::string> entries) {
	_DMESSAGE("  debug_entries for %s:", name.c_str());
	std::string str;
	for (std::vector<std::string>::iterator itr = entries.begin(); itr != entries.end(); itr++) {
		if (str.length() != 0) {
			str.append(", ");
		}
		str.append((*itr).c_str());
	}
	_DMESSAGE("    [%s]", str.c_str());
}

template <typename T>
void debug_map(std::map < uint64_t, std::map<std::string, T>> * map) {
	_DMESSAGE(" **** debug map **** ");
	for (std::map < uint64_t, std::map<std::string, T>>::iterator itr = map->begin(); itr != map->end(); itr++) {
		_DMESSAGE("  id %d entries %d", itr->first, itr->second.size());
		for (std::map<std::string, T>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++) {
			debug_entries(itr2->first, itr2->second);
		}
	}
}

template <typename T>
void ProcessJSON(Action action, std::map < uint64_t, std::map<std::string, T>> * map, Json::Value * root) {
	mutex.Enter();

	std::map < uint64_t, std::map<std::string, T>>::iterator itr = map->begin();

	while (itr != map->end()) {

		bool deleted = false;
		std::map<std::string, T>::iterator itr2 = itr->second.begin();

		while (itr2 != itr->second.end() && !deleted) {

			bool deleted2 = false;
			if (hasJSONPrefix(itr2->first)) {
				if (action == ERASE) {
					itr->second.erase(itr2++);
					deleted2 = true;
					if (itr->second.size() == 0) {
						map->erase(itr++);
						deleted = true;
					}
				}
				else if (action == STORE) {
					ValueToJSON(root, itr2->first.substr(6), itr2->second);
				}
			}
			if (!deleted2) {
				++itr2;
			}
		}
		if (!deleted) {
			++itr;
		}
	}
	mutex.Leave();
}


BSFixedString _jsonbs(BSFixedString key) {
	return BSFixedString(std::string("json__").append(key.data).c_str());
}
BSFixedString _jsonc(const char * key) {
	return BSFixedString(std::string("json__").append(key).c_str());
}

bool isIntPrefix(const char * name) {
	return name[0] == 'i' || name[0] == 'b';
}
bool isFloatPrefix(const char * name) {
	return name[0] == 'f';
}
bool isStringPrefix(const char * name) {
	return name[0] == 's';
}

void parseArray(StaticFunctionTag * base, const char * name, Json::Value arr) {
	for (auto item : arr) {
		if (isStringPrefix(name) && item.isString()) {
			i_StringListAdd(base, NULL, _jsonc(name), BSFixedString(item.asCString()), true);
		}
		else if (isFloatPrefix(name) && item.isNumeric()) {
			i_FloatListAdd(base, NULL, _jsonc(name), item.asFloat(), true);
		}
		else if (isIntPrefix(name) && item.isInt()) {
			i_IntListAdd(base, NULL, _jsonc(name), item.asInt(), true);
		}
		else {
			_VMESSAGE("Unknown JSON element: %s", name);
		}
	}
}

template <typename T>
int RemoveDuplicates(std::map<uint64_t, std::map<std::string, std::vector<T>>> *map, char * keyList) {
	int removed = 0;
	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, std::vector<T>>>::iterator itr = map->begin(); itr != map->end(); itr++) {
		for (std::map<std::string, std::vector<T>>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(keyList, itr2->first.c_str())) {
				int count = itr2->second.size();
				std::set<T> s(itr2->second.begin(), itr2->second.end());
				itr2->second.clear();
				itr2->second.assign(s.begin(), s.end());
				removed += count - itr2->second.size();
			}
		}
	}
	mutex.Leave();
	return removed;
}

int RemoveFormDuplicates(char * key) {

	int removed = RemoveDuplicates(savedFormList, key);
	if (removed > 0) {
		_MESSAGE("  removed %i duplicated entries from %s", removed, key);
	}
	return removed;
}

bool validKey(BSFixedString key) {

	if (!(key != NULL) || !key.data || strlen(key.data) == 0)
		return false;

	if (strlen(key.data) >= 2 && key.data[0] == '_' && key.data[1] == '_')
		return false;

	return true;
}

uint64_t GetFormKey(TESForm * form)
{
	if (!form)
		return 0;

	uint64_t key = form->formID;
	key |= ((uint64_t)form->formType) << 32;
	return key;
}

//int GameGetForm(int formId)
//{
//	
//	if (formId == 0)
//		return 0;
//
//	int retVal = 0;
//	
//	/****/
//	LookupFormByID(formId);
//	/*_asm
//	{
//		mov ecx, 0xFF0000
//			push formId
//			call getTesFormId
//			add esp, 4
//			mov retVal, eax
//	}*/
//
//	return retVal;
//}

UInt8 GetNewOrder(UInt8 order)
{
	if (order == 0xFF || order == 0xFE || order == 0 || !_stricmp(CurrentMods[order].c_str(), LoadedMods[order].c_str()))
		return order;

	for (int i = 0; i < LoadedMods.size(); i++)
	{

		if (CurrentMods.size() < LoadedMods.size())
		{
			int x = CurrentMods.size();
			CurrentMods.resize(LoadedMods.size());
			for (x; x < LoadedMods.size(); x++)
			{
				CurrentMods[x] = "nomod";
			}
		}

		//_VMESSAGE("CurrentMods[%i] - %s", i, CurrentMods[i].c_str());
		//_VMESSAGE("LoadedMods[%i]: %s", order, LoadedMods[order].c_str());

		if (i != 253)
		{
			if (!_stricmp(CurrentMods[i].c_str(), LoadedMods[order].c_str()))
				return (UInt8)i;
		}
	}

	return 0xFF;
}

UInt32 GetNewId(UInt32 oldId)
{
	if (oldId == 0)
		return 0;
	UInt32 id = oldId & 0x00FFFFFF;
	UInt8 oldMod = (UInt8)(oldId >> 24);
	UInt8 mod = GetNewOrder(oldMod);
	if (oldMod != 0xFF && mod == 0xFF)
		return 0;
	return (((UInt32)mod) << 24) | id;
}

uint64_t GetNewKey(uint64_t key)
{
	if (key == 0)
		return 0;
	UInt32 type = (UInt32)(key >> 32);
	UInt32 id = GetNewId((UInt32)(key));
	return (((uint64_t)type) << 32) | (uint64_t)id;
}

void EncodeString(std::string& a)
{
	std::replace(a.begin(), a.end(), ' ', (char)0x7);
	if (a.empty())
		a += (char)0x1B;
}

void DecodeString(std::string& a)
{
	if (a.size() == 1 && a[0] == (char)0x1B)
		a.clear();
	else
		std::replace(a.begin(), a.end(), (char)0x7, ' ');
}

bool IsValidObject(TESForm * obj, uint64_t formId)
{
	if (obj == NULL)
		return false;

	if ((UInt32)(formId >> 32) != 0 && obj->formType != (UInt32)(formId >> 32))
		return false;

	if ((formId & 0xFFFFFFFF) != 0 && (UInt32)(formId & 0xFFFFFFFF) != obj->formID)
		return false;

	// Add more conditions here if necessary.

	return true;
}

UInt32 i_Cleanup(StaticFunctionTag * base)
{
	int cleaned = 0;

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->begin(); itr != savedFormList->end();)
	{
		if (itr->first == 0)
		{
			itr++;
			continue;
		}

		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedFormList->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->begin(); itr != savedIntList->end();)
	{
		if (itr->first == 0)
		{
			itr++;
			continue;
		}

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));

		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedIntList->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->begin(); itr != savedFloatList->end();)
	{
		if (itr->first == 0)
		{
			itr++;
			continue;
		}

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));

		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedFloatList->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->begin(); itr != savedStringList->end();)
	{
		if (itr->first == 0)
		{
			itr++;

			continue;
		}

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));

		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedStringList->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, int>>::iterator itr = savedInt->begin(); itr != savedInt->end();)
	{
		if (itr->first == 0)
		{
			itr++;
			continue;
		}

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));

		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedInt->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	mutex.Enter();
	for (std::map<uint64_t, std::map<std::string, float>>::iterator itr = savedFloat->begin(); itr != savedFloat->end();)
	{
		if (itr->first == 0)
		{
			itr++;
			continue;
		}

		//TESForm * ptr = (TESForm*)GameGetForm((UInt32)(itr->first & 0xFFFFFFFF));
		TESForm * ptr = LookupFormByID((UInt32)(itr->first & 0xFFFFFFFF));
		if (!ptr || !IsValidObject(ptr, itr->first))
		{
			cleaned++;
			itr = savedFloat->erase(itr);
		}
		else
			itr++;
	}
	mutex.Leave();

	return cleaned;
}

UInt32 i_FormListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value, bool allowDuplicate)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr == savedFormList->end())
	{
		(*savedFormList)[objKey][key.data].push_back(value ? value->formID : 0);
	}
	else
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (allowDuplicate || std::find(itr2->second.begin(), itr2->second.end(), value ? value->formID : 0) == itr2->second.end())
				{
					itr2->second.push_back(value ? value->formID : 0);
					r = itr2->second.size() - 1;
					break;
				}
				r = -1;
				break;
			}
		}
		if (itr2 == itr->second.end())
			itr->second[key.data].push_back(value ? value->formID : 0);
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value, bool allowDuplicate)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr == savedIntList->end())
	{
		(*savedIntList)[objKey][key.data].push_back(value);
	}
	else
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (allowDuplicate || std::find(itr2->second.begin(), itr2->second.end(), value) == itr2->second.end())
				{
					itr2->second.push_back(value);
					r = itr2->second.size() - 1;
					break;
				}
				r = -1;
				break;
			}
		}

		if (itr2 == itr->second.end())
			itr->second[key.data].push_back(value);
	}
	mutex.Leave();
	return r;
}

UInt32 i_FloatListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value, bool allowDuplicate)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr == savedFloatList->end())
	{
		(*savedFloatList)[objKey][key.data].push_back(value);
	}
	else
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (allowDuplicate || std::find(itr2->second.begin(), itr2->second.end(), value) == itr2->second.end())
				{
					itr2->second.push_back(value);
					r = itr2->second.size() - 1;
					break;
				}
				r = -1;
				break;
			}
		}

		if (itr2 == itr->second.end())
			itr->second[key.data].push_back(value);
	}
	mutex.Leave();
	return r;
}

UInt32 i_StringListAdd(StaticFunctionTag * base, TESForm * obj, BSFixedString key, BSFixedString value, bool allowDuplicate)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();

	std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->find(objKey);
	if (itr == savedStringList->end())
	{
		(*savedStringList)[objKey][key.data].push_back(value.data);
	}
	else
	{
		std::map<std::string, std::vector<std::string>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (!allowDuplicate)
				{
					for (std::vector<std::string>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
					{
						if (!_stricmp((*itr3).c_str(), value.data))
						{
							r = -1;
							break;
						}
					}
				}
				if (r >= 0)
				{
					itr2->second.push_back(value.data);
					r = itr2->second.size() - 1;
				}
				break;
			}
		}

		if (itr2 == itr->second.end())
			itr->second[key.data].push_back(value.data);
	}
	mutex.Leave();
	return r;
}

UInt32 i_FormListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}

	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = itr2->second.size();
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedFormList->erase(itr);
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = itr2->second.size();
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedIntList->erase(itr);
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FloatListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = itr2->second.size();
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedFloatList->erase(itr);
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_StringListClear(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->find(objKey);
	if (itr != savedStringList->end())
	{
		std::map<std::string, std::vector<std::string>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = itr2->second.size();
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedStringList->erase(itr);
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FormListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				r = (int)itr2->second.size();
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				r = (int)itr2->second.size();
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FloatListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				r = (int)itr2->second.size();
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_StringListCount(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->find(objKey);
	if (itr != savedStringList->end())
	{
		std::map<std::string, std::vector<std::string>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				r = (int)itr2->second.size();
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FormListFind(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = -1;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		int fValue = value ? value->formID : 0;
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				for (std::vector<int>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++, ct++)
				{
					if ((*itr3) == fValue)
					{
						r = ct;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListFind(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = -1;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				for (std::vector<int>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++, ct++)
				{
					if ((*itr3) == value)
					{
						r = ct;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FloatListFind(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value)
{
	if (!validKey(key)) {
		return -1;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = -1;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				for (std::vector<float>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++, ct++)
				{
					if ((*itr3) == value)
					{
						r = ct;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

TESForm* i_FormListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	TESForm * r = NULL;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (index >= 0 && index < itr2->second.size())
				{
					int retv = itr2->second.at(index);
					if (retv != 0)
						r = (TESForm*)LookupFormByID(retv);
					//r = (TESForm*)GameGetForm(retv);
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (index >= 0 && index < itr2->second.size())
					r = itr2->second.at(index);
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

float i_FloatListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index)
{
	if (!validKey(key)) {
		return 0.0f;
	}
	uint64_t objKey = GetFormKey(obj);
	float r = 0.0f;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (index >= 0 && index < itr2->second.size())
					r = itr2->second.at(index);
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

BSFixedString i_StringListGet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index)
{
	if (!validKey(key)) {
		return NULL;
	}
	uint64_t objKey = GetFormKey(obj);
	BSFixedString r = NULL;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->find(objKey);
	if (itr != savedStringList->end())
	{
		std::map<std::string, std::vector<std::string>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				if (index >= 0 && index < itr2->second.size())
					r = BSFixedString(itr2->second.at(index).c_str());
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FormListRemove(StaticFunctionTag * base, TESForm * obj, BSFixedString key, TESForm* value, bool allInstances)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		int fValue = value ? value->formID : 0;
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				std::vector<int>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end();)
				{
					if ((*itr3) == fValue)
					{
						itr3 = itr2->second.erase(itr3);
						ct++;
						if (!allInstances)
							break;
					}
					else
						itr3++;
				}
				if (itr2->second.size() == 0)
				{
					itr->second.erase(itr2);
					if (itr->second.size() == 0)
						savedFormList->erase(itr);
				}
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListRemove(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value, bool allInstances)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				std::vector<int>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end();)
				{
					if ((*itr3) == value)
					{
						itr3 = itr2->second.erase(itr3);
						ct++;
						if (!allInstances)
							break;
					}
					else
						itr3++;
				}
				if (itr2->second.size() == 0)
				{
					itr->second.erase(itr2);
					if (itr->second.size() == 0)
						savedIntList->erase(itr);
				}
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_FloatListRemove(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value, bool allInstances)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				int ct = 0;
				std::vector<float>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end();)
				{
					if ((*itr3) == value)
					{
						itr3 = itr2->second.erase(itr3);
						ct++;
						if (!allInstances)
							break;
					}
					else
						itr3++;
				}
				if (itr2->second.size() == 0)
				{
					itr->second.erase(itr2);
					if (itr->second.size() == 0)
						savedFloatList->erase(itr);
				}
				r = ct;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

TESForm* i_FormListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, TESForm* value)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	TESForm * r = NULL;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->find(objKey);
	if (itr != savedFormList->end())
	{
		int nIndex = index;

		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				std::vector<int>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
				{
					if (--nIndex < 0)
					{
						int ct = *itr3;
						itr2->second[index] = value ? value->formID : 0;
						//r = ct != 0 ? (TESForm*)GameGetForm(ct) : NULL;
						r = ct != 0 ? (TESForm*)LookupFormByID(ct) : NULL;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_IntListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, UInt32 value)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int r = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->find(objKey);
	if (itr != savedIntList->end())
	{
		int nIndex = index;

		std::map<std::string, std::vector<int>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				std::vector<int>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
				{
					if (--nIndex < 0)
					{
						int ct = *itr3;
						itr2->second[index] = value;
						r = ct;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

float i_FloatListSet(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 index, float value)
{
	if (!validKey(key)) {
		return 0.0f;
	}
	uint64_t objKey = GetFormKey(obj);
	float r = 0.0f;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->find(objKey);
	if (itr != savedFloatList->end())
	{
		int nIndex = index;

		std::map<std::string, std::vector<float>>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				std::vector<float>::iterator itr3;
				for (itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
				{
					if (--nIndex < 0)
					{
						float ct = *itr3;
						itr2->second[index] = value;
						r = ct;
						break;
					}
				}
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

UInt32 i_GetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 missing)
{
	if (!validKey(key)) {
		return missing;
	}
	uint64_t objKey = GetFormKey(obj);
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, int>>::iterator itr = savedInt->find(objKey);
	if (itr != savedInt->end())
	{
		std::map<std::string, int>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(itr2->first.c_str(), key.data))
			{
				missing = itr2->second;
				break;
			}
		}
	}
	mutex.Leave();
	return missing;
}

float i_GetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float missing)
{
	if (!validKey(key)) {
		return missing;
	}
	uint64_t objKey = GetFormKey(obj);
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, float>>::iterator itr = savedFloat->find(objKey);
	if (itr != savedFloat->end())
	{
		std::map<std::string, float>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(itr2->first.c_str(), key.data))
			{
				missing = itr2->second;
				break;
			}
		}
	}
	mutex.Leave();
	return missing;
}

UInt32 i_SetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, UInt32 value)
{
	if (!validKey(key)) {
		return 0;
	}
	uint64_t objKey = GetFormKey(obj);
	int ct = 0;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, int>>::iterator itr = savedInt->find(objKey);
	if (itr == savedInt->end())
	{
		(*savedInt)[objKey][key.data] = value;
	}
	else
	{
		std::map<std::string, int>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				ct = itr2->second;
				itr2->second = value;
				break;
			}
		}

		if (itr2 == itr->second.end())
			itr->second[key.data] = value;
	}
	mutex.Leave();
	return ct;
}

float i_SetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key, float value)
{
	if (!validKey(key)) {
		return 0.0f;
	}
	uint64_t objKey = GetFormKey(obj);
	float ct = 0.0f;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, float>>::iterator itr = savedFloat->find(objKey);
	if (itr == savedFloat->end())
	{
		(*savedFloat)[objKey][key.data] = value;
	}
	else
	{
		std::map<std::string, float>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				ct = itr2->second;
				itr2->second = value;
				break;
			}
		}

		if (itr2 == itr->second.end())
			itr->second[key.data] = value;
	}
	mutex.Leave();
	return ct;
}
bool i_UnsetIntValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return false;
	}
	uint64_t objKey = GetFormKey(obj);
	bool r = false;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, int>>::iterator itr = savedInt->find(objKey);
	if (itr != savedInt->end())
	{
		std::map<std::string, int>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedInt->erase(itr);
				r = true;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

bool i_UnsetFloatValue(StaticFunctionTag * base, TESForm * obj, BSFixedString key)
{
	if (!validKey(key)) {
		return false;
	}
	uint64_t objKey = GetFormKey(obj);
	bool r = false;
	mutex.Enter();
	std::map<uint64_t, std::map<std::string, float>>::iterator itr = savedFloat->find(objKey);
	if (itr != savedFloat->end())
	{
		std::map<std::string, float>::iterator itr2;
		for (itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			if (!_stricmp(key.data, itr2->first.c_str()))
			{
				itr->second.erase(itr2);
				if (itr->second.size() == 0)
					savedFloat->erase(itr);
				r = true;
				break;
			}
		}
	}
	mutex.Leave();
	return r;
}

namespace Storage {

	// remove all entries prefixed with json__
	void RemoveJSON() {
		_DMESSAGE("Deleting savedIntList");
		ProcessJSON(ERASE, savedIntList, NULL);
		_DMESSAGE("Deleting savedFloatList");
		ProcessJSON(ERASE, savedFloatList, NULL);
		_DMESSAGE("Deleting savedStringList");
		ProcessJSON(ERASE, savedStringList, NULL);
		_DMESSAGE("Deleting savedInt");
		ProcessJSON(ERASE, savedInt, NULL);
		_DMESSAGE("Deleting savedFloat");
		ProcessJSON(ERASE, savedFloat, NULL);
	}

	// get all entries prefixed with json__
	Json::Value GetJSON() {
		Json::Value root;
		_DMESSAGE("Storing savedIntList");
		ProcessJSON(STORE, savedIntList, &root);
		_DMESSAGE("Storing savedFloatList");
		ProcessJSON(STORE, savedFloatList, &root);
		_DMESSAGE("Storing savedStringList");
		ProcessJSON(STORE, savedStringList, &root);
		_DMESSAGE("Storing savedInt");
		ProcessJSON(STORE, savedInt, &root);
		_DMESSAGE("Storing savedFloat");
		ProcessJSON(STORE, savedFloat, &root);
		return root;
	}

	// parse the JSON object
	// fill the appropiate arrays
	void SetJSON(Json::Value root) {
		for (auto member : root.getMemberNames()) {
			const char * name = member.c_str();
			Json::Value value = root[name];
			if (value.isArray()) {
				parseArray(NULL, name, value);
			}
			else if (isFloatPrefix(name) && value.isNumeric()) {
				i_SetFloatValue(NULL, NULL, _jsonc(name), value.asFloat());
			}
			else if (isIntPrefix(name) && value.isInt()) {
				i_SetIntValue(NULL, NULL, _jsonc(name), value.asInt());
			}
			else {
				_VMESSAGE("Unknown JSON element: %s", name);
			}
		}
	}

	void Init()
	{
		if (savedFormList) {
			return;
		}
		savedFormList = new std::map<uint64_t, std::map<std::string, std::vector<int>>>();
		savedIntList = new std::map<uint64_t, std::map<std::string, std::vector<int>>>();
		savedFloatList = new std::map<uint64_t, std::map<std::string, std::vector<float>>>();
		savedStringList = new std::map<uint64_t, std::map<std::string, std::vector<std::string>>>();
		savedInt = new std::map<uint64_t, std::map<std::string, int>>();
		savedFloat = new std::map<uint64_t, std::map<std::string, float>>();
	}

	void LoadModInfo()
	{
		_VMESSAGE("\nloading mod info");
		DataHandler * data = DataHandler::GetSingleton();
		loadedModsCount = data->modList.loadedMods.count;

		//for (int i = 0; i < maxModCount; i++) {
		//	CurrentMods[i] = "";
		//}
		CurrentMods.resize(loadedModsCount);

		for (int i = 0; i < loadedModsCount; i++)
		{
			const char * name;

			//experimental fix for slot 255
			if (i == 254)
				name = "nomod";

			if (data->modList.loadedMods[i])
			{
				name = data->modList.loadedMods[i]->name;
				_DMESSAGE("  %02X (%03i): %s", i, i, name);
			}
			else {
				name = "";
			}
			CurrentMods[i] = name;
		}
	}

	int Load(SKSESerializationInterface * intfc)
	{
		UInt32	type;
		UInt32	version;
		int     storedVersion = 0;
		UInt32	length;
		bool	error = false;
		int		savedModsCount = 0;

		if (!loadedModInfo) {
			loadedModInfo = true;
			LoadModInfo();
		}

		while (!error && intfc->GetNextRecordInfo(&type, &version, &length))
		{
			switch (type)
			{
			case 'DATA':
			{
				if (version == SKSESerializationInterface::kVersion)
				{
					_VMESSAGE("\nSAVEGAME LOAD\nloading savegame, StorageUtil section size: %u", length);

					if (length)
					{
						char * buf = new char[length + 1];

						intfc->ReadRecordData(buf, length);
						buf[length] = 0;

						std::stringstream ss(buf);

						ss >> storedVersion;

						int count;

						_VMESSAGE("storedVersion: %i", storedVersion);

						if (storedVersion >= -1)
						{
							_DMESSAGE("loading stored mod info");

							ss >> savedModsCount;
							_VMESSAGE("found %i ES(M,P)s saved in co-save", savedModsCount);

							LoadedMods.resize(savedModsCount);
							for (int mi = 0; mi < savedModsCount; mi++)
							{
								DataHandler * data = DataHandler::GetSingleton();
								std::string strs;
								ss >> strs;
								DecodeString(strs);
								LoadedMods[mi] = strs;
								_DMESSAGE("  %s", strs.c_str());
								//if (!_stricmp(LoadedMods[mi].c_str(), "nomod")) {
								//	LoadedMods[mi] = "";
								//}
								//else {
								//	_DMESSAGE("  %s", strs.c_str());
								//}
							}

							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									int count3;
									ss >> key2;
									DecodeString(key2);
									ss >> count3;
									for (int k = 0; k < count3; k++)
									{
										int val;
										ss >> val;
										val = GetNewId(val);
										(*savedFormList)[key][key2].push_back(val);
									}
								}
							}
							mutex.Leave();
						}

						if (storedVersion >= 205037) {
							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									int count3;
									ss >> key2;
									DecodeString(key2);
									ss >> count3;

									for (int k = 0; k < count3; k++)
									{
										int val;
										ss >> val;
										(*savedIntList)[key][key2].push_back(val);
									}
								}
							}
							mutex.Leave();

							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									int count3;
									ss >> key2;
									DecodeString(key2);
									ss >> count3;
									for (int k = 0; k < count3; k++)
									{
										float val;
										ss >> val;
										(*savedFloatList)[key][key2].push_back(val);
									}
								}
							}
							mutex.Leave();

							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									int val;

									ss >> key2;
									DecodeString(key2);
									ss >> val;

									(*savedInt)[key][key2] = val;
								}
							}
							mutex.Leave();
						}

						if (storedVersion >= 300002) {
							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									float val;

									ss >> key2;
									DecodeString(key2);
									ss >> val;

									(*savedFloat)[key][key2] = val;
								}
							}
							mutex.Leave();

							ss >> count;

							mutex.Enter();
							for (int i = 0; i < count; i++)
							{
								uint64_t key;
								int count2;
								ss >> key;
								key = GetNewKey(key);
								ss >> count2;
								for (int j = 0; j < count2; j++)
								{
									std::string key2;
									int count3;
									ss >> key2;
									DecodeString(key2);
									ss >> count3;

									for (int k = 0; k < count3; k++)
									{
										std::string val;
										ss >> val;
										DecodeString(val);
										(*savedStringList)[key][key2].push_back(val);
									}
								}
							}
							mutex.Leave();
						}

						delete[] buf;
					}
				}
			}
			break;

			default:
				error = true;
				break;
			}
		}

		return storedVersion;
	}

	void Revert(SKSESerializationInterface * intfc)
	{
		_VMESSAGE("\n REVERT");

		mutex.Enter();
		savedFormList->clear();
		mutex.Leave();

		mutex.Enter();
		savedIntList->clear();
		mutex.Leave();

		mutex.Enter();
		savedFloatList->clear();
		mutex.Leave();

		mutex.Enter();
		savedStringList->clear();
		mutex.Leave();

		mutex.Enter();
		savedInt->clear();
		mutex.Leave();

		mutex.Enter();
		savedFloat->clear();
		mutex.Leave();

		if (!loadedModInfo) {
			loadedModInfo = true;
			LoadModInfo();
		}

		for (int i = 0; i < LoadedMods.size(); i++)
			LoadedMods[i] = "";

	}

	void Save(SKSESerializationInterface * intfc, int pluginVersion)
	{

		if (!loadedModInfo) {
			loadedModInfo = true;
			LoadModInfo();

			LoadedMods.resize(loadedModsCount);
			for (int i = 0; i < LoadedMods.size(); i++)
				LoadedMods[i] = "";
		}

		i_Cleanup(NULL);

		if (intfc->OpenRecord('DATA', SKSESerializationInterface::kVersion))
		{
			std::stringstream ss;
			ss << pluginVersion;
			ss << ' ' << loadedModsCount;
			for (int mi = 0; mi < loadedModsCount; mi++)
			{
				std::string strs = CurrentMods[mi];
				if (!strs.empty())
				{
					EncodeString(strs);
					ss << ' ' << strs;
				}
				//else
				//	ss << ' ' << "nomod";
			}

			mutex.Enter();
			ss << ' ' << (int)savedFormList->size();
			for (std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedFormList->begin(); itr != savedFormList->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, std::vector<int>>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << (int)itr2->second.size();
					for (std::vector<int>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
					{
						ss << ' ' << *itr3;
					}
				}
			}
			mutex.Leave();

			mutex.Enter();
			ss << ' ' << (int)savedIntList->size();
			for (std::map<uint64_t, std::map<std::string, std::vector<int>>>::iterator itr = savedIntList->begin(); itr != savedIntList->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, std::vector<int>>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << (int)itr2->second.size();
					for (std::vector<int>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
					{
						ss << ' ' << *itr3;
					}
				}
			}
			mutex.Leave();

			mutex.Enter();
			ss << ' ' << (int)savedFloatList->size();
			for (std::map<uint64_t, std::map<std::string, std::vector<float>>>::iterator itr = savedFloatList->begin(); itr != savedFloatList->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, std::vector<float>>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << (int)itr2->second.size();
					for (std::vector<float>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
					{
						ss << ' ' << *itr3;
					}
				}
			}
			mutex.Leave();

			mutex.Enter();
			ss << ' ' << (int)savedInt->size();
			for (std::map<uint64_t, std::map<std::string, int>>::iterator itr = savedInt->begin(); itr != savedInt->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, int>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << itr2->second;
				}
			}
			mutex.Leave();

			mutex.Enter();
			ss << ' ' << (int)savedFloat->size();
			for (std::map<uint64_t, std::map<std::string, float>>::iterator itr = savedFloat->begin(); itr != savedFloat->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, float>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << itr2->second;
				}
			}
			mutex.Leave();

			mutex.Enter();
			ss << ' ' << (int)savedStringList->size();
			for (std::map<uint64_t, std::map<std::string, std::vector<std::string>>>::iterator itr = savedStringList->begin(); itr != savedStringList->end(); itr++)
			{
				ss << ' ' << itr->first;
				ss << ' ' << (int)itr->second.size();
				for (std::map<std::string, std::vector<std::string>>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
				{
					std::string strs = itr2->first;
					EncodeString(strs);
					ss << ' ' << strs;
					ss << ' ' << (int)itr2->second.size();
					for (std::vector<std::string>::iterator itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++)
					{
						std::string strs2 = *itr3;
						EncodeString(strs2);
						ss << ' ' << strs2;
					}
				}
			}
			mutex.Leave();

			std::string str = ss.str();
			const char * cstr = str.c_str();
			UInt32 str_length = strlen(cstr);
			intfc->WriteRecordData(cstr, str_length);

		}
	}



	bool RegisterFuncs(VMClassRegistry * registry) {

		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, TESForm*, bool>("FormListAdd", "SOS_SKSE", i_FormListAdd, registry));

		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32, bool>("IntListAdd", "SOS_SKSE", i_IntListAdd, registry));
		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, float, bool>("FloatListAdd", "SOS_SKSE", i_FloatListAdd, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListAdd", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListAdd", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListAdd", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("FormListClear", "SOS_SKSE", i_FormListClear, registry));
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("IntListClear", "SOS_SKSE", i_IntListClear, registry));
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("FloatListClear", "SOS_SKSE", i_FloatListClear, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListClear", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListClear", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListClear", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("FormListCount", "SOS_SKSE", i_FormListCount, registry));
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("IntListCount", "SOS_SKSE", i_IntListCount, registry));
		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, TESForm*, BSFixedString>("FloatListCount", "SOS_SKSE", i_FloatListCount, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListCount", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListCount", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListCount", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, TESForm*>("FormListFind", "SOS_SKSE", i_FormListFind, registry));
		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32>("IntListFind", "SOS_SKSE", i_IntListFind, registry));
		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, float>("FloatListFind", "SOS_SKSE", i_FloatListFind, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListFind", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListFind", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListFind", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, TESForm*, TESForm*, BSFixedString, UInt32>("FormListGet", "SOS_SKSE", i_FormListGet, registry));
		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32>("IntListGet", "SOS_SKSE", i_IntListGet, registry));
		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, float, TESForm*, BSFixedString, UInt32>("FloatListGet", "SOS_SKSE", i_FloatListGet, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListGet", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListGet", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListGet", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, TESForm*, bool>("FormListRemove", "SOS_SKSE", i_FormListRemove, registry));
		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32, bool>("IntListRemove", "SOS_SKSE", i_IntListRemove, registry));
		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, float, bool>("FloatListRemove", "SOS_SKSE", i_FloatListRemove, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListRemove", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListRemove", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListRemove", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, TESForm*, TESForm*, BSFixedString, UInt32, TESForm*>("FormListSet", "SOS_SKSE", i_FormListSet, registry));
		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32, UInt32>("IntListSet", "SOS_SKSE", i_IntListSet, registry));
		registry->RegisterFunction(new NativeFunction4<StaticFunctionTag, float, TESForm*, BSFixedString, UInt32, float>("FloatListSet", "SOS_SKSE", i_FloatListSet, registry));

		registry->SetFunctionFlags("SOS_SKSE", "FormListSet", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "IntListSet", VMClassRegistry::kFunctionFlag_NoWait);
		registry->SetFunctionFlags("SOS_SKSE", "FloatListSet", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32>("GetIntValue", "SOS_SKSE", i_GetIntValue, registry));

		registry->SetFunctionFlags("SOS_SKSE", "GetIntValue", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, TESForm*, BSFixedString, UInt32>("SetIntValue", "SOS_SKSE", i_SetIntValue, registry));

		registry->SetFunctionFlags("SOS_SKSE", "SetIntValue", VMClassRegistry::kFunctionFlag_NoWait);

		registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, TESForm*, BSFixedString>("UnsetIntValue", "SOS_SKSE", i_UnsetIntValue, registry));

		registry->SetFunctionFlags("SOS_SKSE", "UnsetIntValue", VMClassRegistry::kFunctionFlag_NoWait);

		return true;
	};

}
