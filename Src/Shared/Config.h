#pragma once




DEFINE_LOGGED_EXCEPTION(CConfigException);


class CConfig
{
public:

	CConfig(const std::string& filename);
	~CConfig() = default;


#define MACRO_CONFIG_INT( name, def, min, max, desc ) s32 name;
#define MACRO_CONFIG_FLOAT( name, def, min, max, desc ) f32 name;
#define MACRO_CONFIG_STR( name, def, maxlen, desc ) char name[maxlen+1]; // +1 for zero termination!


#include "configVariables.h"




	void ReadFromFile(const char* filename);
	void WriteToFile(const char* filename);



private:


	enum E_READING_STATE
	{
		TOKEN_NAME = 0,
		FIND_SEPERATOR,
		TOKEN_VALUE
	};

};
