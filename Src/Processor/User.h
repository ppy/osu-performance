#pragma once

#include "Score.h"

class CDatabaseConnection;
class CInsertionBatch;

class CUser
{
public:
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

	size_t NumScores() const { return _scores.size(); }

	SPPRecord ComputePPRecord();

	CScore::SPPRecord XthBestScorePPRecord(unsigned int i);

private:
	SPPRecord _rating;
	std::vector<CScore::SPPRecord> _scores;
};
