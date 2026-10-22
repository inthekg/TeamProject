#include "UIStargazerTravelController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "StargazerTravelData.h"
#include "StargazerTravelViewModel.h"
#include "ConfirmationWindowData.h"


HRESULT CUIStargazerTravelController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CStargazerTravelViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    // 별바라기 기본 화면이 텔레포트 UI 열기를 요청하는 이벤트를 구독한다.
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_REQUEST, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::CONFIRMATION_RESULT, this);

    return S_OK;
}

void CUIStargazerTravelController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_REQUEST != eType || nullptr == pData)
        return;

    if (nullptr == m_pViewModel || true == m_pViewModel->Is_Visible())
        return;

    if (false == m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;

    STARGAZER_UI_OPEN_DESC* pOpenDesc = static_cast<STARGAZER_UI_OPEN_DESC*>(pData);

    // Broadcast 데이터는 호출 중에만 유효하므로 ViewModel 내부로 복사한다.
    m_pViewModel->Open_Travel(*pOpenDesc);
}

void CUIStargazerTravelController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_UI::CONFIRMATION_RESULT || pData == nullptr || m_pViewModel == nullptr)
        return;

    const CONFIRMATION_RESULT_DESC* pResultDesc = static_cast<const CONFIRMATION_RESULT_DESC*>(pData);

    if (pResultDesc->eActionType != CONFIRMATION_ACTION_TYPE::STARGAZER_TRAVEL)
        return;

    if (!pResultDesc->bConfirmed || pResultDesc->iTargetStargazerID == static_cast<_uint64>(-1))
        return;

    STARGAZER_TRAVEL_EXECUTE_DESC TravelDesc{};
    TravelDesc.iTargetStargazerID = pResultDesc->iTargetStargazerID;

    // 별바라기 화면을 닫도록 요청한 뒤 즉시 실제 이동 이벤트를 보낸다.
    m_pViewModel->Close_Travel();
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE, &TravelDesc);
}

HRESULT CUIStargazerTravelController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pLocationPreviewImage = Find_ElementAs<CUIImage>(L"LocationPreviewImage");
    m_pPrimaryActionText = Find_ElementAs<CUIText>(L"PrimaryInputHint.ActionText");

    m_pChapterButtons[0] = Find_ElementAs<CUIButton>(L"ChapterTab0Button");
    m_pChapterButtons[1] = Find_ElementAs<CUIButton>(L"ChapterTab1Button");
    m_pChapterButtons[2] = Find_ElementAs<CUIButton>(L"ChapterTab2Button");

    m_pHotelShortcutGroup = Find_Element(L"HotelShortcutGroup");
    m_pHotelShortcutButton = Find_ElementAs<CUIButton>(L"HotelShortcutButton");

    if (nullptr == m_pHotelShortcutGroup || nullptr == m_pHotelShortcutButton)
        return E_FAIL;

    if (m_pChapterButtons[0] == nullptr || m_pChapterButtons[1] == nullptr || m_pChapterButtons[2] == nullptr)
        return E_FAIL;

    if (m_pLocationPreviewImage == nullptr || m_pPrimaryActionText == nullptr)
        return E_FAIL;

    for (_uint iSlotIndex = 0; iSlotIndex < 3; ++iSlotIndex)
    {
        const _wstring strInstanceTag = L"LocationOption" + to_wstring(iSlotIndex);

        m_pLocationOptions[iSlotIndex] = Find_Element(strInstanceTag);
        m_pLocationButtons[iSlotIndex] = Find_ElementAs<CUIButton>(strInstanceTag + L".LocationButton");
        m_pLocationLabels[iSlotIndex] = Find_ElementAs<CUIText>(strInstanceTag + L".LocationLabel");
        m_pCurrentLocationIcons[iSlotIndex] = Find_ElementAs<CUIImage>(strInstanceTag + L".CurrentLocationIcon");

        if (m_pLocationOptions[iSlotIndex] == nullptr || m_pLocationButtons[iSlotIndex] == nullptr || m_pLocationLabels[iSlotIndex] == nullptr || m_pCurrentLocationIcons[iSlotIndex] == nullptr)
            return E_FAIL;
    }

    return S_OK;
}

void CUIStargazerTravelController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Close_Travel();
        return;
    }

    if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        Execute_CurrentSelection();
        return;
    }
}

