#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "GameInstance.h"
#include "UIAnimationPlayer.h"

CUIObject::CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

_bool CUIObject::Is_Visible() const
{
    return m_bVisible;
}

_uint CUIObject::Get_SortOrder() const
{
    return m_iSortOrder;
}

CUIElement* CUIObject::Get_RootElement() const
{
    return m_pRootElement;
}

// 이 UIObject에 포함된 Button을 입력 Router에 제공
const vector<CUIButton*>& CUIObject::Get_Buttons() const
{
    return m_Buttons;
}

_bool CUIObject::Is_RenderSuppressed() const
{
    return m_bRenderSuppressed;
}

_bool CUIObject::Has_Animation(const string& strClipName) const
{
    if (m_pAnimationPlayer == nullptr)
        return false;

    return m_pAnimationPlayer->Has_Clip(strClipName);
}

_bool CUIObject::Is_AnimationPlaying() const
{
    if (m_pAnimationPlayer == nullptr)
        return false;

    return m_pAnimationPlayer->Is_Playing();
}

_bool CUIObject::Is_AnimationFinished() const
{
    if (m_pAnimationPlayer == nullptr)
        return false;

    return m_pAnimationPlayer->Is_Finished();
}

void CUIObject::Set_Visible(_bool bVisible)
{
    m_bVisible = bVisible;
}

void CUIObject::Set_SortOrder(_uint iSortOrder)
{
    m_iSortOrder = iSortOrder;
}

HRESULT CUIObject::Set_RootElement(CUIElement* pRootElement)
{
    if (pRootElement == nullptr)
        return E_FAIL;

    if (m_pRootElement != nullptr)
        return E_FAIL;

    // 이미 다른 엘리먼트의 자식인 엘리먼트는 루트가 될 수 없음.
    if (pRootElement->Get_Parent() != nullptr)
        return E_FAIL;

    m_pRootElement = pRootElement;
    Safe_AddRef(m_pRootElement);

    return S_OK;
}

void CUIObject::Set_RenderSuppressed(_bool bSuppressed)
{
    m_bRenderSuppressed = bSuppressed;
    m_bRenderSuppressionRequested = bSuppressed;
    m_bPendingRenderSuppression = false;
}

HRESULT CUIObject::Set_Animations(const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Animations, const _float2& vRuntimeScale)
{
    Safe_Release(m_pAnimationPlayer);

    if (Animations.empty())
        return S_OK;

    m_pAnimationPlayer = CUIAnimationPlayer::Create(this, Animations, vRuntimeScale);

    if (m_pAnimationPlayer == nullptr)
        return E_FAIL;

    return S_OK;
}

HRESULT CUIObject::Initialize()
{
    return S_OK;
}

void CUIObject::Priority_Update(_float fTimeDelta)
{
    if (m_bVisible == false)
        return;

    for (auto& Pair : m_Elements)
    {
        if (Pair.second == nullptr)
            continue;

        Pair.second->Priority_Update(fTimeDelta);
    }
}

void CUIObject::Update(_float fTimeDelta)
{
    if (m_bVisible == false)
        return;

    // Element Update 전에 애니메이션 값을 적용해 이번 프레임 렌더에 반영한다.
    if (m_pAnimationPlayer != nullptr)
        m_pAnimationPlayer->Update(fTimeDelta);

    // Close 애니메이션이 끝난 다음 실제 렌더 억제를 적용합니다.
    if (m_bPendingRenderSuppression && m_bRenderSuppressionRequested && Is_AnimationFinished())
    {
        m_bRenderSuppressed = true;
        m_bPendingRenderSuppression = false;
    }

    for (auto& Pair : m_Elements)
    {
        if (Pair.second == nullptr)
            continue;

        Pair.second->Update(fTimeDelta);
    }
}

void CUIObject::Late_Update(_float fTimeDelta)
{
    if (m_bVisible == false)
        return;

    for (auto& Pair : m_Elements)
    {
        if (Pair.second == nullptr)
            continue;

        Pair.second->Late_Update(fTimeDelta);
    }
}

void CUIObject::Submit_RenderCommand()
{
    if (!m_bVisible || m_bRenderSuppressed)
        return;

    for (auto& Pair : m_Elements)
    {
        if (Pair.second == nullptr)
            continue;

        Pair.second->Submit_RenderCommand(m_iSortOrder);
    }
}

HRESULT CUIObject::Add_Element(const _wstring& strElementTag, CUIElement* pElement)
{
    if (strElementTag.empty() || pElement == nullptr)
        return E_FAIL;

    if (m_Elements.find(strElementTag) != m_Elements.end())
        return E_FAIL;

    m_Elements.emplace(strElementTag, pElement);
    Safe_AddRef(pElement);

    return S_OK;
}

CUIElement* CUIObject::Find_Element(const _wstring& strElementTag)
{
    auto iter = m_Elements.find(strElementTag);

    if (iter == m_Elements.end())
        return nullptr;

    return iter->second;
}

