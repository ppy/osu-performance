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

	CException(std::string File, s32 Line, std::string Description);
	~CException();

	void Print();

	std::string Description()
	{
		return m_Description;
	}

	std::string File()
	{
		return m_File;
	}

	s32 Line()
	{
		return m_Line;
	}

protected:

	std::string m_File;
	s32 m_Line;
	std::string m_Description;
};


class CLoggedException : public CException
{

public:


	CLoggedException(std::string File, s32 Line, std::string Description);
	~CLoggedException();

	void Log();
};
