#pragma once

#include <pp/common.h>
#include <pp/performance/Score.h>

#include <vector>

PP_NAMESPACE_BEGIN

class User
{
public:
	User(s64 id) : _id{id}
	{
	}

	struct PPRecord
	{
		f64 Value = 0;
		f64 Accuracy = 0;
	};

	void AddScorePPRecord(Score::PPRecord score)
	{
		_scores.push_back(score);
	}

	s64 Id() const { return _id; }

	size_t NumScores() const { return _scores.size(); }

	void ComputePPRecord();

	const PPRecord& GetPPRecord() const { return _rating; }

	Score::PPRecord XthBestScorePPRecord(unsigned int i);

private:
	s64 _id;
	PPRecord _rating;
	std::vector<Score::PPRecord> _scores;
};

PP_NAMESPACE_END
