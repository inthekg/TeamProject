#pragma once
#include "State.h"

NS_BEGIN(GameCore)

class CLocoState : public CState
{
public:
	explicit CLocoState(PlayerLocoID id) : m_iLocoID(id) {}
public:
	PlayerLocoID Get_ActionID() const { return m_iLocoID; }
	virtual PlayerLocoID Check_NextState(StateContext& Context) { return m_iLocoID; } // 기본 : 현 상태 유지 

protected:
	PlayerLocoID          m_iLocoID = {};
};

class GroundedState : public CLocoState
{
public:
	GroundedState() : CLocoState(PlayerLocoID::Grounded) {}
	PlayerLocoID Check_NextState(StateContext& Context) override;
};

class AirborneState : public CLocoState
{
public:
	AirborneState() : CLocoState(PlayerLocoID::Airborne) {}
	PlayerLocoID Check_NextState(StateContext& Context) override;
};

NS_END