void CUIStargazerTravelController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    _bool bOpenedThisFrame = false;
    _bool bPreviewTextureChanged = false;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    const _bool bHotelSelected = m_pViewModel->Is_HotelShortcutSelected();

    m_pHotelShortcutGroup->Set_Visible(true);
    m_pHotelShortcutButton->Set_Interactable(m_pViewModel->Is_HotelShortcutAvailable());
    m_pHotelShortcutButton->Set_Selected(bHotelSelected);

    if (bShouldOpen)
    {
        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pViewModel->Close_Travel();
                return;
            }

            // 별바라기 기본 Screen 위에 텔레포트 화면을 Modal Input Owner로 등록한다.
            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pViewModel->Close_Travel();
                return;
            }

            // 이번 Apply에서 처음 열린 상태임을 기록한다.
            bOpenedThisFrame = true;
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

    // 활성화된 Chapter 개수만큼 탭을 표시한다.
    const _uint iVisibleChapterCount = m_pViewModel->Get_VisibleChapterCount();
    const _uint iSelectedChapterSlot = m_pViewModel->Get_SelectedChapterSlot();

    for (_uint iSlotIndex = 0; iSlotIndex < 3; ++iSlotIndex)
    {
        const _bool bVisible = iSlotIndex < iVisibleChapterCount;

        m_pChapterButtons[iSlotIndex]->Set_Visible(bVisible);
        m_pChapterButtons[iSlotIndex]->Set_Interactable(bVisible);
        m_pChapterButtons[iSlotIndex]->Set_Selected(bVisible && iSlotIndex == iSelectedChapterSlot);
    }

    // 선택한 Chapter에 속한 활성화 지역만 표시한다.
    const _uint iVisibleLocationCount = m_pViewModel->Get_VisibleLocationCount();
    const _uint iSelectedLocationSlot = m_pViewModel->Get_SelectedLocationSlot();

    for (_uint iSlotIndex = 0; iSlotIndex < 3; ++iSlotIndex)
    {
        const _bool bVisible = iSlotIndex < iVisibleLocationCount;

        m_pLocationOptions[iSlotIndex]->Set_Visible(bVisible);
        m_pLocationButtons[iSlotIndex]->Set_Interactable(bVisible);
        m_pLocationButtons[iSlotIndex]->Set_Selected(bVisible && !bHotelSelected && iSlotIndex == iSelectedLocationSlot);
        m_pCurrentLocationIcons[iSlotIndex]->Set_Visible(bVisible && m_pViewModel->Is_CurrentLocation(iSlotIndex));

        if (bVisible)
            m_pLocationLabels[iSlotIndex]->Set_Text(m_pViewModel->Get_LocationText(iSlotIndex));
    }

    const string& strPreviewTexturePath = m_pViewModel->Get_PreviewTexturePath();
    const _bool bHasPreviewTexture = !strPreviewTexturePath.empty();

    m_pLocationPreviewImage->Set_Visible(bHasPreviewTexture);

    if (bHasPreviewTexture && m_strAppliedPreviewTexturePath != strPreviewTexturePath)
    {
        if (SUCCEEDED(m_pGameInstance->Set_UIImageTexture(m_hUI, L"LocationPreviewImage", strPreviewTexturePath)))
        {
            m_strAppliedPreviewTexturePath = strPreviewTexturePath;
            bPreviewTextureChanged = true;
        }
    }

    if (m_pUIObject != nullptr)
    {
        if (bOpenedThisFrame)
        {
            // 최초 진입은 전체 Open 애니메이션이 프리뷰 Reveal까지 담당한다.
            if (m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
        else if (bPreviewTextureChanged)
        {
            // 지역이나 호텔 선택으로 프리뷰 텍스처가 바뀌었을 때만 방사형 Reveal을 재생한다.
            if (m_pUIObject->Has_Animation("SelectLocation"))
                m_pUIObject->Play_Animation("SelectLocation", true);
        }
    }

    m_pViewModel->Clear_Dirty();
}

void CUIStargazerTravelController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI || !m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "SelectStargazerChapter")
    {
        if (Event.strRuntimeElementTag == L"ChapterTab0Button")
            m_pViewModel->Select_ChapterBySlot(0);
        else if (Event.strRuntimeElementTag == L"ChapterTab1Button")
            m_pViewModel->Select_ChapterBySlot(1);
        else if (Event.strRuntimeElementTag == L"ChapterTab2Button")
            m_pViewModel->Select_ChapterBySlot(2);

        return;
    }

    if (Event.strCommandKey == "SelectStargazerLocation")
    {
        if (Event.strRuntimeElementTag == L"LocationOption0.LocationButton")
            m_pViewModel->Select_LocationBySlot(0);
        else if (Event.strRuntimeElementTag == L"LocationOption1.LocationButton")
            m_pViewModel->Select_LocationBySlot(1);
        else if (Event.strRuntimeElementTag == L"LocationOption2.LocationButton")
            m_pViewModel->Select_LocationBySlot(2);

        return;
    }

    if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"PrimaryInputHint.HintButtonSpace")
            Execute_CurrentSelection();
        else if (Event.strRuntimeElementTag == L"CloseInputHint.HintButton")
            Close_Travel();

        return;
    }

    if (Event.strCommandKey == "TravelToHotel")
    {
        // 우측 프리뷰를 호텔 이미지로 전환한 후 확인창을 연다.
        m_pViewModel->Select_HotelShortcut();
        return;
    }
}

