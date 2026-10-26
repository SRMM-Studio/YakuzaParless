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

	void* loadFxoShader = Memory::ReadCallFrom(get_pattern("E8 ? ? ? ? 4A 89 44 2B"));
	void* loadPsoShader = Memory::ReadCallFrom(get_pattern("E8 ? ? ? ? 41 89 84 9D ? ? ? ? 41 FF D7 85 C0 0F 88 ? ? ? ? 41 FF C4 41 81 FC ? ? ? ? 72 ? 41 FF D7"));
	void* loadVsoShader = Memory::ReadCallFrom(get_pattern("E8 ? ? ? ? 41 89 84 9D ? ? ? ? 41 FF D7 85 C0 0F 88 ? ? ? ? 41 FF C4 41 81 FC ? ? ? ? 72 ? 41 BC"));

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

	if (MH_CreateHook(loadFxoShader, &CBaseParlessGameOE::OEShaderEntryFXO, reinterpret_cast<LPVOID*>(&orgOEShaderEntryFXO)) != MH_OK)
	{
		cout << "FXO Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(loadFxoShader) != MH_OK)
	{
		cout << "FXO Hook could not be enabled. Aborting.\n";
		return false;
	}

	if (MH_CreateHook(loadPsoShader, &CBaseParlessGameOE::OEShaderEntryPSO, reinterpret_cast<LPVOID*>(&orgOEShaderEntryPSO)) != MH_OK)
	{
		cout << "PSO Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(loadPsoShader) != MH_OK)
	{
		cout << "PSO Hook could not be enabled. Aborting.\n";
		return false;
	}

	if (MH_CreateHook(loadVsoShader, &CBaseParlessGameOE::OEShaderEntryVSO, reinterpret_cast<LPVOID*>(&orgOEShaderEntryVSO)) != MH_OK)
	{
		cout << "VSO Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(loadVsoShader) != MH_OK)
	{
		cout << "VSO Hook could not be enabled. Aborting.\n";
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

	void* renameMotionArchiveFuncPtr = get_pattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 4C 24 ? 56 57 41 56 48 83 EC ? 41 8B F0");

	
	if (MH_CreateHook(renameMotionArchiveFuncPtr, &CBaseParlessGameOE::OEMotionArchiveEntry, reinterpret_cast<LPVOID*>(&orgOEMotionArchiveEntry)) != MH_OK)
		return false;

	if (MH_EnableHook(renameMotionArchiveFuncPtr) != MH_OK)
		return false;

	return true;
}