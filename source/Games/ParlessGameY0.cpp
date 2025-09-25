#include "ParlessGameY0.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

__int64 (*ParlessGameY0::orgY0AddFileEntry)(__int64 a1, char* filepath, __int64 a3, int a4, __int64 a5, __int64 a6, char a7, __int64 a8, char a9, char a10, char a11, char a12, char a13) = NULL;
__int64 (*ParlessGameY0::orgY0CpkEntry)(__int64 a1, __int64 a2, __int64 a3, __int64 a4) = NULL;

std::string ParlessGameY0::get_name()
{
	return "Yakuza 0";
}

bool ParlessGameY0::hook_add_file()
{
	void* renameFilePathsFunc;

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	//CPK redirecting: If a loose file exists, force the function to load the file loose (old leftover behavior)
	//Otherwise its gonna load it from the CPK
	Memory::ReadCall(get_pattern("E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 4C 24 20 4C 8B C0 E8 ? ? ? ? 48 8B 4F 18"), orgOEGetBGMName);
	Memory::ReadCall(get_pattern("E8 ? ? ? ? 48 8B 4F 18 48 8D 54 24 20"), orgOEGetFileFullPath);
	
	void* loadBgmFunc = get_pattern("48 89 6C 24 10 56 57 41 56 48 81 EC ? ? ? ? 44 89 41 24");
	void* loadStreamFunc = get_pattern("40 53 48 83 EC ? 48 8B D9 48 8B 49 10 4C 8B C2"); //get_pattern("40 53 48 83 EC ? 48 8B D9 48 8B 49 10 4C 8B C2");

	hook_BindCpk = (t_CriBind*)get_pattern("41 57 48 83 EC ? 4C 8B A4 24 B8 00 00 00", -22);

	cpkRedirectPatchLocation = get_pattern("75 ? 49 83 F8 ? 75 ? 48 8B 47 18 48 89 9C 24 60 01 00 00");

	if (MH_CreateHook(loadBgmFunc, &CBaseParlessGameOE::LoadBGM, reinterpret_cast<LPVOID*>(&orgOELoadBGM)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(loadBgmFunc) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	if (MH_CreateHook(loadStreamFunc, &CBaseParlessGameOE::LoadStreamFile, reinterpret_cast<LPVOID*>(&orgOELoadStream)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(loadStreamFunc) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	// might want to hook the other call of this function as well, but currently not necessary (?)
	renameFilePathsFunc = get_pattern("48 89 44 24 20 48 8B D5 48 8B 0D ? ? ? ?", 15);
	ReadCall(renameFilePathsFunc, orgY0AddFileEntry);

	// this will take care of every file that is read from disk
	InjectHook(renameFilePathsFunc, trampoline->Jump(Y0AddFileEntry));

	renameFilePathsFunc = get_pattern("8B 4D D7 4A 8D 74 28 20 48 83 E6 E0 48 8D BE 9F 02 00 00", -13);
	ReadCall(renameFilePathsFunc, orgY0CpkEntry);

	InjectHook(renameFilePathsFunc, trampoline->Jump(Y0CpkEntry));

	cout << "Applied CPK loading hook.\n";

	org_BindCpk = (t_CriBind)((char*)org_BindCpk + 1);


	return true;
}