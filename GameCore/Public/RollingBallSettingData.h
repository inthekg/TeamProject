#pragma once

#include "Core_Defines.h"
#include <cmath>

NS_BEGIN(GameCore)

// Editor 저장 데이터다. 경로점은 절대 월드 위치로 보관한다.
struct ROLLING_BALL_SETTING_DATA
{
	_uint iRollingBallSpawnID = UINT_MAX;

	vector<_float3> WaypointPositions = {};

	_float fRadius = 0.5f;
	_float fTriggerRadius = 0.65f;
	_float fMass = 50.f;

	_float fMoveSpeed = 6.f;
	_float fSteerGain = 4.f;
	_float fMaxSteerAcceleration = 12.f;
	_float fLookAheadDistance = 2.f;

	_float fWaypointRadius = 0.75f;
	_float fFinishRadius = 1.f;
	_float fHiddenTime = 1.f;

	_float fDamage = 10.f;

	_float fKillY = -1000.f;
	_float fStuckSpeed = 0.1f;
	_float fStuckTime = 3.f;
};

// Editor, Serializer, Runtime Manager가 동일한 설정 규칙을 사용한다.
inline _bool Is_ValidRollingBallSettingData(const ROLLING_BALL_SETTING_DATA& Setting)
{
	if (UINT_MAX == Setting.iRollingBallSpawnID || Setting.WaypointPositions.empty())
		return false;

	if (false == std::isfinite(Setting.fRadius) || false == std::isfinite(Setting.fTriggerRadius) || false == std::isfinite(Setting.fMass))
		return false;

	if (Setting.fRadius <= 0.f || Setting.fTriggerRadius < Setting.fRadius || Setting.fMass <= 0.f)
		return false;

	if (false == std::isfinite(Setting.fMoveSpeed) || false == std::isfinite(Setting.fSteerGain) || false == std::isfinite(Setting.fMaxSteerAcceleration) || false == std::isfinite(Setting.fLookAheadDistance))
		return false;

	if (Setting.fMoveSpeed <= 0.f || Setting.fSteerGain <= 0.f || Setting.fMaxSteerAcceleration <= 0.f || Setting.fLookAheadDistance <= 0.f)
		return false;

	if (false == std::isfinite(Setting.fWaypointRadius) || false == std::isfinite(Setting.fFinishRadius) || false == std::isfinite(Setting.fHiddenTime))
		return false;

	if (Setting.fWaypointRadius <= 0.f || Setting.fFinishRadius <= 0.f || Setting.fHiddenTime < 0.f)
		return false;

	if (false == std::isfinite(Setting.fDamage) || false == std::isfinite(Setting.fKillY) || false == std::isfinite(Setting.fStuckSpeed) || false == std::isfinite(Setting.fStuckTime))
		return false;

	if (Setting.fDamage < 0.f || Setting.fStuckSpeed < 0.f || Setting.fStuckTime <= 0.f)
		return false;

	for (const _float3& vPosition : Setting.WaypointPositions)
	{
		if (false == std::isfinite(vPosition.x) || false == std::isfinite(vPosition.y) || false == std::isfinite(vPosition.z))
			return false;
	}

	return true;
}

NS_END
