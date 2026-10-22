#include "UIEquipmentController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EquipmentViewModel.h"
#include "Layer.h"
#include "EquipmentPreviewCharacter.h"
#include "EquipmentPreviewBackground.h"
#include "Player.h"
#include "Inventory.h"
#include "ItemTable.h"

HRESULT CUIEquipmentController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CEquipmentViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    // ESC 메뉴가 보내는 장비창 열기 요청을 받기
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::EQUIPMENT_OPEN_REQUEST, this);

    if (FAILED(Apply_StaticPresentation()))
        return E_FAIL;

    return S_OK;
}

void CUIEquipmentController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pViewModel->Is_CostumeSelectVisible())
    {
        if (Event.strCommandKey == "PrevCostumeCategory")
        {
            m_pViewModel->Move_CostumeCategory(1);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "NextCostumeCategory")
        {
            m_pViewModel->Move_CostumeCategory(-1);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "AccessoriesCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::ACCESSORY);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "MaskCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::MASK);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "CostumeCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::BODY);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "SelectCostumeSlot")
        {
            if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot0.SlotButton")
                Select_CostumeSlot(0);
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot1.SlotButton")
                Select_CostumeSlot(1);
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot2.SlotButton")
                Select_CostumeSlot(2);
        }
        else if (Event.strCommandKey == "ActivateInputHint")
        {
            if (Event.strRuntimeElementTag == L"CostumeSelectPage.BackHint.HintButton")
                Close_CostumeSelect();
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.EquipHint.HintButtonSpace")
                Equip_SelectedCostume();
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.UnequipHint.HintButton")
                Unequip_SelectedCostume();
        }

        return;
    }

    if (Event.strCommandKey == "ShowEquipmentPage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::EQUIPMENT);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ShowUseItemPage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::USE_ITEM);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ShowCostumePage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::COSTUME);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "PrevEquipmentCategory")
    {
        m_pViewModel->Move_Category(-1);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "NextEquipmentCategory")
    {
        m_pViewModel->Move_Category(1);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"CloseHint.HintButton" || Event.strRuntimeElementTag == L"CompactCloseHint.HintButton")
            Close_Equipment();
    }
    else if (Event.strCommandKey == "EquipmentItemSlot")
    {
        if (Event.strRuntimeElementTag == L"WeaponSlotPrimary.SlotButton" ||
            Event.strRuntimeElementTag == L"WeaponSlotSecondary.SlotButton" ||
            Event.strRuntimeElementTag == L"UpperBeltSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"LowerBeltSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"LowerBeltSlot2.SlotButton" ||
            Event.strRuntimeElementTag == L"PouchSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"PouchSlot3.SlotButton")
        {
            Select_MainDetailSlot(Event.strRuntimeElementTag);
        }
        else if (Event.strRuntimeElementTag == L"LegionArmSlot.SlotButton")
        {
            // 리전 암 메인 슬롯은 선택 시 기존 리전 암 교체 화면을 연다.
            m_pGameInstance->Broadcast(EVENT_TYPE_UI::LEGION_ARM_CHANGE_OPEN_REQUEST, nullptr);
        }
        else if (Event.strRuntimeElementTag == L"AccessoryCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::ACCESSORY);
        }
        else if (Event.strRuntimeElementTag == L"HeadCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::MASK);
        }
        else if (Event.strRuntimeElementTag == L"BodyCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::BODY);
        }
    }
}

void CUIEquipmentController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    UNREFERENCED_PARAMETER(pData);

    if (eType != EVENT_TYPE_UI::EQUIPMENT_OPEN_REQUEST)
        return;

    if (m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    // ESC Screen 위에 장비창을 Modal Input Owner로 올린다.
    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;
    if (FAILED(Bind_PlayerInventory()))
        return;

    m_pViewModel->Set_CurrentErgo(m_pInventory->Get_Ergo());

    m_bCloseAfterCameraRestore = false;
    m_bWasTopInputOwner = false;

    m_pViewModel->Open_Equipment();


    Refresh_MainEquipmentSlots();
    Refresh_MainCostumeSlotIcons();
    Select_DefaultSlotForCurrentCategory();
}

void CUIEquipmentController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (Event.strFocusGroup != "EquipmentItemSlot")
        return;
}

