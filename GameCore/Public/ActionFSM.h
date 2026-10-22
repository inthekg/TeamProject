#pragma once
#include "ActionState.h"


NS_BEGIN(GameCore)

class CActionFSM final : public CBase
{
public:
	struct NotifyDesc
	{
		_uint iComboStep;
	};

private:
	CActionFSM();
	virtual ~CActionFSM() = default;

public:
	void Add_GlobalTransition(int iPriority, PlayerActionID eTargetActionID, Guard guard)
	{
		m_GlobalTransitions.emplace_back(iPriority, eTargetActionID, guard);
		Sort_TransitionList(m_GlobalTransitions);
	}
public:
	PlayerActionID									Get_CurActionID() const { return m_pCurActionState ? m_pCurActionState->Get_ActionID() : PlayerActionID::None; }
	const vector<Transition>&						Get_GlobalTransitions() const { return m_GlobalTransitions; }
	const CActionState*								Get_CurActionState() const { return m_pCurActionState; }

public:
	HRESULT											Initialize();
	void											Update(StateContext& Context, _float fTimeDelta);

	HRESULT											Start(PlayerActionID eActionID, StateContext& Context);
public:
	//void OnNotify(EVENT_TYPE eType, Event_Desc* pData) override;

private:
	PlayerActionID									Evaluate_Priority(StateContext& Context);

	CActionState*									Find_ActionState(PlayerActionID eActionID);
	void											Change_ActionState(PlayerActionID eNextActionID, StateContext& Context);

private:
	unordered_map<PlayerActionID, CActionState*>	m_ActionStateMap;
	CActionState*									m_pCurActionState = {};

	vector<Transition>								m_GlobalTransitions;
	vector<Transition>								m_EmptyTransition;

public:
	static CActionFSM* Create();
	virtual void Free() override;
};
NS_END
