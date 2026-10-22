#include "UIInteractionPromptController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIText.h"
#include "MapGameplayObject.h"
#include "InteractionPromptViewModel.h"

HRESULT CUIInteractionPromptController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CInteractionPromptViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::OBJ_UI, this);

    return S_OK;
}

HRESULT CUIInteractionPromptController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pPromptRoot = Find_Element(L"PromptRoot");
    m_pBGBlackOuter = Find_Element(L"PromptBGBlackOuter");
    m_pBGMain = Find_Element(L"PromptBGMain");
    m_pBGBlackInner = Find_Element(L"PromptBGBlackInner");
    m_pBGLight = Find_Element(L"PromptBGLight");
    m_pBGHighlight = Find_Element(L"PromptBGHighlight");
    m_pDirt = Find_Element(L"PromptDirt");
    m_pLineTop = Find_Element(L"PromptLineTop");
    m_pLineBottom = Find_Element(L"PromptLineBottom");
    m_pKeyGroup = Find_Element(L"PromptKeyGroup");
    m_pPromptText = Find_ElementAs<CUIText>(L"PromptText");

    if (m_pPromptRoot == nullptr || m_pBGBlackOuter == nullptr ||
        m_pBGMain == nullptr || m_pBGBlackInner == nullptr ||
        m_pBGLight == nullptr || m_pBGHighlight == nullptr ||
        m_pDirt == nullptr || m_pLineTop == nullptr ||
        m_pLineBottom == nullptr || m_pKeyGroup == nullptr ||
        m_pPromptText == nullptr)
    {
        return E_FAIL;
    }

    constexpr _float fDesignRootWidth = 170.f;

    m_fRuntimeScale = m_pPromptRoot->Get_UITransform().vSize.x / fDesignRootWidth;

    // 상호작용 이벤트를 받기 전에는 전체 안내창을 숨긴다.
    m_pUIObject->Set_Visible(false);

    return S_OK;
}

void CUIInteractionPromptController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (!m_bInactiveAnimationPending)
        return;

    // Inactive 애니메이션이 끝난 뒤에만 실제 UI를 숨긴다.
    if (m_pUIObject == nullptr || m_pUIObject->Is_AnimationFinished())
        Finalize_Inactive();
}

void CUIInteractionPromptController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldShow = m_pViewModel->Is_Visible();
    const _bool bActuallyVisible = m_pUIObject != nullptr && m_pUIObject->Is_Visible();

    if (bShouldShow)
    {
        const _wstring& strPromptText = m_pViewModel->Get_PromptText();

        // 애니메이션 시작 전에 텍스트와 배경 길이를 먼저 확정한다.
        if (m_pPromptText != nullptr)
        {
            m_pPromptText->Set_Text(strPromptText);
            Resize_Prompt(strPromptText);
        }

        const _bool bWasClosing = m_bInactiveAnimationPending;
        m_bInactiveAnimationPending = false;

        if (m_pUIObject != nullptr)
        {
            m_pUIObject->Set_Visible(true);

            // 처음 나타나거나 Inactive 도중 새 상호작용이 들어온 경우 Active를 재생한다.
            if ((!bActuallyVisible || bWasClosing) && m_pUIObject->Has_Animation("Active"))
                m_pUIObject->Play_Animation("Active", true);
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    if (bActuallyVisible)
        Begin_InactiveAnimation();
    else
        m_bInactiveAnimationPending = false;

    // Inactive 재생 중에는 실제 UI를 아직 숨기지 않는다.
    m_pViewModel->Clear_Dirty();
}

void CUIInteractionPromptController::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
   
}

void CUIInteractionPromptController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_MAP_OBJECT::OBJ_UI)
        return;

    if (pData == nullptr || m_pViewModel == nullptr)
        return;

    OBJ_UI_EVENT_DESC* pDesc = static_cast<OBJ_UI_EVENT_DESC*>(pData);

    if (pDesc->bVisible)
    {
        const _wstring strPromptText = Resolve_PromptText(pDesc->eObjType);

        // END 또는 아직 문구가 없는 타입은 표시하지 않는다.
        if (strPromptText.empty())
            return;

        m_pViewModel->Set_Prompt(pDesc->eObjType, strPromptText);
        m_pViewModel->Set_Visible(true);

        return;
    }

    /*
     * 예:
     *
     * 문 근처에 있다가 별바라기 영역으로 이동
     * → 별바라기 안내창 표시
     * → 뒤늦게 문 Hide 이벤트 도착
     *
     * 이때 현재 타입이 STARGAZER라면
     * DOOR의 Hide 이벤트로 숨기지 않는다.
     */

    if (m_pViewModel->Get_ObjectType() == pDesc->eObjType)
    {
        m_pViewModel->Set_Visible(false);
    }
}