HRESULT CUIEquipmentController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pPages[0] = Find_Element(L"EquipmentPage");
    m_pPages[1] = Find_Element(L"UseItemPage");
    m_pPages[2] = Find_Element(L"CostumePage");

    m_pCategoryButtons[0] = Find_ElementAs<CUIButton>(L"CategoryEquipmentButton");
    m_pCategoryButtons[1] = Find_ElementAs<CUIButton>(L"CategoryUseItemButton");
    m_pCategoryButtons[2] = Find_ElementAs<CUIButton>(L"CategoryCostumeButton");

    m_pCategoryTabGroup = Find_Element(L"CategoryTabGroup");

    m_pDetailPanel = Find_Element(L"DetailPanel");
    m_pInputHintActionGroup = Find_Element(L"InputHintActionGroup");
    m_pInputHintCompactGroup = Find_Element(L"InputHintCompactGroup");
    m_pCharacterPreviewArea = Find_Element(L"CharacterPreviewArea");

    m_pDetailTitle = Find_ElementAs<CUIText>(L"DetailTitle");
    m_pDetailIcon = Find_ElementAs<CUIImage>(L"DetailIcon");
    m_pDetailCategory = Find_ElementAs<CUIText>(L"DetailCategory");
    m_pDetailBodyText = Find_ElementAs<CUIText>(L"DetailBodyText");

    m_pDetailText = Find_Element(L"DetailText");
    m_pWeaponType1 = Find_Element(L"WeaponType1");
    m_pWeaponType2 = Find_Element(L"WeaponType2");
    m_pWeaponTypeDetailText = Find_Element(L"WeaponTypeDetailText");

    m_pDetailIconWeapon = Find_ElementAs<CUIImage>(L"DetailIcon_Weapon");
    m_pDetailIconGrip = Find_ElementAs<CUIImage>(L"DetailIcon_Grip");
    m_pDetailIconWeapon2 = Find_ElementAs<CUIImage>(L"DetailIcon_Weapon2");

    m_pWeaponTitleText = Find_ElementAs<CUIText>(L"WeaponTitleText");
    m_pWeaponBodyText = Find_ElementAs<CUIText>(L"WeaponBodyText");

    m_pCostumeSelectPage = Find_Element(L"CostumeSelectPage");
    m_pCostumePreviewArea = Find_Element(L"CostumeSelectPage.CostumePreviewArea");

    m_pCostumeCategoryButtons[0] = Find_ElementAs<CUIButton>(L"CostumeSelectPage.AccessoriesButton");
    m_pCostumeCategoryButtons[1] = Find_ElementAs<CUIButton>(L"CostumeSelectPage.MaskButton");
    m_pCostumeCategoryButtons[2] = Find_ElementAs<CUIButton>(L"CostumeSelectPage.CostumeButton");

    m_pCostumeDetailPanel = Find_Element(L"CostumeSelectPage.DetailPanel");
    m_pCostumeDetailTitle = Find_ElementAs<CUIText>(L"CostumeSelectPage.DetailTitle");
    m_pCostumeDetailIcon = Find_ElementAs<CUIImage>(L"CostumeSelectPage.DetailIcon");
    m_pCostumeDetailCategory = Find_ElementAs<CUIText>(L"CostumeSelectPage.DetailCategory");
    m_pCostumeDetailBodyText = Find_ElementAs<CUIText>(L"CostumeSelectPage.DetailBodyText");
    m_pCostumeCategoryText = Find_ElementAs<CUIText>(L"CostumeSelectPage.CostumeCategoryText");

    m_pCostumeBackHint = Find_Element(L"CostumeSelectPage.BackHint");
    m_pCostumeEquipHint = Find_Element(L"CostumeSelectPage.EquipHint");
    m_pCostumeUnequipHint = Find_Element(L"CostumeSelectPage.UnequipHint");

    m_pErgoProfile = Find_Element(L"ErgoProfile");
    m_pErgoCountText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoCount");
    m_pErgoGetText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoGet");

    m_pSecondaryWeaponButton = Find_ElementAs<CUIButton>(L"WeaponSlotSecondary.SlotButton");
    m_pSecondaryWeaponIcon = Find_ElementAs<CUIImage>(L"WeaponSlotSecondary.ItemIcon");

    m_pLegionArmButton = Find_ElementAs<CUIButton>(L"LegionArmSlot.SlotButton");
    m_pLegionArmIcon = Find_ElementAs<CUIImage>(L"LegionArmSlot.ItemIcon");

    if (m_pSecondaryWeaponButton == nullptr || m_pSecondaryWeaponIcon == nullptr || m_pLegionArmButton == nullptr || m_pLegionArmIcon == nullptr)
        return E_FAIL;

    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return E_FAIL;

    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        const _wstring strSlotTag = L"CostumeSelectPage.CostumeSlot" + to_wstring(iIndex);

        m_pCostumeSlotRoots[iIndex] = Find_Element(strSlotTag);
        m_pCostumeSlotButtons[iIndex] = Find_ElementAs<CUIButton>(strSlotTag + L".SlotButton");
        m_pCostumeSlotIcons[iIndex] = Find_ElementAs<CUIImage>(strSlotTag + L".CostumeIcon");
        m_pCostumeEquippedIcons[iIndex] = Find_ElementAs<CUIImage>(strSlotTag + L".EquippedIcon");
    }

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        if (m_pPages[iIndex] == nullptr || m_pCategoryButtons[iIndex] == nullptr)
            return E_FAIL;

        if (m_pCostumeCategoryButtons[iIndex] == nullptr || m_pCostumeSlotRoots[iIndex] == nullptr || m_pCostumeSlotButtons[iIndex] == nullptr || m_pCostumeSlotIcons[iIndex] == nullptr || m_pCostumeEquippedIcons[iIndex] == nullptr)
            return E_FAIL;
    }

    if (m_pCategoryTabGroup == nullptr || m_pDetailPanel == nullptr || m_pDetailText == nullptr || m_pWeaponType1 == nullptr || m_pWeaponType2 == nullptr || m_pWeaponTypeDetailText == nullptr || m_pDetailTitle == nullptr || m_pDetailIcon == nullptr || m_pDetailIconWeapon == nullptr || m_pDetailIconGrip == nullptr || m_pDetailIconWeapon2 == nullptr || m_pDetailCategory == nullptr || m_pDetailBodyText == nullptr || m_pWeaponTitleText == nullptr || m_pWeaponBodyText == nullptr || m_pInputHintActionGroup == nullptr || m_pInputHintCompactGroup == nullptr || m_pCharacterPreviewArea == nullptr)
        return E_FAIL;

    if (m_pCostumeSelectPage == nullptr || m_pCostumePreviewArea == nullptr || m_pCostumeDetailPanel == nullptr || m_pCostumeDetailTitle == nullptr || m_pCostumeDetailIcon == nullptr || m_pCostumeDetailCategory == nullptr || m_pCostumeDetailBodyText == nullptr || m_pCostumeCategoryText == nullptr || m_pCostumeBackHint == nullptr || m_pCostumeEquipHint == nullptr || m_pCostumeUnequipHint == nullptr)
        return E_FAIL;

    m_pLegionArmIcon->Set_Size(_float2(95.f, 95.f));

    return S_OK;
}

void CUIEquipmentController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
    {
        m_bPreviewDragging = false;
        return;
    }

    if (m_bCloseAfterCameraRestore)
    {
        m_bCloseAfterCameraRestore = false;
        m_pViewModel->Close_Equipment();
        return;
    }

    const _bool bIsTopInputOwner = m_pGameInstance->Is_TopUIInputOwner(m_hUI);

    if (!bIsTopInputOwner)
    {
        // 리전 암 교체 등 하위 Modal이 열린 동안에는 다음 복귀를 기다린다.
        m_bWasTopInputOwner = false;
        m_bPreviewDragging = false;
        return;
    }

    if (!m_bWasTopInputOwner)
    {
        // 하위 Modal이 닫힌 뒤 장비창으로 돌아온 첫 프레임에 실제 장착 정보를 다시 읽는다.
        Refresh_MainEquipmentSlots();
        Refresh_MainCostumeSlotIcons();
        m_bWasTopInputOwner = true;
    }

    if (m_bCostumeSelectOpening && m_pUIObject->Is_AnimationFinished())
    {
        // 기존 CostumePage의 퇴장이 끝나면 렌더 대상에서 제외합니다.
        m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(false);
        m_bCostumeSelectOpening = false;
    }

    Update_PreviewRotation();

    if (m_pViewModel->Is_CostumeSelectVisible())
    {
        if (m_pGameInstance->Key_Down(DIK_Q))
        {
            // 이전 코스튬 카테고리로 이동한다.
            m_pViewModel->Move_CostumeCategory(1);
            Refresh_CostumeSlots();
        }
        else if (m_pGameInstance->Key_Down(DIK_E))
        {
            // 다음 코스튬 카테고리로 이동한다.
            m_pViewModel->Move_CostumeCategory(-1);
            Refresh_CostumeSlots();
        }
        else if (m_pGameInstance->Key_Down(DIK_ESCAPE))
            Close_CostumeSelect();
        else if (m_pGameInstance->Key_Down(DIK_SPACE))
            Equip_SelectedCostume();
        else if (m_pGameInstance->Key_Down(DIK_R))
            Unequip_SelectedCostume();

        return;
    }

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Close_Equipment();
        m_bWasTopInputOwner = false;
    }
}

