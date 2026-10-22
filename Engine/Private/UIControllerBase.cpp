#include "UIControllerBase.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"

CUIControllerBase::CUIControllerBase()
    : m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CUIControllerBase::Initialize(void* pArg)
{
    UI_CONTROLLER_DESC* pDesc = static_cast<UI_CONTROLLER_DESC*>(pArg);

    if (pDesc != nullptr)
        m_hUI = pDesc->hUI;

    if (FAILED(Bind_UI()))
        return E_FAIL;

    return S_OK;
}

void CUIControllerBase::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    // Button Command가 필요 없는 Controller의 기본 동작
    UNREFERENCED_PARAMETER(Event);
}

void CUIControllerBase::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    // Focus 처리가 필요 없는 Controller의 기본 동작
    UNREFERENCED_PARAMETER(Event);
}

void CUIControllerBase::Update(_float fTimeDelta)
{
    // 핸들을 사용하는 일반 컨트롤러는 인스턴스 제거 후 더 이상 캐시 포인터에 접근하지 않는다
    if (m_hUI.Is_Valid() && !m_pGameInstance->Is_AliveUIObject(m_hUI))
        return;

    Update_ViewModel(fTimeDelta);
    Apply_ViewModel();
}

HRESULT CUIControllerBase::Bind_UI()
{
    if (!m_hUI.Is_Valid())
        return E_FAIL;

    m_pUIObject = m_pGameInstance->Find_UIObject(m_hUI);

    if (m_pUIObject == nullptr)
        return E_FAIL;

    return S_OK;
}

CUIElement* CUIControllerBase::Find_Element(const _wstring& strElementTag)
{
    if (m_pUIObject == nullptr)
        return nullptr;

    return m_pUIObject->Find_Element(strElementTag);
}

_bool CUIControllerBase::WorldToScreen(_fvector vWorldPosition, _float2& vOutScreenPosition)
{
    const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
    const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);

    if (pView == nullptr || pProj == nullptr)
        return false;

    const _matrix matView = XMLoadFloat4x4(pView);
    const _matrix matProj = XMLoadFloat4x4(pProj);

    _vector vView = XMVector3TransformCoord(vWorldPosition, matView);
    _vector vClip = XMVector3TransformCoord(vView, matProj);

    const _float fNdcX = XMVectorGetX(vClip);
    const _float fNdcY = XMVectorGetY(vClip);
    const _float fNdcZ = XMVectorGetZ(vClip);

    if (fNdcZ < 0.f || fNdcZ > 1.f)
        return false;

    if (fNdcX < -1.2f || fNdcX > 1.2f || fNdcY < -1.2f || fNdcY > 1.2f)
        return false;

    const _float2 vViewportSize = m_pGameInstance->Get_ViewportDesc();

    vOutScreenPosition.x = fNdcX * vViewportSize.x * 0.5f;
    vOutScreenPosition.y = fNdcY * vViewportSize.y * 0.5f;

    return true;
}

void CUIControllerBase::Free()
{
    __super::Free();

    // UIObject는 UI_Manager가 소유한다. 컨트롤러는 포인터만 캐시한다.
    m_hUI.Invalidate();
    m_pUIObject = nullptr;
}