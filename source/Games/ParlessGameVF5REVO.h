#include "CBaseParlessGameDE.h"
#include <iostream>

class ParlessGameVF5REVO : public CBaseParlessGameDE
{
	virtual std::string get_name() override
	{
		return "Virtua Fighter 5 R.E.V.O.";
	};

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
	typedef char* (*t_orgVF5REVOAddFileEntry)(char* a1, uint64_t a2, char* a3, char* a4);
	static t_orgVF5REVOAddFileEntry orgVF5REVOAddFileEntry;
	static t_orgVF5REVOAddFileEntry(*hookVF5REVOAddFileEntry);

	typedef void (*t_orgVF5REVOFilepath)(char* a1, uint64_t a2, char* a3, uint64_t a4);
	static t_orgVF5REVOFilepath orgVF5REVOFilepath;
	static t_orgVF5REVOFilepath(*hookVF5REVOFilepath);

	static char* VF5REVOAddFileEntry(char* a1, uint64_t a2, char* a3, char* a4)
	{
		char* result = orgVF5REVOAddFileEntry(a1, a2, a3, a4);
		RenameFilePaths(a1);
		return result;
	}

	static void VF5REVOFilepath(char* a1, uint64_t a2, char* a3, uint64_t a4)
	{
		RenameFilePaths(a3);
		orgVF5REVOFilepath(a1, a2, a3, a4);
	}
};