void CUIEquipmentController::Apply_ViewModel()
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
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Equipment();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Equipment();
                return;
            }

            // UI와 Input Owner가 모두 열린 뒤 프리뷰를 활성화
            Begin_EquipmentPreview();
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

        // 장비창이 닫힌 뒤 프리뷰 캐릭터와 카메라를 함께 종료
        End_EquipmentPreview();

        if (m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Release_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = false;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    Apply_ErgoProfile();
    const _uint iCurrentCategory = static_cast<_uint>(m_pViewModel->Get_Category());

    const _bool bCostumeSelectVisible = m_pViewModel->Is_CostumeSelectVisible();

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        const _bool bKeepCostumePageForAnimation = m_bCostumeSelectOpening && iIndex == static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME);
        const _bool bSelected = (!bCostumeSelectVisible && iIndex == iCurrentCategory) || bKeepCostumePageForAnimation;

        m_pPages[iIndex]->Set_Visible(bSelected);
        m_pCategoryButtons[iIndex]->Set_Selected(iIndex == iCurrentCategory);
    }

    Apply_CostumeSelectViewModel();

    if (bCostumeSelectVisible)
    {
        m_pDetailPanel->Set_Visible(false);
        m_pInputHintActionGroup->Set_Visible(false);
        m_pInputHintCompactGroup->Set_Visible(false);

        m_pViewModel->Clear_Dirty();
        return;
    }

    const EQUIPMENT_DETAIL_DATA& Detail = m_pViewModel->Get_Detail();

    const _bool bShowItem = Detail.eMode == EQUIPMENT_DETAIL_MODE::ITEM;
    const _bool bShowAssembledWeapon = Detail.eMode == EQUIPMENT_DETAIL_MODE::WEAPON_ASSEMBLED;
    const _bool bShowSingleWeapon = Detail.eMode == EQUIPMENT_DETAIL_MODE::WEAPON_SINGLE;
    const _bool bShowWeapon = bShowAssembledWeapon || bShowSingleWeapon;
    const _bool bShowDetail = bShowItem || bShowWeapon;

    // 상세 모드에 따라 각 이미지·텍스트 그룹을 배타적으로 표시한다.
    m_pDetailPanel->Set_Visible(bShowDetail);
    m_pDetailIcon->Set_Visible(bShowItem);
    m_pDetailText->Set_Visible(bShowItem);
    m_pWeaponType1->Set_Visible(bShowAssembledWeapon);
    m_pWeaponType2->Set_Visible(bShowSingleWeapon);
    m_pWeaponTypeDetailText->Set_Visible(bShowWeapon);

    m_pInputHintActionGroup->Set_Visible(bShowDetail);
    m_pInputHintCompactGroup->Set_Visible(!bShowDetail);

    if (bShowItem)
    {
        m_pDetailTitle->Set_Text(Detail.strTitle);
        m_pDetailCategory->Set_Text(Detail.strCategory);
        m_pDetailBodyText->Set_Text(Detail.strDescription);

        if (FAILED(Set_ImageTexture(L"DetailIcon", Detail.strIconTexturePath)))
            return;
    }
    else if (bShowWeapon)
    {
        m_pDetailTitle->Set_Text(Detail.strTitle);
        m_pWeaponTitleText->Set_Text(Detail.strWeaponTitle);
        m_pWeaponBodyText->Set_Text(Detail.strDescription);

        if (bShowAssembledWeapon)
        {
            if (FAILED(Set_ImageTexture(L"DetailIcon_Weapon", Detail.strIconTexturePath)))
                return;

            if (FAILED(Set_ImageTexture(L"DetailIcon_Grip", Detail.strSubIconTexturePath)))
                return;
        }
        else
        {
            if (FAILED(Set_ImageTexture(L"DetailIcon_Weapon2", Detail.strIconTexturePath)))
                return;
        }
    }

    m_pViewModel->Clear_Dirty();
}

void CUIEquipmentController::Close_Equipment()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible() || m_bCloseAfterCameraRestore)
        return;

    // 현재 UI 프레임에서는 장비 UI와 프리뷰 객체를 유지한 채, 다음 CameraManager::Late_Update부터 게임 카메라를 사용하도록 요청
    m_pGameInstance->End_UIPreviewCamera();

    // 다음 프레임의 카메라/파이프라인 갱신이 끝난 후 실제 장비창을 닫는다.
    m_bCloseAfterCameraRestore = true;
    m_bPreviewDragging = false;
}

HRESULT CUIEquipmentController::Bind_PlayerInventory()
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

    if (pPlayer == nullptr)
        return E_FAIL;

    m_pInventory = pPlayer->Get_Inventory();

    if (m_pInventory == nullptr)
        return E_FAIL;

    return S_OK;
}

PLAYER_PART CUIEquipmentController::Convert_ToPlayerPart(COSTUME_CATEGORY eCategory) const
{
    switch (eCategory)
    {
    case COSTUME_CATEGORY::ACCESSORY:
        return PLAYER_PART::ACC;

    case COSTUME_CATEGORY::MASK:
        return PLAYER_PART::HELMET;

    case COSTUME_CATEGORY::BODY:
        return PLAYER_PART::BODY;

    default:
        return PLAYER_PART::END;
    }
}

_wstring CUIEquipmentController::Get_CostumeCategoryText(COSTUME_CATEGORY eCategory) const
{
    switch (eCategory)
    {
    case COSTUME_CATEGORY::ACCESSORY:
        return L"악세서리";

    case COSTUME_CATEGORY::MASK:
        return L"가면";

    case COSTUME_CATEGORY::BODY:
        return L"의상";

    default:
        return L"";
    }
}

string CUIEquipmentController::Resolve_CostumeIconPath(_uint64 iItemID) const
{
    if (iItemID == ItemID::BODY_BASE)
        return "UI/Item/Costume/UIT_Item_Costume_01_Body.dds";

    if (iItemID == ItemID::BODY_WHITE)
        return "UI/Item/Costume/UIT_Item_Costume_06.dds";

    if (iItemID == ItemID::HELMET_ALCHEMIST)
        return "UI/Item/Costume/UIT_Item_Helmet_Alchemist.dds";

    if (iItemID == ItemID::HELMET_MADDONKEY)
        return "UI/Item/Costume/UIT_Item_Helmet_Madman.dds";

    if (iItemID == ItemID::HELMET_SANTA)
        return "UI/Item/Costume/UIT_Item_Helmet_RedHat.dds";

    if (iItemID == ItemID::ACC_EYEPATCH)
        return "UI/Item/Costume/UIT_Item_Helmet_Eyepatch.dds";

    if (iItemID == ItemID::ACC_RUDOLF_NOSE)
        return "UI/Item/Costume/UIT_Item_Helmet_RudolfNose.dds";

    return "";
}

void CUIEquipmentController::Open_CostumeSelect(COSTUME_CATEGORY eCategory)
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr || m_bCostumeSelectOpening)
        return;

    m_pViewModel->Open_CostumeSelect(eCategory);
    Refresh_CostumeSlots();

    // 선택창과 기존 CostumePage를 함께 표시한 상태로 기존 페이지를 퇴장시킵니다.
    m_pCostumeSelectPage->Set_Visible(true);
    m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(true);

    if (m_pUIObject->Has_Animation("CostumeSelectPageOpen"))
    {
        m_bCostumeSelectOpening = true;
        m_pUIObject->Play_Animation("CostumeSelectPageOpen", true);
    }
    else
    {
        m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(false);
    }
}

