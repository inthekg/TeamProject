#include "UILockOnController.h"
#include "LockOnViewModel.h"
#include "UIObject.h"
#include "Status.h"
#include "AIObject.h"
#include "UIElement.h"
#include "GameInstance.h"


CUILockOnController::CUILockOnController()
{
}

HRESULT CUILockOnController::Initialize(void* pArg)
{
	LOCKON_CONTROLLER_DESC* pDesc = static_cast<LOCKON_CONTROLLER_DESC*>(pArg);

	if (pDesc == nullptr)
		return E_FAIL;

	if (pDesc->pStatusCom == nullptr)
		return E_FAIL;

	m_pStatusCom = pDesc->pStatusCom;
	m_vScreenOffset = pDesc->vScreenOffset;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pViewModel = CLockOnViewModel::Create();

	if (m_pViewModel == nullptr)
		return E_FAIL;

	return S_OK;
}

HRESULT CUILockOnController::Bind_UI()
{
	if (FAILED(__super::Bind_UI()))
		return E_FAIL;

	m_pLockOnElement = Find_Element(m_strLockOnElementTag);

	if (m_pLockOnElement == nullptr)
		return E_FAIL;

	m_pLockOnElement->Set_Visible(false);

	return S_OK;
}

void CUILockOnController::Update_ViewModel(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (m_pViewModel == nullptr)
		return;

	if (m_pStatusCom == nullptr || !m_pStatusCom->Is_LockedOn())
	{
		// 락온이 완전히 해제되었으므로 다음 락온은 새로운 진입으로 처리한다.
		m_pPreviousLockedTarget = nullptr;
		m_bPendingActiveAnimation = false;
		m_pViewModel->Set_Visible(false);
		return;
	}

	CGameObject* pTargetObject = m_pStatusCom->Get_LockedTarget();
	CAIObject* pTarget = dynamic_cast<CAIObject*>(pTargetObject);

	if (pTarget == nullptr || pTarget->Is_Dead())
	{
		// 유효하지 않은 타깃은 락온이 해제된 것으로 처리한다.
		m_pPreviousLockedTarget = nullptr;
		m_bPendingActiveAnimation = false;
		m_pViewModel->Set_Visible(false);
		return;
	}

	_vector vLockOnWorldPosition = {};

	if (!pTarget->Get_LockOnWorldPosition(vLockOnWorldPosition))
	{
		m_pViewModel->Set_Visible(false);
		return;
	}

	_float2 vScreenPosition = {};

	if (!WorldToScreen(vLockOnWorldPosition, vScreenPosition))
	{
		m_pViewModel->Set_Visible(false);
		return;
	}

	vScreenPosition.x += m_vScreenOffset.x;
	vScreenPosition.y += m_vScreenOffset.y;

	// 최초 락온 또는 다른 타깃으로 락온이 변경된 순간에만 애니메이션을 예약한다.
	if (m_pPreviousLockedTarget != pTargetObject)
	{
		m_pPreviousLockedTarget = pTargetObject;
		m_bPendingActiveAnimation = true;
	}

	m_pViewModel->Set_Position(vScreenPosition);
	m_pViewModel->Set_Visible(true);
}

void CUILockOnController::Apply_ViewModel()
{
	if (m_pViewModel == nullptr)
		return;

	const _bool bDirty = m_pViewModel->Is_Dirty();

	if (bDirty && m_pLockOnElement != nullptr)
	{
		m_pLockOnElement->Set_Position(m_pViewModel->Get_Position());
		m_pLockOnElement->Set_Visible(m_pViewModel->Is_Visible());
	}

	// 엘리먼트를 먼저 표시한 뒤 Active 애니메이션을 한 번 재생한다.
	if (m_bPendingActiveAnimation && m_pViewModel->Is_Visible() && m_pUIObject != nullptr)
	{
		m_pUIObject->Play_Animation("Active", true);
		m_bPendingActiveAnimation = false;
	}

	if (bDirty)
		m_pViewModel->Clear_Dirty();
}

CUILockOnController* CUILockOnController::Create()
{
	return new CUILockOnController;
}

void CUILockOnController::Free()
{
	__super::Free();

	Safe_Release(m_pViewModel);

	m_pStatusCom = nullptr;
	m_pLockOnElement = nullptr;
	m_pPreviousLockedTarget = nullptr;
	m_bPendingActiveAnimation = false;
}
