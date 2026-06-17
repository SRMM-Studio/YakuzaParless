#include "CBaseParlessGameOE.h"

class ParlessGameYK1 : public CBaseParlessGameOE
{
	static __int64 (*orgYK1AddFileEntry)(__int64 a1, char* filepath, __int64 a3, int a4, __int64 a5, __int64 a6, char a7, __int64 a8, char a9, char a10, char a11, char a12, char a13);
	static __int64 (*orgYK1CpkEntry)(__int64 a1, __int64 a2, __int64 a3, __int64 a4);
public:
	std::string get_name() override;

	bool hook_add_file() override;

	virtual parless_stringmap get_game_map(Locale locale) override
	{
		std::vector<const char*> locVec{ "c", "j"};

		std::string curLoc;

		auto map = parless_stringmap({
				{"/font" , "/fontpar/font"},
				{"/2d/ui_common", "/2dpar/ui_common"},
				{"/boot" , "/bootpar/boot"},
				{"/stay" , "/staypar/stay"},
				{"/sound" , "/soundpar/sound"},
				{"/battle" , "/battlepar/battle"},
				{"/reactor_w64" , "/reactorpar/reactor_w64"},
				{"/light_anim" , "/light_anim/light_anim"},
			});

		for (int i = 0; i < locVec.size(); i++)
		{
			curLoc = std::string(locVec[i]);
			map["/2d/ui_" + curLoc] = "/2dpar/ui_" + curLoc;
		}

		for (int i = 0; i < locVec.size(); i++)
		{
			curLoc = std::string(locVec[i]);
			map["/wdr_" + curLoc + "/common"] = "/wdr_par_" + curLoc + "/common";
			map["/wdr_" + curLoc] = "/wdr_par_" + curLoc + "/wdr";
			map["/2d/sprite_" + curLoc] = "/2dpar/sprite_" + curLoc;
		}

		return map;
	};


	static __int64 YK1AddFileEntry(__int64 a1, char* filepath, __int64 a3, int a4, __int64 a5, __int64 a6, char a7, __int64 a8, char a9, char a10, char a11, char a12, char a13)
	{
		RenameFilePaths(filepath);
		return orgYK1AddFileEntry(a1, filepath, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	}

	static __int64 YK1CpkEntry(__int64 a1, __int64 filepath, __int64 a3, __int64 a4)
	{
		RenameFilePaths((char*)filepath);
		return orgYK1CpkEntry(a1, filepath, a3, a4);
	}
};