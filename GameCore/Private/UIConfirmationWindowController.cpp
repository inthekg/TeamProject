#include "UIConfirmationWindowController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIText.h"

HRESULT CUIConfirmationWindowController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CConfirmationWindowViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    // 상점, 강화창 등에서 보내는 공용 확인창 열기 요청을 받는다.
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST, this);

    return S_OK;
}

void CUIConfirmationWindowController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey != "ActivateInputHint")
        return;

    if (Event.strRuntimeElementTag == L"ConfirmKeyHint.HintButtonSpace")
    {
        Resolve_Confirmation(true);
    }
    else if (Event.strRuntimeElementTag == L"CancelKeyHint.HintButton")
    {
        Resolve_Confirmation(false);
    }
}

void CUIConfirmationWindowController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST || pData == nullptr || m_pViewModel == nullptr)
        return;

    // 확인창이 열린 상태에서는 두 번째 요청을 중첩해서 받지 않는다.
    if (m_pViewModel->Is_Visible())
        return;

    CONFIRMATION_REQUEST_DESC* pRequestDesc = static_cast<CONFIRMATION_REQUEST_DESC*>(pData);

    if (pRequestDesc->eActionType == CONFIRMATION_ACTION_TYPE::END)
        return;

    m_pViewModel->Open_Confirmation(*pRequestDesc);
}

HRESULT CUIConfirmationWindowController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pConfirmTitle = Find_ElementAs<CUIText>(L"ConfirmTitle");
    m_pGoodsText = Find_ElementAs<CUIText>(L"GoodsText");
    m_pConfirmText = Find_ElementAs<CUIText>(L"ConfirmText");

    m_pPriceRoot = Find_Element(L"Price");
    m_pPriceText = Find_ElementAs<CUIText>(L"PriceText");
    m_pGoodsPrice = Find_ElementAs<CUIText>(L"GoodsPrice");
    m_pErgoCount = Find_ElementAs<CUIText>(L"ErgoCount");

    m_pConfirmKeyHint = Find_Element(L"ConfirmKeyHint");
    m_pConfirmActionText = Find_ElementAs<CUIText>(L"ConfirmKeyHint.ActionText");

    m_pCancelKeyHint = Find_Element(L"CancelKeyHint");
    m_pCancelActionText = Find_ElementAs<CUIText>(L"CancelKeyHint.ActionText");

    if (m_pConfirmTitle == nullptr || m_pGoodsText == nullptr || m_pConfirmText == nullptr)
        return E_FAIL;

    if (m_pPriceRoot == nullptr || m_pPriceText == nullptr || m_pGoodsPrice == nullptr || m_pErgoCount == nullptr)
        return E_FAIL;

    if (m_pConfirmKeyHint == nullptr || m_pConfirmActionText == nullptr || m_pCancelKeyHint == nullptr || m_pCancelActionText == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUIConfirmationWindowController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Resolve_Confirmation(false);
    }
    else if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        Resolve_Confirmation(true);
    }
}

void CUIConfirmationWindowController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (bShouldOpen)
    {
        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pViewModel->Close_Confirmation();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_OVERLAY)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pViewModel->Close_Confirmation();
                return;
            }

            // 실제 UI와 Modal Input Owner가 등록된 뒤 Open 알파 애니메이션을 시작합니다.
            if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
                return;

            if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
                return;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    Apply_Contents();
    Apply_InputHints();

    m_pViewModel->Clear_Dirty();
}

void CUIConfirmationWindowController::Apply_Contents()
{
    const CONFIRMATION_REQUEST_DESC& RequestDesc = m_pViewModel->Get_RequestDesc();

    m_pConfirmTitle->Set_Text(RequestDesc.strTitle);
    m_pGoodsText->Set_Text(RequestDesc.strGoodsName);
    m_pConfirmText->Set_Text(RequestDesc.strConfirmText);

    m_pPriceRoot->Set_Visible(RequestDesc.bShowPrice);

    if (RequestDesc.bShowPrice)
    {
        m_pPriceText->Set_Text(RequestDesc.strPriceLabel);
        m_pGoodsPrice->Set_Text(Format_Number(RequestDesc.iPrice));
        m_pErgoCount->Set_Text(Format_Number(RequestDesc.iCurrentErgo));
    }
}

void CUIConfirmationWindowController::Apply_InputHints()
{
    const CONFIRMATION_REQUEST_DESC& RequestDesc = m_pViewModel->Get_RequestDesc();

    m_pConfirmKeyHint->Set_Visible(true);
    m_pCancelKeyHint->Set_Visible(true);

    m_pConfirmActionText->Set_Text(RequestDesc.strConfirmActionText);
    m_pCancelActionText->Set_Text(RequestDesc.strCancelActionText);
}

void CUIConfirmationWindowController::Resolve_Confirmation(_bool bConfirmed)
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
        return;

    // 뷰모델의 요청 데이터가 초기화되기 전에 결과를 먼저 복사한다.
    CONFIRMATION_RESULT_DESC ResultDesc = m_pViewModel->Make_Result(bConfirmed);

    m_pViewModel->Close_Confirmation();

    // 실제 구매와 강화 처리는 요청한 컨트롤러가 결과 이벤트를 받아 수행한다.
    m_pGameInstance->Broadcast(EVENT_TYPE_UI::CONFIRMATION_RESULT, &ResultDesc);
}

_wstring CUIConfirmationWindowController::Format_Number(_uint iValue) const
{
    _wstring strValue = to_wstring(iValue);

    // 뒤에서부터 세 자리마다 쉼표를 추가한다.
    for (_int iPosition = static_cast<_int>(strValue.length()) - 3; iPosition > 0; iPosition -= 3)
        strValue.insert(static_cast<size_t>(iPosition), 1, L',');

    return strValue;
}

CUIConfirmationWindowController* CUIConfirmationWindowController::Create()
{
    return new CUIConfirmationWindowController;
}

void CUIConfirmationWindowController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    Safe_Release(m_pViewModel);

    __super::Free();
}