#include "UITitleController.h"
#include "GameInstance.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIText.h"

HRESULT CUITitleController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    // 로고 화면의 Title UI가 마우스 입력을 독점하도록 Screen 입력 소유자로 등록한다.
    if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::SCREEN)))
        return E_FAIL;

    m_pGameInstance->Set_CursorState(CURSOR_STATE::CLIP);

    // 같은 컴포넌트를 사용하므로 버튼마다 실제 커맨드를 구분해서 덮어쓴다.
    m_pContinueButton->Set_CommandKey("ContinueGame");
    m_pExitButton->Set_CommandKey("ExitGame");

    // 타이틀에서는 마우스를 올린 버튼이 선택 표시를 갖도록 한다.
    m_pContinueButton->Set_FocusOnHover(true);
    m_pContinueButton->Set_SelectOnFocus(true);
    m_pExitButton->Set_FocusOnHover(true);
    m_pExitButton->Set_SelectOnFocus(true);

    // 처음 열렸을 때 계속하기 버튼을 기본 선택 상태로 표시한다.
    m_pContinueButton->Set_Selected(true);

    m_pContinueText->Set_Text(L"계속하기");
    m_pExitText->Set_Text(L"게임 종료");

    return S_OK;
}

void CUITitleController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "ContinueGame")
    {
        // 실제 레벨 변경은 다음 레벨 정보를 가진 CLevel_Logo가 처리한다.
        m_pGameInstance->Broadcast(EVENT_TYPE::START_LEVEL, nullptr);
    }

    // ExitGame은 이번 작업 범위에서 처리하지 않는다.
}

HRESULT CUITitleController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pTitleMenuRoot = Find_Element(L"TitleMenuRoot");

    m_pContinueButton = Find_ElementAs<CUIButton>(L"ContinueButtonSlot.TitleMenuButton");
    m_pExitButton = Find_ElementAs<CUIButton>(L"ExitButtonSlot.TitleMenuButton");

    m_pContinueText = Find_ElementAs<CUIText>(L"ContinueButtonSlot.MenuText");
    m_pExitText = Find_ElementAs<CUIText>(L"ExitButtonSlot.MenuText");

    if (m_pTitleMenuRoot == nullptr || m_pContinueButton == nullptr || m_pExitButton == nullptr || m_pContinueText == nullptr || m_pExitText == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUITitleController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
}

void CUITitleController::Apply_ViewModel()
{
    // 고정 메뉴이므로 프레임마다 적용할 ViewModel 데이터는 없다.
}

CUITitleController* CUITitleController::Create()
{
    return new CUITitleController;
}

void CUITitleController::Free()
{
    m_pExitText = nullptr;
    m_pContinueText = nullptr;
    m_pExitButton = nullptr;
    m_pContinueButton = nullptr;
    m_pTitleMenuRoot = nullptr;

    __super::Free();
}