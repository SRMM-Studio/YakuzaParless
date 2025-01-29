#include "ParlessGameVF5REVO.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>
#include <thread>
#include "StringHelpers.h"

using namespace hook;
using namespace Memory;

ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry ParlessGameVF5REVO::orgVF5REVOAddFileEntry = NULL;
ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry* ParlessGameVF5REVO::hookVF5REVOAddFileEntry = NULL;

bool ParlessGameVF5REVO::hook_add_file()
{
	void* renameFilePathsFunc;
	void* musicPlayFunc;

	renameFilePathsFunc = get_pattern("48 8B C4 4C 89 48 20 89 50 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 68 FE FF FF", 0);

	if (MH_CreateHook(renameFilePathsFunc, &VF5REVOAddFileEntry, reinterpret_cast<LPVOID*>(&orgVF5REVOAddFileEntry)) != MH_OK)
	{
		cout << "Hook creation failed. Aborting.\n";
		return false;
	}

	if (MH_EnableHook(renameFilePathsFunc) != MH_OK)
	{
		cout << "Hook could not be enabled. Aborting.\n";
		return false;
	}
	return true;
}