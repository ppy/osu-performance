#pragma once

#include "Score.h"

class CDatabaseConnection;
class CInsertionBatch;

class CUser
{
public:
	CUser(s64 id) : _id{id}
	{
	}

	struct SPPRecord
	{
		f64 Value = 0;
		f64 Accuracy = 0;
	};

	void AddScorePPRecord(CScore::SPPRecord score)
	{
		_scores.push_back(score);
	}

	void AppendToBatch(u32 sizeThreshold, CInsertionBatch& batch);

	s64 Id() const { return _id; }

	size_t NumScores() const { return _scores.size(); }

	void ComputePPRecord();

	const SPPRecord& PPRecord() const { return _rating; }

	CScore::SPPRecord XthBestScorePPRecord(unsigned int i);

private:
	s64 _id;
	SPPRecord _rating;
	std::vector<CScore::SPPRecord> _scores;
};
