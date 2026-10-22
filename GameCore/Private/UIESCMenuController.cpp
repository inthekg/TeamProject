#include "UIESCMenuController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIButton.h"
#include "UIText.h"
#include "ESCMenuViewModel.h"

HRESULT CUIESCMenuController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CESCMenuViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUIESCMenuController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    // 현재 최상단 Input Owner가 아닌 UI의 Command는 처리하지 않는다.
    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "OpenEquipment")
    {
        m_pGameInstance->Broadcast(EVENT_TYPE_UI::EQUIPMENT_OPEN_REQUEST, nullptr);
    }
    else if (Event.strCommandKey == "OpenInventory")
    {
        m_pViewModel->Set_Category(ESCAPE_MENU_CATEGORY::INVENTORY);
    }
    else if (Event.strCommandKey == "OpenCharacterInfo")
    {
        m_pViewModel->Set_Category(ESCAPE_MENU_CATEGORY::CHARACTER_INFO);
    }
    else if (Event.strCommandKey == "OpenSettings")
    {
        m_pViewModel->Set_Category(ESCAPE_MENU_CATEGORY::SETTINGS);
    }
}

void CUIESCMenuController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    if (m_pViewModel == nullptr)
        return;

    if (Event.hUI != m_hUI)
        return;

    // 포커스를 잃었고 새 포커스가 없다면 설명을 숨긴다.
    if (!Event.bFocused)
    {
        m_pViewModel->Clear_Description();
        return;
    }

    if (Event.strFocusGroup == "EscapeMain")
    {
        Apply_MainMenuDescription(Event.strCommandKey);
        return;
    }

    if (Event.strFocusGroup == "EscapeItemSlot")
    {
        Apply_ItemSlotDescription(Event.strRuntimeElementTag);

        return;
    }

    m_pViewModel->Clear_Description();
}

HRESULT CUIESCMenuController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pMenuContent = Find_Element(L"MenuContent");
    m_pEquipmentButton = Find_ElementAs<CUIButton>(L"BtnEquipment");
    m_pInventoryButton = Find_ElementAs<CUIButton>(L"BtnInventory");
    m_pCharacterInfoButton = Find_ElementAs<CUIButton>(L"BtnCharacterInfo");
    m_pSettingsButton = Find_ElementAs<CUIButton>(L"BtnSettings");
    m_pTopInfoGroup = Find_Element(L"TopInfoGroup");
    m_pTopInfoTitle = Find_ElementAs<CUIText>(L"TopInfoTitle");
    m_pTopInfoDescription = Find_ElementAs<CUIText>(L"TopInfoDescription");
    m_pBottomInfoGroup = Find_Element(L"BottomInfoGroup");
    m_pBottomInfoTitle = Find_ElementAs<CUIText>(L"BottomInfoTitle");
    m_pBottomPrimaryActionGroup = Find_Element(L"BottomPrimaryActionGroup");
    m_pBottomPrimaryActionText = Find_ElementAs<CUIText>(L"BottomPrimaryActionText");
    m_pBottomSecondaryActionGroup = Find_Element(L"BottomSecondaryActionGroup");
    m_pBottomSecondaryActionText = Find_ElementAs<CUIText>(L"BottomSecondaryActionText");

    if (m_pMenuContent == nullptr ||
        m_pEquipmentButton == nullptr ||
        m_pInventoryButton == nullptr ||
        m_pCharacterInfoButton == nullptr ||
        m_pSettingsButton == nullptr ||
        m_pTopInfoGroup == nullptr ||
        m_pTopInfoTitle == nullptr ||
        m_pTopInfoDescription == nullptr ||
        m_pBottomInfoGroup == nullptr ||
        m_pBottomInfoTitle == nullptr ||
        m_pBottomPrimaryActionGroup == nullptr ||
        m_pBottomPrimaryActionText == nullptr ||
        m_pBottomSecondaryActionGroup == nullptr ||
        m_pBottomSecondaryActionText == nullptr)
    {
        return E_FAIL;
    }

    return S_OK;
}

