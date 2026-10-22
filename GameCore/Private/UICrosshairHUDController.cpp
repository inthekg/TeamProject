#include "UICrosshairHUDController.h"
#include "CrosshairHUDViewModel.h"
#include "CrosshairHUDData.h"
#include "GameInstance.h"
#include "UIElement.h"

HRESULT CUICrosshairHUDController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    m_pViewModel = CCrosshairHUDViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    // 플레이어가 보내는 조준점 표시 이벤트를 구독한다.
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::CROSSHAIR_VISIBILITY_CHANGED, this);

    return S_OK;
}

HRESULT CUICrosshairHUDController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pCrosshair = Find_Element(m_strCrosshairBindingKey);

    if (m_pCrosshair == nullptr)
        return E_FAIL;

    // 조준 이벤트를 받기 전에는 숨긴다.
    m_pCrosshair->Set_Visible(false);
    m_pViewModel->Set_Visible(false);
    m_pViewModel->Clear_Dirty();

    return S_OK;
}

void CUICrosshairHUDController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    // 상태는 이벤트로 갱신하므로 매 프레임 조회할 내용은 없다.
}

void CUICrosshairHUDController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    if (m_pCrosshair != nullptr)
        m_pCrosshair->Set_Visible(m_pViewModel->Is_Visible());

    m_pViewModel->Clear_Dirty();
}

void CUICrosshairHUDController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_UI::CROSSHAIR_VISIBILITY_CHANGED)
        return;

    if (pData == nullptr || m_pViewModel == nullptr)
        return;

    CROSSHAIR_VISIBILITY_DESC* pDesc = static_cast<CROSSHAIR_VISIBILITY_DESC*>(pData);

    m_pViewModel->Set_Visible(pDesc->bVisible);
}

CUICrosshairHUDController* CUICrosshairHUDController::Create()
{
    return new CUICrosshairHUDController;
}

void CUICrosshairHUDController::Free()
{
    __super::Free();

    Safe_Release(m_pViewModel);

    m_pCrosshair = nullptr;
}