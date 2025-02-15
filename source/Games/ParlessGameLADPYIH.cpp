#include "ParlessGameLADPYIH.h"
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

ParlessGameLADPYIH::t_orgPYIHAddFileEntry ParlessGameLADPYIH::orgPYIHAddFileEntry = NULL;
ParlessGameLADPYIH::t_orgPYIHAddFileEntry(*ParlessGameLADPYIH::hookPYIHAddFileEntry) = NULL;

ParlessGameLADPYIH::t_orgPYIHGetEntityPath ParlessGameLADPYIH::orgPYIHGetEntityPath = NULL;
ParlessGameLADPYIH::t_orgPYIHGetEntityPath(*ParlessGameLADPYIH::hookPYIHGetEntityPath) = NULL;

bool ParlessGameLADPYIH::hook_add_file()
{
	hookPYIHAddFileEntry = (t_orgPYIHAddFileEntry*)(t_orgPYIHAddFileEntry*)pattern("48 8B C4 4C 89 48 20 89 50 10 55").get_first(0);

	if (MH_CreateHook(hookPYIHAddFileEntry, &PYIHAddFileEntry, reinterpret_cast<LPVOID*>(&orgPYIHAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookPYIHAddFileEntry) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	hookPYIHGetEntityPath = (t_orgPYIHGetEntityPath*)pattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 0F B7 F2 48 8B F9 45 85 C0").get_first(0);

	if (hookPYIHGetEntityPath != nullptr)
	{
		if (MH_CreateHook(hookPYIHGetEntityPath, &PYIHGetEntityPath, reinterpret_cast<LPVOID*>(&orgPYIHGetEntityPath)) != MH_OK)
		{
			cout << "Hook creation failed. Aborting.\n";
			return false;
		}

		if (MH_EnableHook(hookPYIHGetEntityPath) != MH_OK)
		{
			cout << "Hook could not be enabled. Aborting.\n";
			return false;
		}
	}

	hook_BindCpk = (t_CriBind*)pattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 20 4C 89 40 18 55 41 54 41 55 41 56 41 57 48 8B EC").get_first(0);
	org_BindDir = (t_CriBind)pattern("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC ? 48 8B B4 24 88 00 00 00").get_first(0);

	if (MH_CreateHook(hook_BindCpk, &BindCpk, reinterpret_cast<LPVOID*>(&org_BindCpk)) != MH_OK)
	{
		std::cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hook_BindCpk) != MH_OK)
	{
		std::cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	if (redirectUbik)
	{
		//UBIK cache is not used in gaiden. we can get away with this insanity
		const char* szUbik = (const char*)pattern("64 61 74 61 2F 63 68 61 72 61 2F 75 62 69 6B 2F 00 00 00 00 00").get_first();
		int len = strlen(modded_ubik_path) + 1; //null terminator

		VirtualProtect((void*)szUbik, len, PAGE_EXECUTE_READWRITE, nullptr);
		memcpy_s((void*)szUbik, len, (void*)modded_ubik_path, len);
	}

	*((char*)orgPYIHAddFileEntry) = 0x48;
}