_wstring CUIInteractionPromptController::Resolve_PromptText(OBJECT_TYPE eObjectType) const
{
    switch (eObjectType)
    {
    case OBJECT_TYPE::BROKEN_STARGAZER:
        return L"별바라기를 복구한다.";

    case OBJECT_TYPE::STARGAZER:
        return L"별바라기를 사용한다.";

    case OBJECT_TYPE::LADDER_UP:
        return L"올라간다.";

    case OBJECT_TYPE::LADDER_DOWN:
        return L"내려간다.";

    case OBJECT_TYPE::DOOR:
        return L"문을 연다.";

    case OBJECT_TYPE::LADDER_ACTIVATOR:
        return L"버튼을 누른다.";

    case OBJECT_TYPE::ELEVATOR_CONTROLLER:
        return L"레버를 올린다.";

    case OBJECT_TYPE::ITEM:
        return L"획득한다.";

    case OBJECT_TYPE::TREASURECHEST:
        return L"조사한다.";

    case OBJECT_TYPE::NPC:
        return L"말을 건다.";

    default:
        return L"";
    }
}

void CUIInteractionPromptController::Resize_Prompt(const _wstring& strPromptText)
{
    if (m_pPromptText == nullptr)
        return;

    const _float fTextWidth = ceilf(m_pPromptText->Measure_TextWidth(strPromptText));
    const _float fScale = m_fRuntimeScale;
    const _float fMinPromptWidth = 170.f * fScale;
    const _float fHorizontalSpace = 124.f * fScale;
    const _float fPromptWidth = max(fMinPromptWidth, fTextWidth + fHorizontalSpace);

    m_pPromptRoot->Set_Size({ fPromptWidth, 30.f * fScale });

    m_pBGBlackOuter->Set_Size({ fPromptWidth + 60.f * fScale, 44.f * fScale });

    m_pBGMain->Set_Size({ fPromptWidth + 76.f * fScale, 44.f * fScale });

    m_pBGBlackInner->Set_Size({ fPromptWidth - 20.f * fScale, 40.f * fScale });

    m_pBGLight->Set_Size({ fPromptWidth - 10.f * fScale, 36.f * fScale });

    m_pBGHighlight->Set_Size({ fPromptWidth - 35.f * fScale, 25.f * fScale });

    m_pDirt->Set_Size({ fPromptWidth - 56.f * fScale, 40.f * fScale });

    m_pLineTop->Set_Size({ fPromptWidth + 20.f * fScale, 5.f * fScale });

    m_pLineBottom->Set_Size({ fPromptWidth + 20.f * fScale, 5.f * fScale });

    // PromptRoot의 Pivot이 중앙이므로 왼쪽 끝은 -Width / 2다.

    const _float fLeft = -fPromptWidth * 0.5f;

    // E 키 그룹 중심 위치
    m_pKeyGroup->Set_Position({ fLeft + 58.f * fScale, 0.f });

    // 텍스트 영역 중심 위치
    m_pPromptText->Set_Position(
        {
           fLeft
                + 81.f * fScale
                + fTextWidth * 0.5f,
            0.f
        });

    m_pPromptText->Set_Size(
        {
           fTextWidth + 2.f * fScale,
            60.f * fScale
        });
}

void CUIInteractionPromptController::Begin_InactiveAnimation()
{
    if (m_pUIObject == nullptr || m_bInactiveAnimationPending)
        return;

    if (m_pUIObject->Has_Animation("Inactive"))
    {
        // 애니메이션이 보이도록 Visibility는 그대로 유지한다.
        m_bInactiveAnimationPending = true;
        m_pUIObject->Play_Animation("Inactive", true);
        return;
    }

    Finalize_Inactive();
}

void CUIInteractionPromptController::Finalize_Inactive()
{
    // Inactive 도중 새로운 표시 이벤트가 들어왔다면 숨기지 않는다.
    if (m_pViewModel != nullptr && m_pViewModel->Is_Visible())
    {
        m_bInactiveAnimationPending = false;
        return;
    }

    if (m_pUIObject != nullptr)
        m_pUIObject->Set_Visible(false);

    m_bInactiveAnimationPending = false;
}

CUIInteractionPromptController* CUIInteractionPromptController::Create()
{
    return new CUIInteractionPromptController;
}

void CUIInteractionPromptController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    Safe_Release(m_pViewModel);

    m_pPromptRoot = nullptr;

    m_pBGBlackOuter = nullptr;
    m_pBGMain = nullptr;
    m_pBGBlackInner = nullptr;
    m_pBGLight = nullptr;
    m_pBGHighlight = nullptr;
    m_pDirt = nullptr;

    m_pLineTop = nullptr;
    m_pLineBottom = nullptr;

    m_pKeyGroup = nullptr;
    m_pPromptText = nullptr;

    __super::Free();
}
