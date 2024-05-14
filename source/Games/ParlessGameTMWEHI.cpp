#include "ParlessGameTMWEHI.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <iostream>

using namespace hook;
using namespace Memory;

ParlessGameTMWEHI::t_orgGaidenAddFileEntry ParlessGameTMWEHI::orgGaidenAddFileEntry = NULL;
ParlessGameTMWEHI::t_orgGaidenAddFileEntry(*ParlessGameTMWEHI::hookGaidenAddFileEntry) = NULL;

ParlessGameTMWEHI::t_orgGDGetEntityPath ParlessGameTMWEHI::orgGDGetEntityPath = NULL;
ParlessGameTMWEHI::t_orgGDGetEntityPath(*ParlessGameTMWEHI::hookGDGetEntityPath) = NULL;

bool ParlessGameTMWEHI::hook_add_file()
{
	if (!isXbox)
		hookGaidenAddFileEntry = (t_orgGaidenAddFileEntry*)pattern("48 8B C4 4C 89 48 20 89 50 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 68 FE FF FF").get_first(0);
	else
		hookGaidenAddFileEntry = (t_orgGaidenAddFileEntry*)pattern("48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FE FF FF").get_first(0);

	if (MH_CreateHook(hookGaidenAddFileEntry, &GaidenAddFileEntry, reinterpret_cast<LPVOID*>(&orgGaidenAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookGaidenAddFileEntry) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	hookGDGetEntityPath = (t_orgGDGetEntityPath*)pattern("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC ? 44 89 C7 48 89 CB").get_first(0);

	if (MH_CreateHook(hookGDGetEntityPath, &GaidenGetEntityPath, reinterpret_cast<LPVOID*>(&orgGDGetEntityPath)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(hookGDGetEntityPath) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}

	*((char*)orgGaidenAddFileEntry) = 0x48;
}