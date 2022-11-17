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

// JSON object --> JSON file
bool WriteJSON(Json::Value root, const char * fileName){
	std::ofstream jsonfile(fileName, std::ofstream::out);
	if (jsonfile){
		jsonfile << Json::StyledWriter().write(root);
		jsonfile.close();
		return true;
	}
	_FATALERROR("Could not write to %s", fileName);
	return false;
}

// JSON file --> JSON object
bool ReadJSON(Json::Value * root, const char * fileName){
	std::ifstream jsonfile(fileName, std::ofstream::in);
	if (jsonfile){
		bool result = Json::Reader().parse(jsonfile, *root);
		jsonfile.close();
		if (!result){
			_FATALERROR("Could not parse JSON from %s", fileName);
		}
		return result;
	}
	_FATALERROR("Could not read from %s", fileName);
	return false;
}

bool LoadJSON(StaticFunctionTag * base, BSFixedString file){
	if (!(file != NULL)){
		_FATALERROR("Invalid argument file");
		return false;
	}

	// clean memory
	Storage::RemoveJSON();

	_MESSAGE("Loading settings from %s", file.data);

	// read from file
	Json::Value root;
	bool ok = ReadJSON(&root, file.data);

	// store in memory
	if (ok){
		Storage::SetJSON(root);
	}

	return ok;
}

bool SaveJSON(StaticFunctionTag * base, BSFixedString file){
	if (!(file != NULL)){
		_FATALERROR("Invalid argument file");
		return false;
	}

	// get the JSON from memory
	Json::Value root = Storage::GetJSON();

	_MESSAGE("Saving settings to %s", file.data);

	// write it to a file
	bool ok = WriteJSON(root, file.data);

	// clean memory
	Storage::RemoveJSON();

	return ok;
}

namespace SosIO{
	bool RegisterFuncs(VMClassRegistry * registry){

		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>("Save", "SOS_IO", SaveJSON, registry));
		registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>("Load", "SOS_IO", LoadJSON, registry));

		return true;
	};
}