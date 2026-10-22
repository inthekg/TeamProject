#include "ActionLayer.h"
#include "ActionFSM.h"
#include "InputComponent.h"
#include "Status.h"
#include "StatComponent.h"

CActionLayer::CActionLayer()
{
	
}

HRESULT CActionLayer::Initialize(StateContext& Context)
{
	m_pActionFSM = CActionFSM::Create();	
	if (m_pActionFSM == nullptr) return E_FAIL;

	m_pActionFSM->Add_GlobalTransition(600, PlayerActionID::Dead, [](StateContext& Context) -> _bool
		{
			return Context.pStatCom->Is_Dead();
		});

	m_pActionFSM->Add_GlobalTransition(500, PlayerActionID::Hit, [](StateContext& Context) -> _bool
	{
		_bool bCanDamanage = !Context.pStatusCom->Is_Invincible() && !Context.pStatusCom->Is_SuperArmor();
		return bCanDamanage && (Context.pStatusCom->Is_TakeDamage() || Context.pStatusCom->Is_TakeGuardHit());
	});

	if (FAILED(m_pActionFSM->Start(PlayerActionID::None, Context)))
		return E_FAIL;

	return S_OK;
}

void CActionLayer::Update(StateContext& Context, _float fTimeDelta)
{
	m_pActionFSM->Update(Context, fTimeDelta);
	Context.pStatusCom->Set_ActionPlaying(m_pActionFSM->Get_CurActionID() != PlayerActionID::None);
}

CActionLayer* CActionLayer::Create(StateContext& Context)
{
	CActionLayer* pInstance = new CActionLayer();
	if (FAILED(pInstance->Initialize(Context)))
	{
		MSG_BOX("Failed to Create CActionLayer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CActionLayer::Free()
{
	__super::Free();
	Safe_Release(m_pActionFSM);
}
