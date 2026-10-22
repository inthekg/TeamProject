#pragma once
#include "Engine_Enum.h"
#include "Engine_Typedef.h"

namespace GameCore
{
	enum class ATK_GROUP { SHORT, LONG };

	typedef struct tagAttackData
	{
		float		fDamage = { 1.f };
		HIT_TYPE	eHitType = { HIT_TYPE::Normal };
	} ATTACK_DATA;

	typedef	struct tagPatternDef
	{
		_int		iID = { 0 };
		ATK_GROUP	eGroup = { ATK_GROUP::SHORT };
		_float		fWeight = { 1.f };
		_float		fAttackLength = { 1.f };
	} PATTERN_DEF;
}
