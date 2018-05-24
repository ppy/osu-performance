#include <pp/common.h>
#include <pp/shared/DatabaseConnection.h>
#include <pp/shared/UpdateBatch.h>

PP_NAMESPACE_BEGIN

UpdateBatch::UpdateBatch(std::shared_ptr<DatabaseConnection> pDB, u32 sizeThreshold)
: _pDB{std::move(pDB)}, _sizeThreshold{sizeThreshold}
{
	reset();
}

UpdateBatch::~UpdateBatch()
{
	// If we are not empty we want to commit what's left in here
	if (!_empty)
		execute();
}

UpdateBatch::UpdateBatch(UpdateBatch&& other)
{
	*this = std::move(other);
}

UpdateBatch& UpdateBatch::operator=(UpdateBatch&& other)
{
	std::lock_guard<std::mutex> otherLock{other._batchMutex};
	std::lock_guard<std::mutex> lock{_batchMutex};

	_sizeThreshold = other._sizeThreshold;
	_empty = other._empty;
	_pDB = std::move(other._pDB);
	_query = std::move(other._query);

	return *this;
}

void UpdateBatch::AppendAndCommit(const std::string& values)
{
	std::lock_guard<std::mutex> lock{_batchMutex};
	AppendAndCommitNonThreadsafe(values);
}

void UpdateBatch::AppendAndCommitNonThreadsafe(const std::string& values)
{
	append(values);

	if (Size() > _sizeThreshold)
	{
		execute();
		reset();
	}
}

void UpdateBatch::reset()
{
	_query = "";//"START TRANSACTION;";
	_empty = true;
}

const std::string& UpdateBatch::query()
{
	//m_Query += "COMMIT;";
	return _query;
}

void UpdateBatch::execute()
{
	_pDB->NonQueryBackground(query());

	/*FILE* pFile = fopen("./updates.log", "ab");

	if(pFile != nullptr)
	{
		fputs(m_Query.c_str(), pFile);
		fclose(pFile);
	}*/
}

PP_NAMESPACE_END
