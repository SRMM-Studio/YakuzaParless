#include "ParlessGameOOE.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

__int64 (*ParlessGameOOE::orgOOEAddFileEntry)(__int64 a1, __int64 filepath, __int64 a3, int a4, __int64 a5, __int64 a6, int a7, __int64 a8, int a9, char a10, int a11, char a12) = NULL;
__int64 (*ParlessGameOOE::orgOOEAdxEntry)(__int64 a1, __int64 a2, __int64 a3) = NULL;
__int64 (*ParlessGameOOE::orgOOEUSMEntry)(__int64 a1, const char* filepath, __int64 a3) = NULL;

bool ParlessGameOOE::hook_add_file()
{
	void* renameFilePathsFunc;
	void* renameUSMFilePathsFunc;

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	renameFilePathsFunc = get_pattern("66 89 83 44 02 00 00 E8 ? ? ? ? 48 8B C3 48 8B 8C 24 A0 03 00 00", -5);

	if (!isXbox)
		renameUSMFilePathsFunc = Memory::ReadCallFrom(get_pattern("E8 ? ? ? ? EB ? 49 8B C6 48 89 45 40"));
	else
		renameUSMFilePathsFunc = Memory::ReadCallFrom(get_pattern("E8 ? ? ? ? 48 8B C8 EB ? 33 C9 48 89 8F B0 01 00 00"));

	ReadCall(renameFilePathsFunc, orgOOEAddFileEntry);

	InjectHook(renameFilePathsFunc, trampoline->Jump(OOEAddFileEntry));

	// Adx
	renameFilePathsFunc = get_pattern("48 89 5C 24 18 57 48 81 EC 90 06 00 00 48 8B 05", 0x84);
	ReadCall(renameFilePathsFunc, orgOOEAdxEntry);

	InjectHook(renameFilePathsFunc, trampoline->Jump(OOEAdxEntry));
	std::cout << "Applied ADX loading hook.\n";

	// Usm

	if (MH_CreateHook(renameUSMFilePathsFunc, &ParlessGameOOE::OOEUSMEntry, reinterpret_cast<LPVOID*>(&orgOOEUSMEntry)) != MH_OK)
		return false;

	if (MH_EnableHook(renameUSMFilePathsFunc) != MH_OK)
		return false;

	return true;
};