void CUIEquipmentController::Close_CostumeSelect()
{
    if (m_pViewModel == nullptr)
        return;

    m_bCostumeSelectOpening = false;
    m_pViewModel->Close_CostumeSelect();

    // 선택창을 숨기고 기존 CostumePage를 다시 표시합니다.
    m_pCostumeSelectPage->Set_Visible(false);
    m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(true);

    if (m_pUIObject->Has_Animation("CostumeSelectPageClose"))
        m_pUIObject->Play_Animation("CostumeSelectPageClose", true);
}

void CUIEquipmentController::Refresh_CostumeSlots()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    vector<_uint64> ItemIDs;

    if (FAILED(m_pInventory->Get_ItemIDs(ITEM_TYPE::BODY_PART, ItemIDs)))
        return;

    const COSTUME_CATEGORY eCategory = m_pViewModel->Get_CostumeCategory();
    const PLAYER_PART eTargetPart = Convert_ToPlayerPart(eCategory);

    if (eTargetPart == PLAYER_PART::END)
        return;

    const _uint64 iEquippedID = m_pInventory->Get_EquippedPartID(eTargetPart);

    m_pViewModel->Clear_CostumeSlots();

    _uint iSlotIndex = 0;

    for (const _uint64 iItemID : ItemIDs)
    {
        const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

        if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::BODY_PART)
            continue;

        if (pItemData->BodyPartData.ePartSlot != eTargetPart)
            continue;

        if (iSlotIndex >= 3)
            break;

        COSTUME_SLOT_DATA SlotData{};

        SlotData.iItemID = iItemID;
        SlotData.bVisible = true;
        SlotData.bEquipped = iItemID == iEquippedID;
        SlotData.strName = pItemData->strDisplayName;
        SlotData.strCategory = Get_CostumeCategoryText(eCategory);
        SlotData.strDescription = pItemData->strDesc;
        SlotData.strIconTexturePath = Resolve_CostumeIconPath(iItemID);

        m_pViewModel->Set_CostumeSlotData(iSlotIndex, SlotData);
        ++iSlotIndex;
    }

    // 현재 카테고리에 보유 코스튬이 하나 이상 있다면 첫 슬롯을 기본 선택한다.
    if (iSlotIndex > 0)
        m_pViewModel->Select_CostumeSlot(0);
}

void CUIEquipmentController::Refresh_MainCostumeSlotIcons()
{
    if (m_pInventory == nullptr)
        return;

    const _uint64 iHelmetID = m_pInventory->Get_EquippedPartID(PLAYER_PART::HELMET);
    const _uint64 iAccessoryID = m_pInventory->Get_EquippedPartID(PLAYER_PART::ACC);
    const _uint64 iBodyID = m_pInventory->Get_EquippedPartID(PLAYER_PART::BODY);

    CUIImage* pHelmetIcon = Find_ElementAs<CUIImage>(L"HeadCostumeSlot.ItemIcon");
    CUIImage* pAccessoryIcon = Find_ElementAs<CUIImage>(L"AccessoryCostumeSlot.ItemIcon");
    CUIImage* pBodyIcon = Find_ElementAs<CUIImage>(L"BodyCostumeSlot.ItemIcon");

    if (pHelmetIcon != nullptr)
        pHelmetIcon->Set_Visible(iHelmetID != 0);

    if (pAccessoryIcon != nullptr)
        pAccessoryIcon->Set_Visible(iAccessoryID != 0);

    if (pBodyIcon != nullptr)
        pBodyIcon->Set_Visible(iBodyID != 0);

    if (iHelmetID != 0)
        Set_ImageTexture(L"HeadCostumeSlot.ItemIcon", Resolve_CostumeIconPath(iHelmetID));

    if (iAccessoryID != 0)
        Set_ImageTexture(L"AccessoryCostumeSlot.ItemIcon", Resolve_CostumeIconPath(iAccessoryID));

    if (iBodyID != 0)
        Set_ImageTexture(L"BodyCostumeSlot.ItemIcon", Resolve_CostumeIconPath(iBodyID));
}

void CUIEquipmentController::Select_CostumeSlot(_uint iSlotIndex)
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Select_CostumeSlot(static_cast<_int>(iSlotIndex));
}

void CUIEquipmentController::Equip_SelectedCostume()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    const _int iSelectedSlot = m_pViewModel->Get_SelectedCostumeSlot();

    if (iSelectedSlot < 0)
        return;

    const COSTUME_SLOT_DATA& SlotData = m_pViewModel->Get_CostumeSlotData(static_cast<_uint>(iSelectedSlot));

    if (!SlotData.bVisible || SlotData.iItemID == 0 || SlotData.bEquipped)
        return;

    // Inventory::Equip에서 실제 플레이어의 CPartSet까지 즉시 적용한다.
    if (!m_pInventory->Equip(SlotData.iItemID))
        return;

    // 실제 장착에 성공한 경우에만 프리뷰에도 동일한 코스튬을 적용한다.
    if (m_pPreviewCharacter != nullptr)
        m_pPreviewCharacter->Equip_PreviewCostume(SlotData.iItemID);

    Refresh_CostumeSlots();
    Refresh_MainCostumeSlotIcons();
}

void CUIEquipmentController::Unequip_SelectedCostume()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    const _int iSelectedSlot = m_pViewModel->Get_SelectedCostumeSlot();

    if (iSelectedSlot < 0)
        return;

    const COSTUME_SLOT_DATA& SlotData = m_pViewModel->Get_CostumeSlotData(static_cast<_uint>(iSelectedSlot));

    if (!SlotData.bVisible || !SlotData.bEquipped)
        return;

    const PLAYER_PART eTargetPart = Convert_ToPlayerPart(m_pViewModel->Get_CostumeCategory());

    // 기본 의상까지 해제되면 몸통 파츠가 사라질 수 있으므로 의상은 교체만 허용한다.
    if (eTargetPart == PLAYER_PART::BODY || eTargetPart == PLAYER_PART::END)
        return;

    m_pInventory->UnEquip_Part(eTargetPart);

    if (m_pPreviewCharacter != nullptr)
        m_pPreviewCharacter->Unequip_PreviewCostume(eTargetPart);
    Refresh_CostumeSlots();
    Refresh_MainCostumeSlotIcons();
}

