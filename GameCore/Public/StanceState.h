#pragma once
#include "State.h"

NS_BEGIN(GameCore)
class CStanceState : public CState
{
public:
    explicit CStanceState(PlayerStanceID id) : m_iStanceID(id) {}
    PlayerStanceID Get_Id() const { return m_iStanceID; }
    virtual PlayerStanceID Check_NextState(StateContext&) { return m_iStanceID; }
protected:
    PlayerStanceID m_iStanceID = {};
};


class CNoneStanceState final : public CStanceState
{
public:
    CNoneStanceState() : CStanceState(PlayerStanceID::None) {}
	virtual PlayerStanceID Check_NextState(StateContext& Context) override;
};


class CGuardState final : public CStanceState
{
public:
    CGuardState() : CStanceState(PlayerStanceID::Guard) {}
    virtual PlayerStanceID Check_NextState(StateContext& Context) override;
};

NS_END