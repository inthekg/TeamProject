#include "UIStargazerController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIText.h"
#include "StargazerViewModel.h"

HRESULT CUIStargazerController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CStargazerViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    // 활성화된 별바라기가 기본 화면 열기를 요청하는 이벤트를 구독한다.
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_UI_OPEN, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE, this);

    return S_OK;
}

void CUIStargazerController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_bCloseAnimationPending)
        return;

    if (m_pViewModel == nullptr || Event.hUI != m_hUI || !m_pViewModel->Is_Visible())
        return;

    // 최상단 UI가 아니면 버튼 Command를 처리하지 않는다.
    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"PrimaryInputHint.HintButtonSpace")
            Handle_PrimaryAction();
        else if (Event.strRuntimeElementTag == L"CloseInputHint.HintButton")
            Close_Stargazer();

        return;
    }

    if (Event.strCommandKey == "SelectDialogueService" && Event.strRuntimeElementTag == L"ServiceOption0.ServiceOptionButton")
        Handle_PrimaryAction();
}

void CUIStargazerController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    // 실제 이동이 확정되면 부모 별바라기 화면도 닫는다.
    if (eType == EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE)
    {
        Close_Stargazer();
        return;
    }

    if (eType != EVENT_TYPE_MAP_OBJECT::STARGAZER_UI_OPEN || nullptr == pData)
        return;

    if (m_bCloseAnimationPending || m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    // 다른 Screen 또는 Push 대기 중인 Screen이 있으면 별바라기 화면을 열지 않는다.
    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::SCREEN))
        return;

    STARGAZER_UI_OPEN_DESC* pOpenDesc = static_cast<STARGAZER_UI_OPEN_DESC*>(pData);

    // 현재 별바라기 ID와 활성화된 목적지 목록을 ViewModel에 복사한다.
    m_pViewModel->Open_Stargazer(*pOpenDesc);
}

HRESULT CUIStargazerController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pServiceMenuGroup = Find_Element(L"ServiceMenuGroup");
    m_pServiceButton = Find_ElementAs<CUIButton>(L"ServiceOption0.ServiceOptionButton");
    m_pServiceLabel = Find_ElementAs<CUIText>(L"ServiceOption0.ServiceOptionLabel");
    m_pPrimaryInputButton = Find_ElementAs<CUIButton>(L"PrimaryInputHint.HintButtonSpace");
    m_pCloseInputButton = Find_ElementAs<CUIButton>(L"CloseInputHint.HintButton");

    if (m_pServiceMenuGroup == nullptr || m_pServiceButton == nullptr || m_pServiceLabel == nullptr || m_pPrimaryInputButton == nullptr ||  m_pCloseInputButton == nullptr )
        return E_FAIL;

    return S_OK;
}

void CUIStargazerController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    // Close 재생 중에는 입력을 막고 종료 시점만 검사한다.
    if (m_bCloseAnimationPending)
    {
        if (m_pUIObject == nullptr || m_pUIObject->Is_AnimationFinished())
            Finalize_Close();

        return;
    }

    if (!m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Close_Stargazer();
        return;
    }

    if (m_pGameInstance->Key_Down(DIK_SPACE))
        Handle_PrimaryAction();
}

void CUIStargazerController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (bShouldOpen)
    {
        if (!m_bHUDLayerSuppressed)
        {
            // 별바라기 화면이 유지되는 동안 게임 HUD를 숨긴다.
            m_pGameInstance->Acquire_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = true;
        }

        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Stargazer();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::SCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Stargazer();
                return;
            }

            // 화면과 입력 소유권이 모두 준비된 후 Open을 재생한다.
            if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            Begin_CloseAnimation();
            return;
        }

        // 실제 UI가 열려 있지 않은 예외 상황에서는 HUD 억제만 정리한다.
        if (m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Release_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = false;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    m_pServiceLabel->Set_Text(m_pViewModel->Get_ServiceText());
    m_pServiceMenuGroup->Set_Visible(true);
    m_pServiceButton->Set_Interactable(true);
    m_pServiceButton->Set_Selected(false);

    m_pViewModel->Clear_Dirty();
}

void CUIStargazerController::Handle_PrimaryAction()
{
    Request_TravelUI();
}

void CUIStargazerController::Request_TravelUI()
{
    if (m_pGameInstance == nullptr || m_pViewModel == nullptr)
        return;

    // 기본 별바라기 화면이 받은 정보를 텔레포트 Modal에 다시 전달한다.
    STARGAZER_UI_OPEN_DESC TravelOpenDesc = m_pViewModel->Get_OpenDesc();
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_REQUEST, &TravelOpenDesc);
}

void CUIStargazerController::Close_Stargazer()
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Close_Stargazer();
}

void CUIStargazerController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Close"))
    {
        // Close가 끝날 때까지 실제 Screen과 HUD 억제를 유지한다.
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    Finalize_Close();
}

void CUIStargazerController::Finalize_Close()
{
    if (m_pGameInstance->Is_OpenUIObject(m_hUI))
    {
        if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
            return;

        if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
            return;
    }

    // 별바라기 화면이 완전히 사라진 다음 플레이어 HUD를 복원한다.
    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    m_bCloseAnimationPending = false;

    if (m_pViewModel != nullptr)
        m_pViewModel->Clear_Dirty();

    m_pGameInstance->Broadcast(EVENT_TYPE::UI_END, nullptr);
}

CUIStargazerController* CUIStargazerController::Create()
{
    return new CUIStargazerController;
}
void CUIStargazerController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    Safe_Release(m_pViewModel);

    m_pServiceMenuGroup = nullptr;
    m_pServiceButton = nullptr;
    m_pServiceLabel = nullptr;

    m_pPrimaryInputButton = nullptr;
    m_pCloseInputButton = nullptr;

    __super::Free();
}