void CUIEquipmentController::Apply_CostumeSelectViewModel()
{
    const _bool bVisible = m_pViewModel->Is_CostumeSelectVisible();

    m_pCostumeSelectPage->Set_Visible(bVisible);
    m_pCategoryTabGroup->Set_Visible(!bVisible);

    if (!bVisible)
        return;

    const COSTUME_CATEGORY eCategory = m_pViewModel->Get_CostumeCategory();
    const _int iSelectedSlot = m_pViewModel->Get_SelectedCostumeSlot();

    m_pCostumeCategoryText->Set_Text(Get_CostumeCategoryText(eCategory));

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
        m_pCostumeCategoryButtons[iIndex]->Set_Selected(iIndex == static_cast<_uint>(eCategory));

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        const COSTUME_SLOT_DATA& SlotData = m_pViewModel->Get_CostumeSlotData(iIndex);

        m_pCostumeSlotRoots[iIndex]->Set_Visible(SlotData.bVisible);
        m_pCostumeSlotButtons[iIndex]->Set_Selected(static_cast<_int>(iIndex) == iSelectedSlot);
        m_pCostumeEquippedIcons[iIndex]->Set_Visible(SlotData.bVisible && SlotData.bEquipped);

        if (SlotData.bVisible && !SlotData.strIconTexturePath.empty())
            Set_ImageTexture(L"CostumeSelectPage.CostumeSlot" + to_wstring(iIndex) + L".CostumeIcon", SlotData.strIconTexturePath);
    }

    const _bool bHasSelection = iSelectedSlot >= 0;
    const COSTUME_SLOT_DATA* pSelectedData = bHasSelection ? &m_pViewModel->Get_CostumeSlotData(static_cast<_uint>(iSelectedSlot)) : nullptr;

    m_pCostumeDetailPanel->Set_Visible(pSelectedData != nullptr);
    m_pCostumeBackHint->Set_Visible(true);
    m_pCostumeEquipHint->Set_Visible(pSelectedData != nullptr && !pSelectedData->bEquipped);
    m_pCostumeUnequipHint->Set_Visible(pSelectedData != nullptr && pSelectedData->bEquipped && eCategory != COSTUME_CATEGORY::BODY);

    if (pSelectedData == nullptr)
        return;

    m_pCostumeDetailTitle->Set_Text(pSelectedData->strName);
    m_pCostumeDetailCategory->Set_Text(pSelectedData->strCategory);
    m_pCostumeDetailBodyText->Set_Text(pSelectedData->strDescription);

    if (!pSelectedData->strIconTexturePath.empty())
        Set_ImageTexture(L"CostumeSelectPage.DetailIcon", pSelectedData->strIconTexturePath);
}

HRESULT CUIEquipmentController::Set_ImageTexture(const _wstring& strImageTag, const string& strTexturePath)
{
    CUIImage* pImage = Find_ElementAs<CUIImage>(strImageTag);

    if (pImage == nullptr)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Set_UIImageTexture(m_hUI, strImageTag, strTexturePath)))
        return E_FAIL;

    // Set_UIImageTexture는 Material만 교체하므로 렌더 플래그는 별도로 켜야 한다.
    pImage->Set_UseTexture(true);
    pImage->Set_Visible(true);

    return S_OK;
}

// 프리뷰 레이어에서 장비창용 캐릭터와 배경을 함께 찾아 보관한다.
HRESULT CUIEquipmentController::Bind_PreviewObjects()
{ 
    // 두 객체가 모두 연결되어 있다면 레이어를 다시 탐색하지 않는다.
    if (m_pPreviewCharacter != nullptr && m_pPreviewBackground != nullptr)
        return S_OK;

    CLayer* pPreviewLayer = m_pGameInstance->Get_CurrentLevel_Layer(L"Layer_EquipmentPreview");

    if (pPreviewLayer == nullptr)
        return E_FAIL;

    for (CGameObject* pGameObject : pPreviewLayer->Get_Objects())
    {
        if (pGameObject == nullptr)
            continue;

        if (m_pPreviewCharacter == nullptr)
        {
            CEquipmentPreviewCharacter* pPreviewCharacter = dynamic_cast<CEquipmentPreviewCharacter*>(pGameObject);

            if (pPreviewCharacter != nullptr)
            {
                m_pPreviewCharacter = pPreviewCharacter;
                Safe_AddRef(m_pPreviewCharacter);
            }
        }

        if (m_pPreviewBackground == nullptr)
        {
            CEquipmentPreviewBackground* pPreviewBackground = dynamic_cast<CEquipmentPreviewBackground*>(pGameObject);

            if (pPreviewBackground != nullptr)
            {
                m_pPreviewBackground = pPreviewBackground;
                Safe_AddRef(m_pPreviewBackground);
            }
        }

        if (m_pPreviewCharacter != nullptr && m_pPreviewBackground != nullptr)
            return S_OK;
    }

    return E_FAIL;
}

// 장비창용 카메라와 프리뷰 캐릭터를 활성화한다.
void CUIEquipmentController::Begin_EquipmentPreview()
{
    if ((m_pPreviewCharacter == nullptr || m_pPreviewBackground == nullptr) && FAILED(Bind_PreviewObjects()))
        return;

    // 프리뷰 캐릭터가 바라보는 높이를 기준으로 카메라를 설정
    const _vector vPreviewEye = XMVectorSet(0.f, -999.f, -5.5f, 1.f);
    const _vector vPreviewAt = XMVectorSet(0.f, -999.f, 0.f, 1.f);
    const _float fPreviewFovy = XMConvertToRadians(22.f);

    // 프리뷰용 View/Projection을 먼저 바인딩할 수 있도록 카메라 모드를 활성화한다.
    m_pGameInstance->Begin_UIPreviewCamera(vPreviewEye, vPreviewAt, fPreviewFovy);

    Sync_PreviewCostumes();

    m_pPreviewBackground->Set_PreviewActive(true);
    m_pPreviewCharacter->Set_PreviewWeaponSlot(0);
    m_pPreviewCharacter->Apply_EquipmentPreset();
    m_pPreviewCharacter->Set_PreviewActive(true);
}

// 장비창용 카메라와 프리뷰 캐릭터를 비활성화한다.
void CUIEquipmentController::End_EquipmentPreview()
{
    if (m_pPreviewBackground != nullptr)
        m_pPreviewBackground->Set_PreviewActive(false);

    if (m_pPreviewCharacter != nullptr)
    {
        m_pPreviewCharacter->Clear_StatRoomPreset();
        m_pPreviewCharacter->Set_PreviewActive(false);
    }

    // 비정상 종료나 외부 종료 경로에서도 프리뷰 카메라가 남지 않도록 보장한다.
    m_pGameInstance->End_UIPreviewCamera();
}

