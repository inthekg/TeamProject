#pragma once
#include "State.h"

NS_BEGIN(GameCore)

class CActionState abstract : public CState
{
public:
	explicit CActionState(PlayerActionID id) : m_iActionID(id) {}
public:
	PlayerActionID				Get_ActionID() const { return m_iActionID; }
	const vector<Transition>&	Get_LocalTransitions() const { return m_LocalTransitions; }
	bool						IsFinished() const { return m_bFinished; }
	//virtual void OnNotify(NotifyEvent eNotify); finished 처리하기 
protected:
	void AddTransition(int iPriority, PlayerActionID eTargetActionID, Guard guard)
	{
		m_LocalTransitions.emplace_back(iPriority, eTargetActionID, guard);
		Sort_TransitionList(m_LocalTransitions);
	}

protected:
	vector<Transition>			m_LocalTransitions;
	PlayerActionID				m_iActionID = {};
	_bool						m_bFinished  = {};
};

/* None Action State == IDLE */
class CNoneState : public CActionState
{
public:
	CNoneState();
	virtual void Begin(StateContext& Context) override;
};


/* Dodge State*/
class CDodgeState : public CActionState
{
public:
	CDodgeState();
	virtual void Begin(StateContext& Context) override;
	virtual void Update(StateContext& Context, _float fTimeDelta) override;
};


/* Interact State */
class CInteractState : public CActionState
{
public:
	CInteractState();
	virtual void		Begin(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDelta) override;
private:
	const ActionData*	m_pData = {};
	_float				m_fElapsed = {};
};

/* Attacking State */
class CAttackingState : public CActionState
{
public:
	CAttackingState();
	virtual void		Begin(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDelta) override;
	virtual void		End(StateContext& Context) override;
private:
	const WeaponData*	m_pCurWeaponData = {};
	_uint				m_iComboStep = {};
	bool				m_bComboEnded = {};
};
	
/* Hit State */
class CHitState : public CActionState
{
public:
	CHitState();
	virtual void		Begin(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDelta) override;
};

NS_END