void CUIStargazerTravelController::Execute_SelectedTravel()
{
    if (m_pViewModel == nullptr)
        return;

    const STARGAZER_TRAVEL_ENTRY* pSelectedEntry = m_pViewModel->Get_SelectedEntry();

    if (pSelectedEntry == nullptr)
        return;

    CONFIRMATION_REQUEST_DESC RequestDesc{};

    RequestDesc.eActionType = CONFIRMATION_ACTION_TYPE::STARGAZER_TRAVEL;
    RequestDesc.strTitle = L"이동 확인";
    RequestDesc.strGoodsName = pSelectedEntry->strLocationName;
    RequestDesc.strConfirmText = L"선택한 별바라기로 이동하시겠습니까?";
    RequestDesc.strConfirmActionText = L"이동";
    RequestDesc.strCancelActionText = L"취소";
    RequestDesc.bShowPrice = false;
    RequestDesc.iTargetStargazerID = pSelectedEntry->iStargazerID;

    m_pGameInstance->Broadcast(EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST, &RequestDesc);
}

void CUIStargazerTravelController::Execute_HotelTravel()
{
    if (m_pViewModel == nullptr)
        return;

    const _uint64 iHotelStargazerID = m_pViewModel->Get_HotelStargazerID();

    if (iHotelStargazerID == static_cast<_uint64>(-1))
        return;

    CONFIRMATION_REQUEST_DESC RequestDesc{};

    RequestDesc.eActionType = CONFIRMATION_ACTION_TYPE::STARGAZER_TRAVEL;
    RequestDesc.strTitle = L"이동 확인";
    RequestDesc.strGoodsName = L"크라트 호텔";
    RequestDesc.strConfirmText = L"크라트 호텔로 이동하시겠습니까?";
    RequestDesc.strConfirmActionText = L"이동";
    RequestDesc.strCancelActionText = L"취소";
    RequestDesc.bShowPrice = false;
    RequestDesc.iTargetStargazerID = iHotelStargazerID;

    m_pGameInstance->Broadcast(EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST, &RequestDesc);
}

void CUIStargazerTravelController::Execute_CurrentSelection()
{
    if (m_pViewModel == nullptr)
        return;

    // 호텔 고정 버튼이 선택되어 있으면 호텔 확인창을 연다.
    if (m_pViewModel->Is_HotelShortcutSelected())
    {
        Execute_HotelTravel();
        return;
    }

    // 일반 지역이 선택되어 있으면 해당 지역 확인창을 연다.
    Execute_SelectedTravel();
}

void CUIStargazerTravelController::Close_Travel()
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Close_Travel();
}

CUIStargazerTravelController* CUIStargazerTravelController::Create()
{
    return new CUIStargazerTravelController;
}

void CUIStargazerTravelController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    Safe_Release(m_pViewModel);

    m_pLocationPreviewImage = nullptr;
    m_pPrimaryActionText = nullptr;

    for (_uint iSlotIndex = 0; iSlotIndex < 3; ++iSlotIndex)
    {
        m_pLocationOptions[iSlotIndex] = nullptr;
        m_pLocationButtons[iSlotIndex] = nullptr;
        m_pLocationLabels[iSlotIndex] = nullptr;
        m_pCurrentLocationIcons[iSlotIndex] = nullptr;
    }

    __super::Free();
}