void CUIESCMenuController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    if (m_bCloseAnimationPending)
    {
        // Close 애니메이션이 끝난 프레임에 실제 UI를 닫습니다.
        if (m_pUIObject->Is_AnimationFinished())
            Finalize_Close();

        return;
    }

    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (m_pViewModel->Is_Visible() != bActuallyOpen)
        m_pViewModel->Set_Visible(bActuallyOpen);

    if (bActuallyOpen)
    {
        // 열린 ESC 메뉴는 자신이 최상단 Input Owner일 때만 ESC 입력을 처리한다.
        if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
            return;

        if (m_pGameInstance->Key_Down(DIK_ESCAPE))
            Begin_CloseAnimation();

        return;
    }

    // 다른 Screen 또는 Modal이 입력을 소유 중이면 ESC 메뉴를 새로 열지 않는다.
    if (m_pGameInstance->Has_UIInputOwner())
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
        m_pViewModel->Set_Visible(true);
}

void CUIESCMenuController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (bShouldOpen)
    {
        if (!m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Acquire_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = true;
        }

        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                // 메뉴 열기에 실패했으면 HUD 억제를 되돌린다.
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                return;
            }

            // 실제로 열린 ESC 메뉴를 Screen Input Owner로 등록한다.
            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::SCREEN)))
            {
                // Input Owner 등록 실패 시 열린 UI와 HUD 억제를 모두 원상복구한다.
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                return;
            }

            m_pGameInstance->Play_Sound("SE_UI_OpenMenu_04", SOUND_SFX, 0.75f, false, false);

            // 실제 UI와 Input Owner 등록이 완료된 뒤 Open 애니메이션을 시작
            if (m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            // 최상단 Screen만 Pop할 수 있으므로 먼저 Input Owner 제거를 예약한다.
            if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
                return;

            if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
                return;
        }

        if (m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Release_UILayerSuppression("HUD");

            m_bHUDLayerSuppressed = false;
        }
    }

    Apply_Description();
    m_pViewModel->Clear_Dirty();
}

void CUIESCMenuController::Apply_MainMenuDescription(const string& strCommandKey)
{
    if (strCommandKey == "OpenEquipment")
    {
        m_pViewModel->Set_MainDescription(L"장비", L"장착하는 장비를 변경할 수 있습니다.");
    }
    else if (strCommandKey == "OpenInventory")
    {
        m_pViewModel->Set_MainDescription(L"가방", L"보유한 아이템을 관리할 수 있습니다.");
    }
    else if (strCommandKey == "OpenCharacterInfo")
    {
        m_pViewModel->Set_MainDescription(L"캐릭터 정보", L"상태를 확인할 수 있습니다.");
    }
    else if (strCommandKey == "OpenSettings")
    {
        m_pViewModel->Set_MainDescription(L"설정", L"게임 내 설정을 변경할 수 있습니다.");
    }
    else
    {
        m_pViewModel->Clear_Description();
    }
}

void CUIESCMenuController::Apply_ItemSlotDescription(const _wstring& strRuntimeElementTag)
{/*
     * 위 벨트 첫 번째 슬롯
     * 펄스전지 개수는 나중에 플레이어 데이터와 연결한다.
     */
    if (strRuntimeElementTag == L"UpperSlot0.SlotButton")
    {
        m_pViewModel->Set_ItemSlotDescription(L"펄스전지", L"");
        return;
    }

    // 아래 벨트 첫 번째 슬롯
    if (strRuntimeElementTag == L"LowerSlot0.SlotButton")
    {
        m_pViewModel->Set_ItemSlotDescription(L"그라인더", L"");
        return;
    }

    // 아래 벨트 두 번째 슬롯
    if (strRuntimeElementTag == L"LowerSlot1.SlotButton")
    {
        m_pViewModel->Set_ItemSlotDescription(L"모나드의 등불", L"");
        return;
    }

    // 보조 가방 첫 번째 슬롯
    if (strRuntimeElementTag == L"PouchSlot0.SlotButton")
    {
        m_pViewModel->Set_ItemSlotDescription(L"문페이즈 회중시계", L"");
        return;
    }

    // 보조 가방 세 번째 슬롯
    if (strRuntimeElementTag == L"PouchSlot2.SlotButton")
    {
        m_pViewModel->Set_ItemSlotDescription(L"최후의 수단", L"");
        return;
    }

    /*
     * 위의 고정 아이템 슬롯이 아니면
     * 현재 ESC 메뉴에서는 모두 빈 슬롯으로 처리한다.
     */
    m_pViewModel->Set_EmptySlotDescription();
}

