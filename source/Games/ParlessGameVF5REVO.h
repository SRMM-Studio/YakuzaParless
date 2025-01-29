#include "CBaseParlessGameDE.h"
#include <algorithm>
#include <string>
#include <filesystem>

class ParlessGameVF5REVO : public CBaseParlessGameDE
{
	virtual std::string get_name() override
	{
		return "Virtua Fighter 5 R.E.V.O.";
	};

	virtual bool enable_vf5fs_support(bool arcadeSupportEnabled) override
	{
		//bro. we are vf5fs
		return true;
	}

	virtual parless_stringmap get_game_map(Locale locale) override
	{
		std::string curLoc;
		std::vector<const char*> locY7Vec{ "de", "en", "es", "fr", "it", "ja", "ko", "pt", "ru", "zh", "zhs", "pt"};

		parless_stringmap result = parless_stringmap();
		result["/entity"] = "/entity_adam";

		for (int i = 0; i < locY7Vec.size(); i++)
		{
			curLoc = std::string(locY7Vec[i]);
			result["/db.adam/" + curLoc] = "/db.adam." + curLoc + "/" + curLoc;
			result["/ui.adam/" + curLoc] = "/ui.adam." + curLoc + "/" + curLoc;
		}

		return result;
	}

	virtual bool hook_add_file() override;

private:
	typedef char* (*t_orgVF5REVOAddFileEntry)(short* a1, int a2, char* a3, char** a4);
	static t_orgVF5REVOAddFileEntry orgVF5REVOAddFileEntry;
	static t_orgVF5REVOAddFileEntry(*hookVF5REVOAddFileEntry);

	static int VF5REVOAddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgVF5REVOAddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}
};