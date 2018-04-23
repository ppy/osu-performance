#pragma once

#define DEFINE_EXCEPTION(x) \
	class x : public CException \
	{ public: x(std::string File, s32 Line, std::string Description) \
	: CException{std::move(File), Line, std::move(Description)} {} }

#define DEFINE_LOGGED_EXCEPTION(x) \
	class x : public CLoggedException \
	{ public: x(std::string File, s32 Line, std::string Description) \
	: CLoggedException{std::move(File), Line, std::move(Description)} {} }

#define SRC_POS __FILE__,__LINE__

class CException
{
public:
	CException(std::string file, s32 line, std::string description);
	~CException() = default;

	void Print();

	std::string Description()
	{
		return _description;
	}

	std::string File()
	{
		return _file;
	}

	s32 Line()
	{
		return _line;
	}

protected:
	std::string _file;
	s32 _line;
	std::string _description;
};

class CLoggedException : public CException
{
public:
	CLoggedException(std::string file, s32 fine, std::string description);
	~CLoggedException() = default;

	void Log();
};
