#include "ParlessGameYK3.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>
#include "Utils/buffer.h"
#include <string>

using namespace hook;
using namespace Memory;

ParlessGameYK3::t_orgYK3AddFileEntry ParlessGameYK3::orgYK3AddFileEntry = NULL;
ParlessGameYK3::t_orgYK3AddFileEntry(*ParlessGameYK3::hookYK3AddFileEntry) = NULL;

ParlessGameYK3::t_orgYK3GetEntityPath ParlessGameYK3::orgYK3GetEntityPath = NULL;
ParlessGameYK3::t_orgYK3GetEntityPath(*ParlessGameYK3::hookYK3GetEntityPath) = NULL;

bool ParlessGameYK3::hook_add_file()
{
	hookYK3AddFileEntry = (t_orgYK3AddFileEntry*)pattern("4C 89 4C 24 ? 89 54 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 8B 04 24 44 8B E2 C5 F8 29 B5 ? ? ? ? C5 F8 29 BD ? ? ? ? C5 78 29 85 ? ? ? ? C5 78 29 8D ? ? ? ? C5 78 29 95 ? ? ? ? C5 78 29 9D ? ? ? ? C5 78 29 A5 ? ? ? ? C5 78 29 AD ? ? ? ? C5 78 29 B5 ? ? ? ? C5 78 29 BD ? ? ? ? 4C 8B F1 33 DB 33 C0").get_first(0);

	if (MH_CreateHook(hookYK3AddFileEntry, &YK3AddFileEntry, reinterpret_cast<LPVOID*>(&orgYK3AddFileEntry)) != MH_OK)
	{
		cout << "Filepath hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookYK3AddFileEntry) != MH_OK)
	{
		cout << "Filepath hook could not be enabled. Aborting.\n";
		return false;
	}
	else
	{
		cout << "Filepath hook enabled, " << std::hex << hookYK3AddFileEntry << std::endl;
	}

	hookYK3GetEntityPath = (t_orgYK3GetEntityPath*)pattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B7 F2 48 8B F9 45 85 C0").get_first(0);

	if (hookYK3GetEntityPath != nullptr)
	{
		if (MH_CreateHook(hookYK3GetEntityPath, &YK3GetEntityPath, reinterpret_cast<LPVOID*>(&orgYK3GetEntityPath)) != MH_OK)
		{
			cout << "Entity hook creation failed. Aborting.\n";
			return false;
		}

		if (MH_EnableHook(hookYK3GetEntityPath) != MH_OK)
		{
			cout << "Entity hook could not be enabled. Aborting.\n";
			return false;
		}
	}
	

	hook_BindCpk = (t_CriBind*)pattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 40 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 4C 8B 65").get_first(0);
	org_BindDir = (t_CriBind)pattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 54 41 56 41 57 48 83 EC ? 48 8B B4 24").get_first(0);


	if (MH_CreateHook(hook_BindCpk, &BindCpk, reinterpret_cast<LPVOID*>(&org_BindCpk)) != MH_OK)
	{
		std::cout << "BindCPK Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hook_BindCpk) != MH_OK)
	{
		std::cout << "BindCPK Hook could not be enabled. Aborting.\n";
		return false;
	}
	

	if (redirectUbik)
	{
		//UBIK cache is not used in gaiden. we can get away with this insanity
		const char* szUbik = (const char*)pattern("64 61 74 61 2F 63 68 61 72 61 2F 75 62 69 6B 2F 00 00 00 00 00").get_first();

		if (szUbik != nullptr)
		{
			int len = strlen(modded_ubik_path) + 1; //null terminator

			VirtualProtect((void*)szUbik, len, PAGE_EXECUTE_READWRITE, nullptr);
			memcpy_s((void*)szUbik, len, (void*)modded_ubik_path, len);
		}
	}
	

		//*((char*)orgPYIHAddFileEntry) = 0x48;
	

	return true;
}