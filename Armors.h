#pragma once

namespace Armors{

	TESForm* FindFirstBodyAA(TESObjectARMO * pArmor);

	bool AddSlotToMask(StaticFunctionTag*, TESObjectARMO * pArmor, UInt32 slotMask);

	bool RemoveSlotFromMask(StaticFunctionTag*, TESObjectARMO * pArmor, UInt32 slotMask);

	bool HasSlot(StaticFunctionTag*, TESObjectARMO * pArmor, UInt32 slotMask);

	bool RegisterFuncs(VMClassRegistry* registry);
}
