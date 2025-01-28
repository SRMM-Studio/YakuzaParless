#include "ParlessGameVF5REVO.h"
#include "Utils/MemoryMgr.h"
#include "Utils/Trampoline.h"
#include "Utils/Patterns.h"
#include <MinHook.h>

using namespace hook;
using namespace Memory;

ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry ParlessGameVF5REVO::orgVF5REVOAddFileEntry = NULL;
ParlessGameVF5REVO::t_orgVF5REVOAddFileEntry* ParlessGameVF5REVO::hookVF5REVOAddFileEntry = NULL;

ParlessGameVF5REVO::t_orgVF5REVOFilepath ParlessGameVF5REVO::orgVF5REVOFilepath = NULL;
ParlessGameVF5REVO::t_orgVF5REVOFilepath* ParlessGameVF5REVO::hookVF5REVOFilepath = NULL;

bool ParlessGameVF5REVO::hook_add_file()
{
	void* renameFilePathsFunc;

	renameFilePathsFunc = get_pattern("40 BA 10 04 00 00 E8", 6);

	Trampoline* trampoline = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));

	ReadCall(renameFilePathsFunc, orgVF5REVOAddFileEntry);
	InjectHook(renameFilePathsFunc, trampoline->Jump(VF5REVOAddFileEntry));

	return true;
}