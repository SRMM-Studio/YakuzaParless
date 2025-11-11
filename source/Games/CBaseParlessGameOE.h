#pragma once
#include "CBaseParlessGame.h"

class CBaseParlessGameOE : public CBaseParlessGame
{
public:
	static const char* (*orgOEGetBGMName)(int soundID);
	static const char* (*orgOEGetFileFullPath)(char* buf, char* directory, char* fileName);
	static __int64 (*orgOELoadBGM)(__int64 thisObj, int* unknown, int id, float unknown2);
	static __int64 (*orgOELoadStream)(__int64 thisObj, char* fullPath);
	typedef void* (*t_OEAddUSMFile)(__int64 a1, char* filePath, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11);
	static __int64 (*orgOEMotionArchiveEntry)(__int64 a1, char* filePath, int a3, int a4, int a5, char a6);


	static t_OEAddUSMFile hook_OEAddUSMFile;
	static t_OEAddUSMFile org_OEAddUSMFile;

	std::string translate_path_original(std::string path, int indexOfData);

	static __int64 LoadBGM(__int64 thisObj, int* unknown, int id, float unknown2);
	static __int64 LoadStreamFile(__int64 thisObj, char* fullPath);
	static void* AddUSMFile(__int64 a1, char* filePath, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11);

	static void* cpkRedirectPatchLocation;

	static __int64 OEMotionArchiveEntry(__int64 a1, char* filePath, int a3, int a4, int a5, char a6)
	{
		char buf[272];
		strcpy_s(buf, 256, filePath);

		RenameFilePaths(buf);
		return orgOEMotionArchiveEntry(a1, buf, a3, a4, a5, a6);
	}
};