#include "CBaseParlessGameDE.h"

class ParlessGameYK2R : public CBaseParlessGameDE
{
	static int (*orgYK2AddFileEntry)(short* a1, int a2, char* a3, char** a4);
	static uint64_t(*orgYK2SprintfAwb)(uint64_t param_1, uint64_t param_2, uint64_t param_3, uint64_t param_4);

	virtual std::string get_name() override
	{
		return "Yakuza Kiwami 2 Remastered";
	};
	
	virtual parless_stringmap get_game_map(Locale locale) override
	{
		return parless_stringmap();
	};

	virtual std::string translate_path(std::string path, int indexOfData) override;
	virtual bool hook_add_file() override;

	static int YK2AddFileEntry(short* a1, int a2, char* a3, char** a4)
	{
		orgYK2AddFileEntry(a1, a2, a3, a4);
		RenameFilePaths((char*)a1);
		return strlen((char*)a1);
	}

	static uint64_t YK2SprintfAwb(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4)
	{
		uint64_t result = orgYK2SprintfAwb(a1, a2, a3, a4);
		RenameFilePaths((char*)result);
		return result;
	}
};