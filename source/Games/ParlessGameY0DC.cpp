#include "ParlessGameY0DC.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

__int64 (*ParlessGameY0DC::orgY0AddFileEntry)(__int64 a1, char* filepath, __int64 a3, int a4, __int64 a5, __int64 a6, char a7, __int64 a8, char a9, char a10, char a11, char a12, char a13) = NULL;
__int64 (*ParlessGameY0DC::orgY0CpkEntry)(__int64 a1, __int64 a2, __int64 a3, __int64 a4) = NULL;

std::string ParlessGameY0DC::get_name()
{
	return "Yakuza 0: Director's Cut";
}

bool ParlessGameY0DC::hook_add_file()
{
	void* renameFilePathsFunc;

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	

	hook_OEAddUSMFile = (t_OEAddUSMFile)ReadCallFrom(get_pattern("E8 ? ? ? ? 48 8B 9C 24 ? ? ? ? 89 AB"));
	//hook_BindCpk = (t_CriBind*)get_pattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 40 ? 55 41 54 41 55 41 56 41 57 48 8B EC");


	void* loadBgmFunc = get_pattern("48 89 5C 24 ? 55 56 41 57 48 81 EC");
	void* loadStreamFunc = get_pattern("40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 49"); 


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

	if (MH_CreateHook(hook_OEAddUSMFile, &CBaseParlessGameOE::AddUSMFile, reinterpret_cast<LPVOID*>(&org_OEAddUSMFile)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hook_OEAddUSMFile) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	BYTE patch[]{ 0xEB };

	//Force the game to load from stream folders and not the pars
	Memory::Patch2((BYTE*)get_pattern("72 ? 48 8B 7E"), patch, 1);
	Memory::Patch2((BYTE*)get_pattern("73 ? 8B CD E8"), patch, 1);
	Memory::Patch2((BYTE*)get_pattern("0F 83 ? ? ? ? 8B CD E8"), patch, 1);


	// might want to hook the other call of this function as well, but currently not necessary (?)
	renameFilePathsFunc = get_pattern("E8 ? ? ? ? 66 89 83 ? ? ? ? 4D 3B FE");
	ReadCall(renameFilePathsFunc, orgY0AddFileEntry);

	// this will take care of every file that is read from disk
	InjectHook(renameFilePathsFunc, trampoline->Jump(Y0AddFileEntry));

	auto cpkFunc = get_pattern("E8 ? ? ? ? 49 8B CE E8 ? ? ? ? 8B 4D");
	ReadCall(cpkFunc, orgY0CpkEntry);
	InjectHook(cpkFunc, trampoline->Jump(Y0CpkEntry));

	cout << "Applied CPK loading hook.\n";

	org_BindCpk = (t_CriBind)((char*)org_BindCpk + 1);

	//TODO URGENT
	
	void* renameMotionArchiveFunc = get_pattern("E8 ? ? ? ? 48 8B D0 EB ? 48 8B D7 8B CF");

	if (MH_CreateHook(renameMotionArchiveFunc, &CBaseParlessGameOE::OEMotionArchiveEntry, reinterpret_cast<LPVOID*>(&orgOEMotionArchiveEntry)) != MH_OK)
		return false;

	if (MH_EnableHook(renameMotionArchiveFunc) != MH_OK)
		return false;

	return true;
}