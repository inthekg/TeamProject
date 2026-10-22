#include "StanceLayer.h"

CStanceLayer::CStanceLayer()
{
}

HRESULT CStanceLayer::Initialize(StateContext& Context)
{
	m_StanceStateMap[PlayerStanceID::None] = new CNoneStanceState();
	if (m_StanceStateMap[PlayerStanceID::None] == nullptr) return E_FAIL;

	m_StanceStateMap[PlayerStanceID::Guard] = new CGuardState();
	if (m_StanceStateMap[PlayerStanceID::Guard] == nullptr) return E_FAIL;

	m_pCurStanceState = m_StanceStateMap[PlayerStanceID::None];
	return S_OK;
}

void CStanceLayer::Update(StateContext& Context, _float fTimeDelta)
{
	if (m_pCurStanceState == nullptr) return;
	if (m_pCurStanceState->Check_NextState(Context) != m_pCurStanceState->Get_Id())
	{
		PlayerStanceID eNextStanceID = m_pCurStanceState->Check_NextState(Context);
		Change_StanceState(eNextStanceID, Context);
	}
	m_pCurStanceState->Update(Context, fTimeDelta);
}

CStanceState* CStanceLayer::Find_StanceState(PlayerStanceID eLocoID)
{
	auto iter = m_StanceStateMap.find(eLocoID);
	if(iter == m_StanceStateMap.end())
		return nullptr;

	return iter->second;
}

void CStanceLayer::Change_StanceState(PlayerStanceID eNextLocoID, StateContext& Context)
{
	CStanceState* pNextState = Find_StanceState(eNextLocoID);
	if (pNextState == nullptr) return;

	if (m_pCurStanceState)
		m_pCurStanceState->End(Context);

	m_pCurStanceState = pNextState;

	m_pCurStanceState->Begin(Context);	
}

CStanceLayer* CStanceLayer::Create(StateContext& Context)
{
	CStanceLayer* pInstance = new CStanceLayer();
	if (FAILED(pInstance->Initialize(Context)))
	{
		MSG_BOX("Failed to Create CStanceLayer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CStanceLayer::Free()
{
	for (auto& Pair : m_StanceStateMap)
		Safe_Release(Pair.second);

	m_StanceStateMap.clear();
	
	__super::Free();
}