void CUIEquipmentController::Update_PreviewRotation()
{
    if (m_pCharacterPreviewArea == nullptr || m_pPreviewCharacter == nullptr)
        return;

    CUIElement* pPreviewArea = m_pViewModel->Is_CostumeSelectVisible() ? m_pCostumePreviewArea : m_pCharacterPreviewArea;

    if (pPreviewArea == nullptr)
        return;

    // 좌클릭을 처음 누른 위치가 CharacterPreviewArea 안인지 검사한다.
    if (m_pGameInstance->Mouse_Down(DIMB::LBUTTON))
    {
        _float2 vMouseClient = {};

        if (!m_pGameInstance->Get_MouseClientPosition(&vMouseClient))
        {
            m_bPreviewDragging = false;
            return;
        }

        const _float2 vViewport = m_pGameInstance->Get_ViewportDesc();

        if (vViewport.x <= 0.f || vViewport.y <= 0.f)
        {
            m_bPreviewDragging = false;
            return;
        }

        // 좌측 상단 기준 Client 좌표를 UI 중앙 기준 좌표로 변환한다.
        const _float2 vMouseUI =
        {
            vMouseClient.x - vViewport.x * 0.5f,
            vViewport.y * 0.5f - vMouseClient.y
        };

        m_bPreviewDragging = m_pCharacterPreviewArea->Is_PointInside(vMouseUI);
    }

    // 영역 안에서 드래그를 시작했다면 버튼을 놓을 때까지 회전시킨다.
    if (m_bPreviewDragging && m_pGameInstance->Mouse_Pressing(DIMB::LBUTTON))
    {
        const _long lMouseDeltaX = m_pGameInstance->Get_DIMouseMove(DIMM::X);

        if (lMouseDeltaX != 0)
            m_pPreviewCharacter->Rotate_Preview(static_cast<_float>(-lMouseDeltaX));
    }

    if (m_pGameInstance->Mouse_Up(DIMB::LBUTTON))
        m_bPreviewDragging = false;
}

void CUIEquipmentController::Apply_ErgoProfile()
{
    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return;

    m_pErgoProfile->Set_Visible(m_pViewModel->Is_Visible());
    m_pErgoCountText->Set_Text(Format_Number(m_pViewModel->Get_CurrentErgo()));

    // 장비창에서는 에르고 획득량 표시를 사용하지 않는다.
    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);
}

_wstring CUIEquipmentController::Format_Number(_uint iValue) const
{
    _wstring strValue = to_wstring(iValue);

    for (_int iPosition = static_cast<_int>(strValue.length()) - 3; iPosition > 0; iPosition -= 3)
        strValue.insert(static_cast<size_t>(iPosition), 1, L',');

    return strValue;
}

void CUIEquipmentController::Sync_PreviewCostumes()
{
    if (m_pInventory == nullptr || m_pPreviewCharacter == nullptr)
        return;

    const _uint64 iBodyID = m_pInventory->Get_EquippedPartID(PLAYER_PART::BODY);
    const _uint64 iHelmetID = m_pInventory->Get_EquippedPartID(PLAYER_PART::HELMET);
    const _uint64 iAccessoryID = m_pInventory->Get_EquippedPartID(PLAYER_PART::ACC);

    // 의상은 비어 있으면 프리뷰의 기본 의상으로 복구한다.
    m_pPreviewCharacter->Equip_PreviewCostume(iBodyID != 0 ? iBodyID : ItemID::BODY_BASE);

    if (iHelmetID != 0)
        m_pPreviewCharacter->Equip_PreviewCostume(iHelmetID);
    else
        m_pPreviewCharacter->Unequip_PreviewCostume(PLAYER_PART::HELMET);

    if (iAccessoryID != 0)
        m_pPreviewCharacter->Equip_PreviewCostume(iAccessoryID);
    else
        m_pPreviewCharacter->Unequip_PreviewCostume(PLAYER_PART::ACC);
}

void CUIEquipmentController::Select_DefaultSlotForCurrentCategory()
{
    if (m_pViewModel == nullptr)
        return;

    CUIButton* pPrimaryWeaponButton = Find_ElementAs<CUIButton>(L"WeaponSlotPrimary.SlotButton");
    CUIButton* pFirstUseItemButton = Find_ElementAs<CUIButton>(L"UpperBeltSlot1.SlotButton");
    CUIButton* pSecondaryWeaponButton = Find_ElementAs<CUIButton>(L"WeaponSlotSecondary.SlotButton");

    // 이전 카테고리의 기본 선택 프레임을 먼저 제거한다.
    if (pPrimaryWeaponButton != nullptr)
        pPrimaryWeaponButton->Set_Selected(false);

    if (pFirstUseItemButton != nullptr)
        pFirstUseItemButton->Set_Selected(false);

    if (pSecondaryWeaponButton != nullptr)
        pSecondaryWeaponButton->Set_Selected(false);

    switch (m_pViewModel->Get_Category())
    {
    case EQUIPMENT_CATEGORY::EQUIPMENT:
        Select_MainDetailSlot(L"WeaponSlotPrimary.SlotButton");
        break;

    case EQUIPMENT_CATEGORY::USE_ITEM:
        Select_MainDetailSlot(L"UpperBeltSlot1.SlotButton");
        break;

    case EQUIPMENT_CATEGORY::COSTUME:
        // 코스튬 메인 페이지는 슬롯 클릭 후 선택 페이지에서 상세 정보를 표시한다.
        m_pViewModel->Clear_Detail();
        break;

    default:
        m_pViewModel->Clear_Detail();
        break;
    }
}

void CUIEquipmentController::Refresh_MainEquipmentSlots()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    /* 2번 무기는 현재 게임 구조상 획득하면 빈 무기 슬롯에 자동 장착된다.
       UI 표시는 보유 여부를 기준으로 하고, 실제 아이콘은 해당 무기 ID로 결정한다. */
    const _bool bHasSecondaryWeapon = m_pInventory->Is_Acquired(ItemID::WEAPON_SCISSOR);

    m_pViewModel->Set_SecondaryWeaponOwned(bHasSecondaryWeapon);
    m_pSecondaryWeaponButton->Set_Interactable(bHasSecondaryWeapon);

    if (bHasSecondaryWeapon)
    {
        const string strWeaponIconPath = Resolve_WeaponIconPath(ItemID::WEAPON_SCISSOR);

        if (!strWeaponIconPath.empty())
            Set_ImageTexture(L"WeaponSlotSecondary.ItemIcon", strWeaponIconPath);
    }
    else
    {
        // 획득 전에는 기존 빈 무기 슬롯 이미지를 유지한다.
        Set_ImageTexture(L"WeaponSlotSecondary.ItemIcon", "UI/Item/UIT_Item_Weapon_0.dds");
        m_pSecondaryWeaponButton->Set_Selected(false);
    }

    // 리전 암은 보유 목록이 아니라 현재 실제 장착 ID를 표시한다.
    const _uint64 iEquippedLegionArmID = m_pInventory->Get_EquippedID(ITEM_TYPE::LEGION_ARM);
    const string strLegionArmIconPath = Resolve_LegionArmIconPath(iEquippedLegionArmID);
    const _bool bHasEquippedLegionArm = iEquippedLegionArmID != 0 && !strLegionArmIconPath.empty();

    m_pLegionArmButton->Set_Interactable(bHasEquippedLegionArm);
    m_pLegionArmIcon->Set_Visible(bHasEquippedLegionArm);

    if (bHasEquippedLegionArm)
        Set_ImageTexture(L"LegionArmSlot.ItemIcon", strLegionArmIconPath);
    else
        m_pLegionArmButton->Set_Selected(false);
}

