#include <PrecompiledHeader.h>


#include "Processor.h"

#include "Score.h"
#include "SharedEnums.h"

#include "../Shared/Network/UpdateBatch.h"


using namespace SharedEnums;


CScore::CScore(
	s64 scoreId,
	EGamemode mode,
	s32 userId,
	s32 beatmapId,
	s32 score,
	s32 maxCombo,
	s32 amount300,
	s32 amount100,
	s32 amount50,
	s32 amountMiss,
	s32 amountGeki,
	s32 amountKatu,
	EMods mods)
:
_scoreId{scoreId},
_mode{mode},
_userId{userId},
_beatmapId{beatmapId},
_score{std::max(0, score)},
_maxCombo{std::max(0, maxCombo)},
_amount300{std::max(0, amount300)},
_amount100{std::max(0, amount100)},
_amount50{std::max(0, amount50)},
_amountMiss{std::max(0, amountMiss)},
_amountGeki{std::max(0, amountGeki)},
_amountKatu{std::max(0, amountKatu)},
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
		_scoreId));
}