CUIElement* CUIObject::Find_Element(_uint iElementID)
{
    auto iter = m_AnimationTargets.find(iElementID);

    if (iter == m_AnimationTargets.end())
        return nullptr;

    return iter->second;
}

HRESULT CUIObject::Register_AnimationTarget(CUIElement* pElement)
{
    if (pElement == nullptr)
        return E_FAIL;

    const _uint iElementID = pElement->Get_ElementID();

    if (iElementID == UINT_MAX)
        return E_FAIL;

    if (m_AnimationTargets.find(iElementID) != m_AnimationTargets.end())
        return E_FAIL;

    m_AnimationTargets.emplace(iElementID, pElement);

    return S_OK;
}

// 생성된 Button을 입력용 목록에 한 번 등록
HRESULT CUIObject::Register_Button(CUIButton* pButton)
{
    if (pButton == nullptr)
        return E_FAIL;

    // 실제로 이 UIObject가 소유한 Element인지 확인
    const _bool bOwned = any_of(m_Elements.begin(), m_Elements.end(), [pButton](const auto& Pair)
        {
            return Pair.second == static_cast<CUIElement*>(pButton);
        });

    if (!bOwned)
        return E_FAIL;

    // 같은 Button의 중복 등록 방지
    if (find(m_Buttons.begin(), m_Buttons.end(), pButton) != m_Buttons.end())
        return E_FAIL;

    m_Buttons.push_back(pButton);

    return S_OK;
}

void CUIObject::Remove_Element(const _wstring& strElementTag)
{
    auto iter = m_Elements.find(strElementTag);

    if (iter == m_Elements.end())
        return;

    CUIElement* pRemovedElement = iter->second;

    m_Buttons.erase(remove_if(m_Buttons.begin(), m_Buttons.end(), [pRemovedElement](CUIButton* pButton)
        {
            return static_cast<CUIElement*>(pButton) == pRemovedElement;
        }), m_Buttons.end());

    for (auto iterTarget = m_AnimationTargets.begin(); iterTarget != m_AnimationTargets.end();)
    {
        if (iterTarget->second == pRemovedElement)
            iterTarget = m_AnimationTargets.erase(iterTarget);
        else
            ++iterTarget;
    }

    Safe_Release(iter->second);
    m_Elements.erase(iter);
}

void CUIObject::Clear_Elements()
{
    m_AnimationTargets.clear();
    m_Buttons.clear();

    for (auto& Pair : m_Elements)
        Safe_Release(Pair.second);

    m_Elements.clear();
}

_bool CUIObject::Play_Animation(const string& strClipName, _bool bRestart)
{
    if (m_pAnimationPlayer == nullptr)
        return false;

    return m_pAnimationPlayer->Play(strClipName, bRestart);
}

void CUIObject::Stop_Animation()
{
    if (m_pAnimationPlayer != nullptr)
        m_pAnimationPlayer->Stop();
}

void CUIObject::Pause_Animation()
{
    if (m_pAnimationPlayer != nullptr)
        m_pAnimationPlayer->Pause();
}

void CUIObject::Resume_Animation()
{
    if (m_pAnimationPlayer != nullptr)
        m_pAnimationPlayer->Resume();
}

_bool CUIObject::Set_AnimationTime(_float fTime)
{
    if (m_pAnimationPlayer == nullptr)
        return false;

    return m_pAnimationPlayer->Set_Time(fTime);
}

void CUIObject::Request_RenderSuppressed(_bool bSuppressed)
{
    // 같은 상태에 대한 반복 요청은 무시합니다.
    if (m_bRenderSuppressionRequested == bSuppressed)
        return;

    m_bRenderSuppressionRequested = bSuppressed;

    if (bSuppressed)
    {
        // Close 애니메이션이 없다면 기존처럼 즉시 숨깁니다.
        if (!m_bVisible || !Has_Animation("Close"))
        {
            m_bRenderSuppressed = true;
            m_bPendingRenderSuppression = false;
            return;
        }

        // Close가 보이도록 렌더링은 유지하고 애니메이션 종료를 기다립니다.
        m_bRenderSuppressed = false;
        m_bPendingRenderSuppression = true;
        Play_Animation("Close", true);
        return;
    }

    // 다시 표시할 때는 먼저 렌더 억제를 해제한 뒤 Open을 재생합니다.
    m_bPendingRenderSuppression = false;
    m_bRenderSuppressed = false;

    if (m_bVisible && Has_Animation("Open"))
        Play_Animation("Open", true);
}

CUIObject* CUIObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUIObject* pInstance = new CUIObject(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CUIObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CUIObject::Free()
{
    __super::Free();

    Safe_Release(m_pAnimationPlayer);

    Clear_Elements();
    Safe_Release(m_pRootElement);

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
