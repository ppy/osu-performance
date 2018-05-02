#include <PrecompiledHeader.h>

#include "Processor.h"

#include "Score.h"
#include "SharedEnums.h"

#include "../Shared/Network/UpdateBatch.h"

CScore::CScore(
	s64 scoreId,
	EGamemode mode,
	s32 userId,
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

void CScore::AppendToUpdateBatch(CUpdateBatch& batch)
{
	batch.AppendAndCommitNonThreadsafe(StrFormat(
		"UPDATE `osu_scores{0}_high` "
		"SET `pp`={1} "
		"WHERE `score_id`={2};",
		CProcessor::GamemodeSuffix(_mode),
		TotalValue(),
		_scoreId
	));
}
