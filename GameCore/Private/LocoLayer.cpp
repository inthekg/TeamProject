#include "LocoLayer.h"

CLocoLayer::CLocoLayer()
{
}

HRESULT CLocoLayer::Initialize(StateContext& Context)
{
	m_LocoStateMap[PlayerLocoID::Grounded] = new GroundedState();
	if (m_LocoStateMap[PlayerLocoID::Grounded] == nullptr) return E_FAIL;

	m_LocoStateMap[PlayerLocoID::Airborne] = new AirborneState();
	if (m_LocoStateMap[PlayerLocoID::Airborne] == nullptr) return E_FAIL;

	//m_LocoStateMap[PlayerLocoID::Climb] = new ClimbState();

	m_pCurLocoState = m_LocoStateMap[PlayerLocoID::Grounded];
	return S_OK;
}

void CLocoLayer::Update(StateContext& Context, _float fTimeDelta)
{
	if (m_pCurLocoState == nullptr) return;
	if(m_pCurLocoState->Check_NextState(Context) != m_pCurLocoState->Get_ActionID())
	{
		PlayerLocoID eNextLocoID = m_pCurLocoState->Check_NextState(Context);
		Change_LocoState(eNextLocoID, Context);
	}
	m_pCurLocoState->Update(Context, fTimeDelta);
}

CLocoState* CLocoLayer::Find_LocoState(PlayerLocoID eLocoID)
{
	auto iter = m_LocoStateMap.find(eLocoID);
	if (iter == m_LocoStateMap.end()) return nullptr;
	else return iter->second;
}

void CLocoLayer::Change_LocoState(PlayerLocoID eNextLocoID, StateContext& Context)
{
	CLocoState* pNextState = Find_LocoState(eNextLocoID);
	if (pNextState == nullptr) return;

	if(m_pCurLocoState)
		m_pCurLocoState->End(Context);

	m_pCurLocoState = pNextState;

	m_pCurLocoState->Begin(Context);
}

CLocoLayer* CLocoLayer::Create(StateContext& Context)
{
	CLocoLayer* pInstance = new CLocoLayer();
	if (FAILED(pInstance->Initialize(Context)))
	{
		MSG_BOX("Failed to Create CLocoLayer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CLocoLayer::Free()
{
	for(auto& Pair : m_LocoStateMap)
	{
		Safe_Release(Pair.second);
	}
	m_LocoStateMap.clear();

	__super::Free();
}
