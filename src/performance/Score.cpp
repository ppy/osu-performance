#include <pp/Common.h>

#include <pp/performance/Score.h>
#include <pp/shared/UpdateBatch.h>

PP_NAMESPACE_BEGIN

Score::Score(
	s64 scoreId,
	EGamemode mode,
	s64 userId,
	s32 beatmapId,
	s32 score,
	s32 maxCombo,
	s32 num300,
	s32 num100,
	s32 num50,
	s32 numMiss,
	s32 numGeki,
	s32 numKatu,
	EMods mods
)
:
_scoreId{scoreId},
_mode{mode},
_userId{userId},
_beatmapId{beatmapId},
_score{std::max(0, score)},
_maxCombo{std::max(0, maxCombo)},
_num300{std::max(0, num300)},
_num100{std::max(0, num100)},
_num50{std::max(0, num50)},
_numMiss{std::max(0, numMiss)},
_numGeki{std::max(0, numGeki)},
_numKatu{std::max(0, numKatu)},
_mods{mods}
{
}

void Score::AppendToUpdateBatch(UpdateBatch& batch) const
{
	batch.AppendAndCommitNonThreadsafe(StrFormat(
		"UPDATE `osu_scores{0}_high` "
		"SET `pp`={1} "
		"WHERE `score_id`={2};",
		GamemodeSuffix(_mode),
		TotalValue(),
		_scoreId
	));

	batch.AppendAndCommitNonThreadsafe(StrFormat("UPDATE score_process_queue SET status = 1 WHERE mode = {0} AND score_id = {1};", static_cast<int>(_mode), _scoreId));
}

PP_NAMESPACE_END
