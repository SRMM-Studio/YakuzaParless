#include "ParlessGameYK1R.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

__int64 (*ParlessGameYK1R::orgY0AddFileEntry)(__int64 a1, char* filepath, __int64 a3, int a4, __int64 a5, __int64 a6, char a7, __int64 a8, char a9, char a10, char a11, char a12, char a13) = NULL;
__int64 (*ParlessGameYK1R::orgY0CpkEntry)(__int64 a1, __int64 a2, __int64 a3, __int64 a4) = NULL;

ParlessGameYK1R::t_YK1RAddUSMFile ParlessGameYK1R::hook_YK1RAddUSMFile = NULL;
ParlessGameYK1R::t_YK1RAddUSMFile ParlessGameYK1R::org_YK1RAddUSMFile = NULL;

std::string ParlessGameYK1R::get_name()
{
	return "Yakuza Kiwami: Remastered";
}

bool ParlessGameYK1R::hook_add_file()
{
	void* renameFilePathsFunc;

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	hook_YK1RAddUSMFile = (t_YK1RAddUSMFile)ReadCallFrom(get_pattern("E8 ? ? ? ? C7 83 ? ? ? ? ? ? ? ? 0F B6 84 24"));
	//hook_BindCpk = (t_CriBind*)get_pattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 40 ? 55 41 54 41 55 41 56 41 57 48 8B EC");

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

	if (MH_CreateHook(hook_YK1RAddUSMFile, &CBaseParlessGameOE::AddUSMFile, reinterpret_cast<LPVOID*>(&org_OEAddUSMFile)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hook_YK1RAddUSMFile) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	//Force the game to load from stream folders and not the pars
	BYTE patch[]{ 0xEB };
	Memory::Patch2((BYTE*)get_pattern("72 ? 49 8B 07 49 8B CF 48 89 9C 24"), patch, 1);

	// might want to hook the other call of this function as well, but currently not necessary (?)
	renameFilePathsFunc = get_pattern("E8 ? ? ? ? 66 89 83 ? ? ? ? 48 8D 4C 24");
	ReadCall(renameFilePathsFunc, orgY0AddFileEntry);

	// this will take care of every file that is read from disk
	InjectHook(renameFilePathsFunc, trampoline->Jump(YK1RAddFileEntry));

	auto cpkFunc = get_pattern("E8 ? ? ? ? 48 8B CE E8 ? ? ? ? 8B 4D");
	ReadCall(cpkFunc, orgY0CpkEntry);
	InjectHook(cpkFunc, trampoline->Jump(Y0CpkEntry));

	cout << "Applied CPK loading hook.\n";

	org_BindCpk = (t_CriBind)((char*)org_BindCpk + 1);

	void* renameMotionArchiveFunc = get_pattern("E8 ? ? ? ? 48 8B D8 EB ? 48 8B DF 44 8B C7");

	/*
	if (MH_CreateHook(renameMotionArchiveFunc, &CBaseParlessGameOE::OEMotionArchiveEntry, reinterpret_cast<LPVOID*>(&orgOEMotionArchiveEntry)) != MH_OK)
		return false;

	if (MH_EnableHook(renameMotionArchiveFunc) != MH_OK)
		return false;
		*/

	return true;
}