void CUIESCMenuController::Apply_Description()
{
    if (m_pViewModel == nullptr)
        return;

    const ESCAPE_DESCRIPTION_DATA& Description = m_pViewModel->Get_Description();

    const _bool bShowTopInfo = Description.eMode == ESCAPE_DESCRIPTION_MODE::MAIN_MENU;
    const _bool bShowBottomInfo = Description.eMode == ESCAPE_DESCRIPTION_MODE::ITEM_SLOT;


    // 위 버튼 포커스면 위 설명창만 슬롯 포커스면 아래 설명창만 보인다.
    m_pTopInfoGroup->Set_Visible(bShowTopInfo);
    m_pBottomInfoGroup->Set_Visible(bShowBottomInfo);

    if (bShowTopInfo)
    {
        m_pTopInfoTitle->Set_Text(Description.strTitle);
        m_pTopInfoDescription->Set_Text(Description.strDescription);

        return;
    }

    if (!bShowBottomInfo)
        return;

    m_pBottomInfoTitle->Set_Text(Description.strTitle);
    m_pBottomPrimaryActionGroup->Set_Visible(Description.bShowPrimaryAction);
    m_pBottomPrimaryActionText->Set_Text(Description.strPrimaryAction);
    m_pBottomSecondaryActionGroup->Set_Visible(Description.bShowSecondaryAction);
    m_pBottomSecondaryActionText->Set_Text(Description.strSecondaryAction);

    if (Description.bShowSecondaryAction)
    {
        /*
         * 아이템 슬롯
         *
         * [SPACE 변경] [R 장착 해제]
         */
        m_pBottomPrimaryActionGroup->Set_Position({ -103.f, 12.f });

        // RGBA 0~255 기준: 241, 60, 60, 255
        m_pBottomInfoTitle->Set_Color(
            {
                241.f / 255.f,
                60.f / 255.f,
                60.f / 255.f,
                1.f
            });
    }
    else
    {
        /*
         * 빈 슬롯
         *
         * Secondary가 없으므로
         * [SPACE 장착] 그룹을 오른쪽으로 옮긴다.
         */
        m_pBottomPrimaryActionGroup->Set_Position({ 0.f, 12.f });

        // RGBA 0~255 기준: 194, 194, 194, 255
        m_pBottomInfoTitle->Set_Color(
            {
                194.f / 255.f,
                194.f / 255.f,
                194.f / 255.f,
                1.f
            });
    }
}

void CUIESCMenuController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    m_pGameInstance->Play_Sound("SE_UI_CloseMenu_05", SOUND_SFX, 0.75f, false, false);

    // 애니메이션이 있으면 UI를 유지한 상태로 종료를 기다립니다.
    if (m_pUIObject->Has_Animation("Close"))
    {
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    // Close 클립이 없으면 즉시 닫습니다.
    Finalize_Close();
}

void CUIESCMenuController::Finalize_Close()
{
    if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
        return;

    if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
        return;

    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    m_bCloseAnimationPending = false;
    m_pViewModel->Set_Visible(false);
    m_pViewModel->Clear_Dirty();
}

CUIESCMenuController* CUIESCMenuController::Create()
{
    return new CUIESCMenuController;
}

void CUIESCMenuController::Free()
{
    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    Safe_Release(m_pViewModel);

    m_pBottomInfoGroup = nullptr;
    m_pBottomInfoTitle = nullptr;

    m_pBottomPrimaryActionGroup = nullptr;
    m_pBottomPrimaryActionText = nullptr;

    m_pBottomSecondaryActionGroup = nullptr;
    m_pBottomSecondaryActionText = nullptr;

    m_pTopInfoGroup = nullptr;
    m_pTopInfoTitle = nullptr;
    m_pTopInfoDescription = nullptr;

    m_pMenuContent = nullptr;

    m_pEquipmentButton = nullptr;
    m_pInventoryButton = nullptr;
    m_pCharacterInfoButton = nullptr;
    m_pSettingsButton = nullptr;

    __super::Free();
}