void CUIEquipmentController::Select_MainDetailSlot(const _wstring& strRuntimeElementTag)
{
    if (m_pViewModel == nullptr)
        return;

    // 2번 무기는 획득 전에는 선택할 수 없다.
    if (strRuntimeElementTag == L"WeaponSlotSecondary.SlotButton" && !m_pViewModel->Has_SecondaryWeapon())
        return;

    const _wstring DetailSlotTags[] =
    {
        L"WeaponSlotPrimary.SlotButton",
        L"WeaponSlotSecondary.SlotButton",
        L"UpperBeltSlot1.SlotButton",
        L"LowerBeltSlot1.SlotButton",
        L"LowerBeltSlot2.SlotButton",
        L"PouchSlot1.SlotButton",
        L"PouchSlot3.SlotButton"
    };

    for (const _wstring& strSlotTag : DetailSlotTags)
    {
        CUIButton* pSlotButton = Find_ElementAs<CUIButton>(strSlotTag);

        if (pSlotButton != nullptr)
            pSlotButton->Set_Selected(strSlotTag == strRuntimeElementTag);
    }

    // 무기 슬롯 선택에 맞춰 프리뷰 캐릭터가 들고 있는 무기를 변경한다.
    if (m_pPreviewCharacter != nullptr)
    {
        if (strRuntimeElementTag == L"WeaponSlotPrimary.SlotButton")
            m_pPreviewCharacter->Set_PreviewWeaponSlot(0);
        else if (strRuntimeElementTag == L"WeaponSlotSecondary.SlotButton")
            m_pPreviewCharacter->Set_PreviewWeaponSlot(1);
    }

    // 선택이 확정된 슬롯의 상세 정보만 갱신한다.
    Apply_SlotDetail(strRuntimeElementTag);
}

string CUIEquipmentController::Resolve_WeaponIconPath(_uint64 iItemID) const
{
    switch (iItemID)
    {
    case ItemID::WEAPON_SCISSOR:
        return "UI/Item/Weapon/UIT_Heroic_ScissorSword_01.dds";

    default:
        return "";
    }
}

string CUIEquipmentController::Resolve_LegionArmIconPath(_uint64 iItemID) const
{
    switch (iItemID)
    {
    case ItemID::LEGION_BASE:
        return "UI/Item/Arm/UIT_Item_Arm_1.dds";

    case ItemID::LEGION_GRAPPLE:
        return "UI/Item/Arm/UIT_Item_Arm_GG_01.dds";

    case ItemID::LEGION_CHARGEGUN:
        return "UI/Item/Arm/UIT_Item_Arm_SniperCannon.dds";

    default:
        return "";
    }
}

HRESULT CUIEquipmentController::Set_Header(const _wstring& strHeaderTag, const _wstring& strHeaderText, const string& strIconTexturePath)
{
    CUIText* pHeaderText = Find_ElementAs<CUIText>(strHeaderTag + L".HeaderText");

    if (pHeaderText == nullptr)
        return E_FAIL;

    pHeaderText->Set_Text(strHeaderText);

    return Set_ImageTexture(strHeaderTag + L".HeaderIcon", strIconTexturePath);
}

HRESULT CUIEquipmentController::Set_InputHint(const _wstring& strHintTag, const _wstring& strActionText, const string& strKeyTexturePath)
{
    CUIText* pActionText = Find_ElementAs<CUIText>(strHintTag + L".ActionText");

    if (pActionText == nullptr)
        return E_FAIL;

    pActionText->Set_Text(strActionText);

    return Set_ImageTexture(strHintTag + L".KeyIcon", strKeyTexturePath);
}

