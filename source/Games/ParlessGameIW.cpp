#include "ParlessGameIW.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

ParlessGameIW::t_orgIWAddFileEntry ParlessGameIW::orgIWAddFileEntry = NULL;
ParlessGameIW::t_orgIWAddFileEntry(*ParlessGameIW::hookIWAddFileEntry) = NULL;

ParlessGameIW::t_orgIWGetEntityPath ParlessGameIW::orgIWGetEntityPath = NULL;
ParlessGameIW::t_orgIWGetEntityPath(*ParlessGameIW::hookIWGetEntityPath) = NULL;

bool ParlessGameIW::hook_add_file()
{
	if (!isXbox)
		hookIWAddFileEntry = (t_orgIWAddFileEntry*)pattern("48 8B C4 4C 89 48 20 89 50 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 68 FE FF FF").get_first(0);
	else
		hookIWAddFileEntry = (t_orgIWAddFileEntry*)pattern("48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FE FF FF").get_first(0);

	if(!isXbox)
		hookIWGetEntityPath = (t_orgIWGetEntityPath*)pattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 0F B7 F2 48 8B F9").get_first(0);

	if (MH_CreateHook(hookIWAddFileEntry, &IWAddFileEntry, reinterpret_cast<LPVOID*>(&orgIWAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookIWAddFileEntry) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	if (hookIWGetEntityPath != nullptr)
	{
		if (MH_CreateHook(hookIWGetEntityPath, &IWGetEntityPath, reinterpret_cast<LPVOID*>(&orgIWGetEntityPath)) != MH_OK)
		{
			cout << "Hook creation failed. Aborting.\n";
			return false;
		}

		if (MH_EnableHook(hookIWGetEntityPath) != MH_OK)
		{
			cout << "Hook could not be enabled. Aborting.\n";
			return false;
		}
	}

	*((char*)orgIWAddFileEntry) = 0x48;
}