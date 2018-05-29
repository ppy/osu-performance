#include <pp/Common.h>
#include <pp/performance/User.h>

PP_NAMESPACE_BEGIN

// Our own implementation of unique ensures that the first unique element
// (in our case the highest pp-giving score) is always kept
template<class ForwardIt, class BinaryPredicate>
ForwardIt highestUnique(ForwardIt first, ForwardIt last, BinaryPredicate p)
{
	if (first == last)
		return last;

	ForwardIt result = first;
	while (++first != last)
		if (!p(*result, *first))
			*(++result) = *first;

	return ++result;
}

void User::ComputePPRecord()
{
	// Eliminate duplicate beatmaps with lower pp
	std::sort(std::begin(_scores), std::end(_scores), [](const Score::PPRecord& a, const Score::PPRecord& b)
	{
		return a.BeatmapId < b.BeatmapId || (a.BeatmapId == b.BeatmapId && b.Value < a.Value);
	});

	_scores.erase(
		highestUnique(
			_scores.begin(),
			_scores.end(),
			[](const Score::PPRecord& a, const Score::PPRecord& b) { return a.BeatmapId == b.BeatmapId; }
		),
		_scores.end()
	);

	// Sort values in descending order
	std::sort(std::begin(_scores), std::end(_scores), [](const Score::PPRecord& a, const Score::PPRecord& b)
	{
		return b.Value < a.Value;
	});

	_rating = PPRecord{};

	// Build the diminishing sum
	f64 factor = 1;

	for (const auto& score : _scores)
	{
		_rating.Value += score.Value * factor;
		_rating.Accuracy += score.Accuracy * factor;
		factor *= 0.95;
	}

	// This weird factor is to keep legacy compatibility with the diminishing bonus of 0.25 by 0.9994 each score
	_rating.Value += (417.0 - 1.0 / 3.0) * (1.0 - pow(0.9994, _scores.size()));

	// We want our accuracy to be normalized.
	if (_scores.size() > 0)
		// We want the percentage, not a factor in [0, 1], hence we divide 20 by 100
		_rating.Accuracy *= 100.0 / (20 * (1 - pow(0.95, _scores.size())));
}

Score::PPRecord User::XthBestScorePPRecord(unsigned int i)
{
	if (i >= _scores.size())
		return Score::PPRecord{0, 0, 0, 0};

	return _scores[i];
}

PP_NAMESPACE_END
