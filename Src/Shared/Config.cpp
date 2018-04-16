#include <Shared.h>

#include "Config.h"

CConfig::CConfig(const std::string& filename)
{
#define MACRO_CONFIG_INT( name, def, min, max, desc ) name = def;
#define MACRO_CONFIG_FLOAT( name, def, min, max, desc ) name = def;
#define MACRO_CONFIG_STR( name, def, maxlen, desc ) strcpy(name, def);

#include "configVariables.h"

	ReadFromFile(filename.c_str());
}


s32 config_switch(const char* str)
{
	if(strncmp(str, "true", 4) == 0 || strncmp(str, "on", 2) == 0 || strncmp(str, "yes", 3) == 0 || strncmp(str, "oui", 3) == 0 || strncmp(str, "ja", 2) == 0 || strncmp(str, "si", 2) == 0)
		return 1;
	if(strncmp(str, "false", 5) == 0 || strncmp(str, "no", 2) == 0 || strncmp(str, "non", 3) == 0 || strncmp(str, "nein", 4) == 0)
		return 0;

	return (s32)strtol(str, nullptr, 0);
}

void CConfig::ReadFromFile(const char* filename)
{
	// Has to use non-filesystem based opening funcs.
	FILE* pFile = fopen(filename, "rb");

	if(pFile == NULL)
	{
		throw CConfigException(SRC_POS, StrFormat("Config file '{0}' could not be opened.", filename));
	}

	// Find file size
	fseek(pFile, 0, SEEK_END);
	u32 Size = ftell(pFile);
	rewind(pFile);


	char* buffer = new char[Size];

	size_t NumBytesRead = fread(buffer, sizeof(char), Size, pFile);
	if(NumBytesRead != Size)
	{
		throw CConfigException(SRC_POS, StrFormat("Config file '{0}' could not be fully read. (read {1} of {2} bytes)", filename, NumBytesRead, Size));
	}

	// No need for the file anymore
	fclose(pFile);

	// Now evaluate the contents
	u32 pos = 0;
	u32 i;
	E_READING_STATE readingState = TOKEN_NAME;
	char szCurrentToken[256] = "";
	char szCurrentValue[1024] = "";

	while(pos < Size)
	{
		// Skip spaces
		if(std::isspace(buffer[pos]))
		{
			pos++;
		}
		// // Comment
		else if(buffer[pos] == '/' && buffer[pos + 1] == '/')
		{
			// Skip until newline
			while(pos < Size && buffer[pos] != '\n')
			{
				pos++;
			}
		}
		// /* */ Comment
		else if(buffer[pos] == '/' && buffer[pos + 1] == '*')
		{
			// Skip until end of comment
			while(pos < Size && !(buffer[pos] == '*' && buffer[pos + 1] == '/'))
			{
				pos++;
			}

			// Skip the "*/"
			pos += 2;
		}
		// We found something
		else
		{
			switch(readingState)
			{
			case TOKEN_NAME:
				// Read token
				i = 0;
				while(pos < Size &&
					  !std::isspace(buffer[pos]) &&
					  buffer[pos] != ':')
				{
					szCurrentToken[i++] = buffer[pos++];
				}
				szCurrentToken[i] = '\0';

				// Check if we need the find seperator state
				if(buffer[pos] == ':')
				{
					pos++;
					readingState = TOKEN_VALUE;
				}
				else
				{
					readingState = FIND_SEPERATOR;
				}
				break;


			case FIND_SEPERATOR:
				if(buffer[pos] != ':')
				{
					Log(CLog::Warning, StrFormat("Config '{0}' is corrupted. (Wrong seperator.)", filename));
				}
				pos++;
				readingState = TOKEN_VALUE;
				break;


			case TOKEN_VALUE:
				// Read token
				i = 0;

				// Using " ?
				if(buffer[pos] == '"')
				{
					// Skip the first '"'
					pos++;

					// Read until end '"'
					while(pos < Size && buffer[pos] != '"')
					{
						szCurrentValue[i++] = buffer[pos++];
					}
					szCurrentValue[i] = '\0';

					// Skip the end '"'
					pos++;
				}
				else
				{
					// Read until newline / space or comment, dismiss all trailing spaces
					while(pos < Size &&
						  !(buffer[pos] == '/' && buffer[pos + 1] == '/') &&
						  buffer[pos] != '\n' &&
						  buffer[pos] != ' ' &&
						  buffer[pos] != '\t' &&
						  buffer[pos] != '\r')
					{
						szCurrentValue[i++] = buffer[pos++];
					}

					// Dismiss trailing ' '
					while(szCurrentValue[i - 1] == ' ')
					{
						i--;
					}
					szCurrentValue[i] = '\0';
				}

				// Update reading state
				readingState = TOKEN_NAME;

#define MACRO_CONFIG_INT( name, def, min, max, desc ) \
				if(!strcmp(#name, szCurrentToken)) { name = config_switch(szCurrentValue); break; }

#define MACRO_CONFIG_FLOAT( name, def, min, max, desc ) \
				if(!strcmp(#name, szCurrentToken)) { name = (float)atof(szCurrentValue); break; }

#define MACRO_CONFIG_STR( name, def, maxlen, desc ) \
				if(!strcmp(#name, szCurrentToken)) \
				{ if(i > maxlen) i = maxlen; \
				strncpy(name, szCurrentValue, i); name[i] = '\0'; break; }



#include "configVariables.h"



				break;
			}
		}
	}


	delete[] buffer;
}


void CConfig::WriteToFile(const char* Filename)
{
	// TODO: implement
}
