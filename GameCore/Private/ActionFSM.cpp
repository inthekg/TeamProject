#include "ActionFSM.h"

CActionFSM::CActionFSM() 
{
	
}

HRESULT CActionFSM::Initialize()
{
	m_ActionStateMap[PlayerActionID::None] = new CNoneState();
	if (m_ActionStateMap[PlayerActionID::None] == nullptr) return E_FAIL;

	m_ActionStateMap[PlayerActionID::Dodge] = new CDodgeState();
	if (m_ActionStateMap[PlayerActionID::Dodge] == nullptr) return E_FAIL;

	return S_OK;
}

void CActionFSM::Update(StateContext& Context, _float fTimeDelta)
{
	PlayerActionID eNextActionID = Evaluate_Priority(Context);
	if(eNextActionID != PlayerActionID::End)
	{
		Change_ActionState(eNextActionID, Context);
	}

	if(m_pCurActionState)
		m_pCurActionState->Update(Context, fTimeDelta);	
}

HRESULT CActionFSM::Start(PlayerActionID eActionID, StateContext& Context)
{
	CActionState* pState = Find_ActionState(eActionID);
	if (pState == nullptr) return E_FAIL;

	m_pCurActionState = pState;
	m_pCurActionState->Begin(Context);
	return S_OK;
}

PlayerActionID CActionFSM::Evaluate_Priority(StateContext& Context)
{
	const vector<Transition>& localTransitions = m_pCurActionState ? m_pCurActionState->Get_LocalTransitions() : m_EmptyTransition;	
	size_t iLocalIndex = 0, iGlobalIndex = 0;

	while(iLocalIndex < localTransitions.size() || iGlobalIndex < m_GlobalTransitions.size())
	{
		_bool bGlobalValid = false;

		if(iLocalIndex >= localTransitions.size() && iGlobalIndex >= m_GlobalTransitions.size()) return PlayerActionID::End;

		else if(iLocalIndex >= localTransitions.size()) bGlobalValid = true;
		else if(iGlobalIndex >= m_GlobalTransitions.size()) bGlobalValid = false;
		else
		{
			bGlobalValid = (m_GlobalTransitions[iGlobalIndex].iPriority >= localTransitions[iLocalIndex].iPriority);
		}

		if (bGlobalValid)
		{
			if (m_GlobalTransitions[iGlobalIndex].Guard_Func(Context))
				return m_GlobalTransitions[iGlobalIndex].ActionID;

			++iGlobalIndex;
		}
		else
		{
			if (localTransitions[iLocalIndex].Guard_Func(Context))
				return localTransitions[iLocalIndex].ActionID;

			++iLocalIndex;
		}
	}

	return PlayerActionID::End;	
}

CActionState* CActionFSM::Find_ActionState(PlayerActionID eActionID)
{
	auto iter = m_ActionStateMap.find(eActionID);
	if(iter == m_ActionStateMap.end())
		return nullptr;	
	return iter->second;
}

void CActionFSM::Change_ActionState(PlayerActionID eNextActionID, StateContext& Context)
{
	CActionState* pNextState = Find_ActionState(eNextActionID);
	if (pNextState)
	{
		if (m_pCurActionState)
			m_pCurActionState->End(Context);

		m_pCurActionState = pNextState;
		m_pCurActionState->Begin(Context);
	}
}

CActionFSM* CActionFSM::Create()
{
	CActionFSM* pInstance = new CActionFSM();
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create CActionFSM");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CActionFSM::Free()
{
	for(auto& pair : m_ActionStateMap)
	{
		Safe_Release(pair.second);
	}
	m_ActionStateMap.clear();
	__super::Free();
}
