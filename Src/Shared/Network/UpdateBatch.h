#pragma once

class CDatabaseConnection;

class CUpdateBatch
{
public:
	CUpdateBatch(std::shared_ptr<CDatabaseConnection> pDB, u32 sizeThreshold);
	~CUpdateBatch();

	CUpdateBatch& operator=(CUpdateBatch&& other);
	CUpdateBatch(CUpdateBatch&& other);

	void AppendAndCommit(const std::string& values);
	void AppendAndCommitNonThreadsafe(const std::string& values);

	std::mutex& Mutex() { return _batchMutex; }

private:
	void Append(const std::string& values)
	{
		_empty = false;
		_query += values;
	}

	void Reset();
	const std::string& Query();

	u32 Size() const { return (u32)_query.size(); }

	void Execute();

	u32 _sizeThreshold;

	bool _empty = true;

	std::shared_ptr<CDatabaseConnection> _pDB;
	std::mutex _batchMutex;

	std::string _query;
};
