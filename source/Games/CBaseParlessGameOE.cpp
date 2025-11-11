#include "CBaseParlessGameOE.h"
#include "StringHelpers.h"
#include "Utils/MemoryMgr.h"
#include <iostream>
#include <filesystem>

const char* (*CBaseParlessGameOE::orgOEGetBGMName)(int id) = nullptr;
const char* (*CBaseParlessGameOE::orgOEGetFileFullPath)(char* buf, char* directory, char* fileName) = nullptr;
__int64 (*CBaseParlessGameOE::orgOELoadBGM)(__int64 thisObj, int* unknown, int id, float unknown2);
__int64 (*CBaseParlessGameOE::orgOELoadStream)(__int64 thisObj, char* fullPath);

CBaseParlessGameOE::t_OEAddUSMFile CBaseParlessGameOE::hook_OEAddUSMFile = NULL;
CBaseParlessGameOE::t_OEAddUSMFile CBaseParlessGameOE::org_OEAddUSMFile = NULL;

void* CBaseParlessGameOE::cpkRedirectPatchLocation = 0;


__int64 (*CBaseParlessGameOE::orgOEMotionArchiveEntry)(_int64 a1, char* filePath, int a3, int a4, int a5, char a6) = NULL;


std::string CBaseParlessGameOE::translate_path_original(std::string path, int indexOfData)
{
	std::vector<int> parts = splitPath(path, indexOfData, 3); //TODO!!!

	string sub;
	parless_stringmap::const_iterator match;

	const int START = 1; // Start index for checking paths
	const int MAX_SPLITS = (START + 1) + 2; // Max splits is 2

	// Look for matches in the map
	for (int i = START + 1; i < MAX_SPLITS && i < parts.size(); i++)
	{
		sub = path.substr(parts[START], parts[i] - parts[START]);
		match = m_gameMap.find(sub);

		if (match != m_gameMap.end())
		{
			// translate path
			return path.replace(parts[START], parts[i] - parts[START], match->second);
		}
	}

	return path;
}

//CPK redirecting: If a loose file exists, force the function to load the file loose (old leftover behavior)
//Otherwise its gonna load it from the CPK
__int64 CBaseParlessGameOE::LoadBGM(__int64 thisObj, int* unknown, int id, float unknown2)
{
	//Is there a better way to do all this?
	const char* name = orgOEGetBGMName(id);
	char buffer[272];

	orgOEGetFileFullPath(buffer, (char*)"data/stream/%s", (char*)name);
	RenameFilePaths(buffer);

	byte patch;

	//Loose file exists, dont load from cpk
	//We are patching a jne instruction to always jump
	//Which means BGM check will be skipped. And file will be loaded loosely
	//If it does not exist, we restore the original jne, checking BGM (original behavior)
	if (!std::filesystem::exists(buffer))
		patch = (byte)0x75;
	else
		patch = (byte)0xEB;

	DWORD oldProtect;
	VirtualProtect(cpkRedirectPatchLocation, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(cpkRedirectPatchLocation, &patch, 1);
	VirtualProtect(cpkRedirectPatchLocation, 1, oldProtect, &oldProtect);

	return orgOELoadBGM(thisObj, unknown, id, unknown2);
}


__int64 CBaseParlessGameOE::LoadStreamFile(__int64 thisObj, char* fullPath)
{
	RenameFilePaths(fullPath);
	return orgOELoadStream(thisObj, fullPath);
}

void* CBaseParlessGameOE::AddUSMFile(__int64 a1, char* filePath, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11)
{
	RenameFilePaths(filePath);
	return org_OEAddUSMFile(a1, filePath, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}