#include "PlayerStateMachine.h"
#include "Player.h"
#include "ActionLayer.h"
#include "LocoLayer.h"
#include "StanceLayer.h"
#include "GameInstance.h"


CPlayerStateMachine::CPlayerStateMachine(CPlayer* pOwner)
	:CComponent(nullptr, nullptr), m_pOwner{ pOwner }
{

}

HRESULT CPlayerStateMachine::Initialize()
{
	if (m_pOwner == nullptr) return E_FAIL;

	m_Context.pInputCom = m_pOwner->Get_InputComponent();
	if (m_Context.pInputCom == nullptr) return E_FAIL;

	m_Context.pStatCom = m_pOwner->Get_StatComponent();
	if (m_Context.pStatCom == nullptr) return E_FAIL;

	m_Context.pStatusCom = m_pOwner->Get_StatusComponent();
	if (m_Context.pStatusCom == nullptr) return E_FAIL;

	m_pActionLayer = CActionLayer::Create(m_Context);
	if (m_pActionLayer == nullptr)
	{
		MSG_BOX("Failed to Create : ActionLayer");
		return E_FAIL;
	}

	m_pLocoLayer = CLocoLayer::Create(m_Context);
	if (m_pLocoLayer == nullptr)
	{
		MSG_BOX("Failed to Create : LocoLayer");
		return E_FAIL;
	}

	m_pStanceLayer = CStanceLayer::Create(m_Context);
	if(m_pStanceLayer == nullptr)
	{
		MSG_BOX("Failed to Create : StanceLayer");
		return E_FAIL;
	}
	return S_OK;
}

void CPlayerStateMachine::Update(_float fTimeDelta)
{
	m_pActionLayer->Update(m_Context, fTimeDelta);
	m_pLocoLayer->Update(m_Context, fTimeDelta);
	m_pStanceLayer->Update(m_Context, fTimeDelta);
}

CPlayerStateMachine* CPlayerStateMachine::Create(CPlayer* pOwner)
{
	CPlayerStateMachine* pInstance = new CPlayerStateMachine(pOwner);
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPlayerStateMachine");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CPlayerStateMachine::Free()
{
	Safe_Release(m_pActionLayer);
	Safe_Release(m_pLocoLayer);
	Safe_Release(m_pStanceLayer);
	__super::Free();
}