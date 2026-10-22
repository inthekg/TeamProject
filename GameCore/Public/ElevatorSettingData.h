#pragma once

#include "Core_Defines.h"

NS_BEGIN(GameCore)

struct ELEVATOR_SETTING_DATA
{
	_uint iElevatorSpawnID = UINT_MAX;

	_float3 vMoveOffset = { 0.f, 20.f ,0.f };
	_float  fMoveSpeed = 2.f;

	_bool bStartAtEnd = false;

};
NS_END 