#include <Shared.h>

#include "DatabaseConnection.h"
#include "UpdateBatch.h"

CUpdateBatch::CUpdateBatch(std::shared_ptr<CDatabaseConnection> pDB, u32 sizeThreshold)
: _pDB{std::move(pDB)}, _sizeThreshold{sizeThreshold}
{
	Reset();
}

CUpdateBatch::~CUpdateBatch()
{
	// If we are not empty we want to commit what's left in here
	if(!_empty)
	{
		Execute();
	}
}

void CUpdateBatch::AppendAndCommit(const std::string& values)
{
	std::lock_guard<std::mutex> lock{_batchMutex};
	AppendAndCommitNonThreadsafe(values);
}

void CUpdateBatch::AppendAndCommitNonThreadsafe(const std::string& values)
{
	Append(values);

	if(Size() > _sizeThreshold)
	{
		Execute();
		Reset();
	}
}

void CUpdateBatch::Reset()
{
	_query = "";//"START TRANSACTION;";
	_empty = true;
}

const std::string& CUpdateBatch::Query()
{
	//m_Query += "COMMIT;";
	return _query;
}

void CUpdateBatch::Execute()
{
	_pDB->NonQueryBackground(Query());

	/*FILE* pFile = fopen("./updates.log", "ab");

	if(pFile != nullptr)
	{
		fputs(m_Query.c_str(), pFile);
		fclose(pFile);
	}*/
}
