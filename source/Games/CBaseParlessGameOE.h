#pragma once
#include "CBaseParlessGame.h"

class CBaseParlessGameOE : public CBaseParlessGame
{
public:
	static const char* (*orgOEGetBGMName)(int soundID);
	static const char* (*orgOEGetFileFullPath)(char* buf, char* directory, char* fileName);
	static __int64 (*orgOELoadBGM)(__int64 thisObj, int* unknown, int id, float unknown2);
	static __int64 (*orgOELoadStream)(__int64 thisObj, char* fullPath);
	std::string translate_path_original(std::string path, int indexOfData);

	static __int64 LoadBGM(__int64 thisObj, int* unknown, int id, float unknown2);
	static __int64 LoadStreamFile(__int64 thisObj, char* fullPath);

	static void* cpkRedirectPatchLocation;
};