HRESULT CUIEquipmentController::Apply_StaticPresentation()
{
    // 고정 헤더는 플레이어 데이터와 무관하므로 한 번만 설정한다.
    if (FAILED(Set_Header(L"Header_Weapon", L"무기", "UI/TabIcon/UIT_Weapon.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_LegionArm", L"리전 암", "UI/TabIcon/UIT_SlaveArm.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_Amulet", L"아뮬렛", "UI/TabIcon/UIT_Acc.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_DefensePart", L"방어 파츠", "UI/TabIcon/UIT_Parts.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_UpperBelt", L"위 벨트", "UI/PlayerMenu/UIT_SubTItleIcon_Bag1.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_LowerBelt", L"아래 벨트", "UI/PlayerMenu/UIT_SubTItleIcon_Bag2.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_Pouch", L"보조가방", "UI/PlayerMenu/UIT_SubTItleIcon_Pouch.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_HeadCostume", L"머리", "UI/TabIcon/UIT_HeadCostume.dds")))
        return E_FAIL;

    if (FAILED(Set_Header(L"Header_BodyCostume", L"의상", "UI/TabIcon/UIT_BodyCostume.dds")))
        return E_FAIL;

    // 아직 두 번째 무기를 얻지 않은 상태에서 사용하는 기본 슬롯 이미지다.
    if (FAILED(Set_ImageTexture(L"WeaponSlotSecondary.ItemIcon", "UI/Item/UIT_Item_Weapon_0.dds")))
        return E_FAIL;

    if (FAILED(Set_ImageTexture(L"AmuletSlot01.ItemIcon", "UI/Item/UIT_Gear_Amulet_0.dds")))
        return E_FAIL;

    if (FAILED(Set_ImageTexture(L"AmuletSlot02.ItemIcon", "UI/Item/UIT_Gear_Amulet_0.dds")))
        return E_FAIL;

    if (FAILED(Set_ImageTexture(L"AmuletSlot03.ItemIcon", "UI/Item/UIT_Gear_Amulet_0.dds")))
        return E_FAIL;

    if (FAILED(Set_ImageTexture(L"AmuletSlot04.ItemIcon", "UI/Item/UIT_Gear_Amulet_0.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"PrimaryActionHint", L"장착", "UI/ImgIcon/UIT_Icon_Key_Space.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"UnequipActionHint", L"장착 해제", "UI/ImgIcon/UIT_Icon_Key_R.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"DisplaySwitchHint", L"표시 전환", "UI/ImgIcon/UIT_Icon_Key_F.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CharacterInfoHint", L"캐릭터 정보", "UI/ImgIcon/UIT_Icon_Key_C.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CloseHint", L"닫기", "UI/ImgIcon/UIT_Icon_Key_ESC.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CompactPrimaryActionHint", L"장착", "UI/ImgIcon/UIT_Icon_Key_Space.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CompactCharacterInfoHint", L"캐릭터 정보", "UI/ImgIcon/UIT_Icon_Key_C.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CompactCloseHint", L"닫기", "UI/ImgIcon/UIT_Icon_Key_ESC.dds")))
        return E_FAIL;

    if (FAILED(Set_ImageTexture(L"BodyCostumeSlot.ItemIcon", "UI/Item/Costume/UIT_Item_Costume_01_Body.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CostumeSelectPage.BackHint", L"이전", "UI/ImgIcon/UIT_Icon_Key_ESC.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CostumeSelectPage.EquipHint", L"장착", "UI/ImgIcon/UIT_Icon_Key_Space.dds")))
        return E_FAIL;

    if (FAILED(Set_InputHint(L"CostumeSelectPage.UnequipHint", L"장착 해제", "UI/ImgIcon/UIT_Icon_Key_R.dds")))
        return E_FAIL;

    // 최초 상태는 상세 설명이 없으므로 Compact 입력 안내만 보여준다.
    m_pDetailPanel->Set_Visible(false);
    m_pInputHintActionGroup->Set_Visible(false);
    m_pInputHintCompactGroup->Set_Visible(true);
    m_pCostumeSelectPage->Set_Visible(false);
    m_pCostumeDetailPanel->Set_Visible(false);
    m_pCostumeEquipHint->Set_Visible(false);
    m_pCostumeUnequipHint->Set_Visible(false);

    return S_OK;
}

void CUIEquipmentController::Apply_SlotDetail(const _wstring& strRuntimeElementTag)
{
    if (m_pViewModel == nullptr)
        return;

    if (strRuntimeElementTag == L"WeaponSlotPrimary.SlotButton")
    {
        // 1번 무기는 항상 보유하며 날과 손잡이를 조합형 이미지로 표시한다.
        m_pViewModel->Set_WeaponDetail(
            EQUIPMENT_DETAIL_MODE::WEAPON_ASSEMBLED,
            L"생사의 대검 날과 자루",
            L"대검",
            L"군용 인형의 무기를 개량한 대검.\n총검으로 사용해도 발사에 방해되지 않게 특이한 형태로 휘어있다.\n사선을 넘나든 스토커의 눈에는 사냥감의 실이 보인다고 한다.\n강한 인형일수록 선명하게 품고 있다는, 끈적거리는 집착의 거미줄이.",
            "UI/Item/Weapon/UIT_Blade_Bbayonet.dds",
            "UI/Item/Weapon/UIT_Handle_Bbayonet.dds");
    }
    else if (strRuntimeElementTag == L"WeaponSlotSecondary.SlotButton")
    {
        // 2번 무기는 시퀀스 진행으로 획득하기 전까지 빈 슬롯으로 취급한다.
        if (!m_pViewModel->Has_SecondaryWeapon())
        {
            m_pViewModel->Clear_Detail();
            return;
        }

        m_pViewModel->Set_WeaponDetail(
            EQUIPMENT_DETAIL_MODE::WEAPON_SINGLE,
            L"인간성의 증거",
            L"도검",
            L"이름 없는 인형이 사용하던 거대한 가위칼.\n누군가의 자유를 끊거나, 또는 조종줄을 끊어 자유를 쟁취할 수 있는 양날의 검이다.\n이펙트가 정말 멋있습니다.",
            "UI/Item/Weapon/UIT_Heroic_ScissorSword_01.dds");
    }
    else if (strRuntimeElementTag == L"UpperBeltSlot1.SlotButton")
    {
        m_pViewModel->Set_ItemDetail(
            L"펄스전지",
            L"기본 아이템",
            L"펄스전지를 사용하면 HP를 회복할 수 있다.\n펄스전지는 최대 사용 횟수가 정해져 있다.\n최대 사용 가능 횟수를 모두 사용하면 방전 상태가 되며, 방전 상태에서 적을 공격하면 펄스전지가 조금씩 충전된다.\n\n펄스전지는 P기관에 직접 동력을 제공하는 에르고 전지다.\n별바라기뿐 아니라, 공격을 통한 충격 에너지를 전환해서도 충전할 수 있다.",
            "UI/Item/Consume/UIT_Item_Potion_01.dds");
    }
    else if (strRuntimeElementTag == L"LowerBeltSlot1.SlotButton")
    {
        m_pViewModel->Set_ItemDetail(
            L"그라인더",
            L"기본 아이템",
            L"무기로 적을 공격할 때 마다 무기 내구도가 감소한다.\n무기는 그라인더로 수리가 가능하다.\n무기 내구도가 모두 소모되면 공격력이 하락하고 그라인더로도 수리가 불가능해지니 내구도 게이지를 신경쓰는 것이 좋다",
            "UI/Item/Consume/UIT_Item_Sharpentool.dds");
    }
    else if (strRuntimeElementTag == L"LowerBeltSlot2.SlotButton")
    {
        m_pViewModel->Set_ItemDetail(
            L"모나드의 등불",
            L"기본 아이템",
            L"귀뚜라미 가이드 인형이 깃든 작은 등불.\n은은한 빛으로 어둠을 밝혀준다.\n\n안개 속에서 길을 잃어도 두려워 마라.\n귀뚜라미의 인도가 너와 함께 하리라.",
            "UI/Item/Consume/UIT_Item_Lamp.dds");
    }
    else if (strRuntimeElementTag == L"PouchSlot1.SlotButton")
    {
        m_pViewModel->Set_ItemDetail(
            L"문페이즈 회중시계",
            L"기본 아이템",
            L"사용한 자를 이전에 방문한 별바라기나 호텔의 별바라기로 돌려보낸다.\n신비한 힘이 깃들어 있는 회중시계.\n시계에 깃든 힘은 소년의 시간을 최상의 상태일 때로 되돌린다.",
            "UI/Item/Consume/UIT_Item_Teleport_2.dds");
    }
    else if (strRuntimeElementTag == L"PouchSlot3.SlotButton")
    {
        m_pViewModel->Set_ItemDetail(
            L"최후의 수단",
            L"기본 아이템",
            L"사용 즉시 사망한다.",
            "UI/Item/Consume/UIT_Item_Death.dds");
    }
    else
    {
        // 빈 슬롯 및 아직 데이터 연결을 미룬 슬롯은 상세 패널을 숨긴다.
        m_pViewModel->Clear_Detail();
    }
}

CUIEquipmentController* CUIEquipmentController::Create()
{
    return new CUIEquipmentController;
}

void CUIEquipmentController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    const _bool bCharacterActive = m_pPreviewCharacter != nullptr && m_pPreviewCharacter->Is_PreviewActive();
    const _bool bBackgroundActive = m_pPreviewBackground != nullptr && m_pPreviewBackground->Is_PreviewActive();

    if (bCharacterActive || bBackgroundActive)
        End_EquipmentPreview();

    Safe_Release(m_pPreviewBackground);
    Safe_Release(m_pPreviewCharacter);

    if (m_bHUDLayerSuppressed && m_pGameInstance != nullptr)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    Safe_Release(m_pViewModel);

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        m_pPages[iIndex] = nullptr;
        m_pCategoryButtons[iIndex] = nullptr;
    }

    m_pDetailPanel = nullptr;
    m_pInputHintActionGroup = nullptr;
    m_pInputHintCompactGroup = nullptr;

    m_pInventory = nullptr;
    m_pErgoProfile = nullptr;
    m_pErgoCountText = nullptr;
    m_pErgoGetText = nullptr;

    __super::Free();
}