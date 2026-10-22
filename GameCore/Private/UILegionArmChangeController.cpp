#include "UILegionArmChangeController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "LegionArmChangeViewModel.h"
#include "Layer.h"
#include "EquipmentPreviewCharacter.h"
#include "Player.h"
#include "Inventory.h"
#include "ItemTable.h"

HRESULT CUILegionArmChangeController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CLegionArmChangeViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    m_pGameInstance->Subscribe(EVENT_TYPE_UI::LEGION_ARM_CHANGE_OPEN_REQUEST, this);

    Configure_Buttons();

    m_pDetailPrimaryActionText->Set_Text(L"변경");
    m_pDetailCloseActionText->Set_Text(L"닫기");
    m_pSelectCloseActionText->Set_Text(L"이전");

    return S_OK;
}

void CUILegionArmChangeController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_bCloseAnimationPending || m_ePageTransitionState != PAGE_TRANSITION_STATE::NONE)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "OpenLegionArmSelectPage")
    {
        Begin_OpenSelectPage();
    }
    else if (Event.strCommandKey == "SelectLegionArm")
    {
        if (Event.strRuntimeElementTag == L"LegionArmSlot01.SlotButton")
            m_pViewModel->Set_FocusedArmIndex(0);
        else if (Event.strRuntimeElementTag == L"LegionArmSlot02.SlotButton")
            m_pViewModel->Set_FocusedArmIndex(1);
        else if (Event.strRuntimeElementTag == L"LegionArmSlot03.SlotButton")
            m_pViewModel->Set_FocusedArmIndex(2);            
        else
            return;

        // 인벤토리 장착에 성공한 경우에만 UI와 프리뷰를 변경한다.
        if (!Equip_FocusedLegionArm())
            return;

        if (m_pPreviewCharacter != nullptr)
            m_pPreviewCharacter->Play_LegionArmStatRoomAction(Get_PreviewLegionArmType());

        // 선택 목록을 닫은 뒤 디테일 페이지를 다시 연다.
        Begin_OpenDetailPage();
    }
    else if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"CompactPrimaryActionHint.HintButtonSpace")
        {
            Begin_OpenSelectPage();
        }
        else if (Event.strRuntimeElementTag == L"CompactCloseHint.HintButton")
        {
            Close_LegionArmChange();
        }
        else if (Event.strRuntimeElementTag == L"SelectCloseHint.HintButton")
        {
            Begin_OpenDetailPage();
        }
    }
}

void CUILegionArmChangeController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (Event.strFocusGroup != "LegionArmSlot" || !Event.bFocused)
        return;

    if (Event.strRuntimeElementTag == L"LegionArmSlot01.SlotButton")
        m_pViewModel->Set_FocusedArmIndex(0);
    else if (Event.strRuntimeElementTag == L"LegionArmSlot02.SlotButton")
        m_pViewModel->Set_FocusedArmIndex(1);
    else if (Event.strRuntimeElementTag == L"LegionArmSlot03.SlotButton")
        m_pViewModel->Set_FocusedArmIndex(2);
}

void CUILegionArmChangeController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    UNREFERENCED_PARAMETER(pData);

    if (eType != EVENT_TYPE_UI::LEGION_ARM_CHANGE_OPEN_REQUEST)
        return;

    if (m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;

    // UI를 열기 직전에 실제 플레이어 인벤토리 상태를 가져온다.
    if (FAILED(Bind_PlayerInventory()))
        return;

    Sync_FromInventory();
    m_pViewModel->Open_LegionArmChange();
}

HRESULT CUILegionArmChangeController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pDetailPageRoot = Find_Element(L"LegionArmDetailPageRoot");
    m_pSelectPageRoot = Find_Element(L"LegionArmSelectPageRoot");

    m_pEquippedArmButton = Find_ElementAs<CUIButton>(L"EquippedLegionArmSlot.SlotButton");
    m_pEquippedArmIcon = Find_ElementAs<CUIImage>(L"EquippedLegionArmSlot.ItemIcon");
    m_pLegionArmNameText = Find_ElementAs<CUIText>(L"LegionArmNameText");

    m_pSelectDetailPanel = Find_Element(L"DetailPanel");
    m_pSelectDetailTitle = Find_ElementAs<CUIText>(L"DetailTitle");
    m_pSelectDetailIcon = Find_ElementAs<CUIImage>(L"DetailIcon");
    m_pSelectDetailCategory = Find_ElementAs<CUIText>(L"DetailCategory");
    m_pSelectDetailBodyText = Find_ElementAs<CUIText>(L"DetailBodyText");

    m_pDetailPrimaryActionText = Find_ElementAs<CUIText>(L"CompactPrimaryActionHint.ActionText");
    m_pDetailCloseActionText = Find_ElementAs<CUIText>(L"CompactCloseHint.ActionText");
    m_pSelectCloseActionText = Find_ElementAs<CUIText>(L"SelectCloseHint.ActionText");

    if (m_pDetailPageRoot == nullptr || m_pSelectPageRoot == nullptr || m_pEquippedArmButton == nullptr || m_pEquippedArmIcon == nullptr || m_pLegionArmNameText == nullptr)
        return E_FAIL;

    if (m_pSelectDetailPanel == nullptr || m_pSelectDetailTitle == nullptr || m_pSelectDetailIcon == nullptr || m_pSelectDetailCategory == nullptr || m_pSelectDetailBodyText == nullptr)
        return E_FAIL;

    if (m_pDetailPrimaryActionText == nullptr || m_pDetailCloseActionText == nullptr || m_pSelectCloseActionText == nullptr)
        return E_FAIL;

    for (_uint iAbilityIndex = 0; iAbilityIndex < LEGION_ARM_CHANGE_ABILITY_UI_COUNT; ++iAbilityIndex)
    {
        const _wstring strNumber = to_wstring(iAbilityIndex + 1);

        m_pAbilityDescriptionGroups[iAbilityIndex] = Find_Element(L"AbilityDescription" + strNumber);
        m_pAbilityNameTexts[iAbilityIndex] = Find_ElementAs<CUIText>(L"AbilityName" + strNumber);
        m_pAbilityDetailTexts[iAbilityIndex] = Find_ElementAs<CUIText>(L"AbilityDetail" + strNumber);

        if (m_pAbilityDescriptionGroups[iAbilityIndex] == nullptr || m_pAbilityNameTexts[iAbilityIndex] == nullptr || m_pAbilityDetailTexts[iAbilityIndex] == nullptr)
            return E_FAIL;
    }

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_CHANGE_ARM_COUNT; ++iArmIndex)
    {
        const _wstring strSlotTag = L"LegionArmSlot0" + to_wstring(iArmIndex + 1);

        m_pLegionArmSlots[iArmIndex] = Find_Element(strSlotTag);
        m_pLegionArmSlotButtons[iArmIndex] = Find_ElementAs<CUIButton>(strSlotTag + L".SlotButton");
        m_pLegionArmSlotIcons[iArmIndex] = Find_ElementAs<CUIImage>(strSlotTag + L".ItemIcon");
        m_pInstallingIcons[iArmIndex] = Find_ElementAs<CUIImage>(L"InstallingIcon" + to_wstring(iArmIndex + 1));

        if (m_pLegionArmSlots[iArmIndex] == nullptr || m_pLegionArmSlotButtons[iArmIndex] == nullptr || m_pLegionArmSlotIcons[iArmIndex] == nullptr || m_pInstallingIcons[iArmIndex] == nullptr)
            return E_FAIL;
    }

    return S_OK;
}

void CUILegionArmChangeController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    // 전체 UI Close 애니메이션 중에는 다른 입력과 페이지 전환을 막는다.
    if (m_bCloseAnimationPending)
    {
        if (m_pUIObject != nullptr && m_pUIObject->Is_AnimationFinished())
            Finalize_Close();

        return;
    }

    // Detail/Select 페이지 전환 애니메이션을 순차 처리한다.
    if (m_ePageTransitionState != PAGE_TRANSITION_STATE::NONE)
    {
        if (m_pUIObject == nullptr || m_pUIObject->Is_AnimationFinished())
            Update_PageTransition();

        return;
    }

    if (!m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        if (m_pViewModel->Is_SelectPage())
            Begin_OpenDetailPage();
        else
            Close_LegionArmChange();
    }
    else if (m_pViewModel->Is_DetailPage() && m_pGameInstance->Key_Down(DIK_SPACE))
    {
        Begin_OpenSelectPage();
    }
}

void CUILegionArmChangeController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);
    const _bool bJustOpened = bShouldOpen && !bActuallyOpen;

    if (bShouldOpen)
    {
        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pViewModel->Close_LegionArmChange();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pViewModel->Close_LegionArmChange();
                return;
            }

            Begin_LegionArmPreview();
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            Begin_CloseAnimation();
            return;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    m_pDetailPageRoot->Set_Visible(m_pViewModel->Is_DetailPage());
    m_pSelectPageRoot->Set_Visible(m_pViewModel->Is_SelectPage());

    if (m_pViewModel->Is_DetailPage())
        Apply_DetailPage();
    else
        Apply_SelectPage();

    if (bJustOpened && m_pViewModel->Is_DetailPage() && m_pUIObject != nullptr && m_pUIObject->Has_Animation("OpenDetail"))
        m_pUIObject->Play_Animation("OpenDetail", true);

    m_pViewModel->Clear_Dirty();
}

void CUILegionArmChangeController::Close_LegionArmChange()
{
    if (m_pViewModel != nullptr)
        m_pViewModel->Close_LegionArmChange();
}

void CUILegionArmChangeController::Apply_DetailPage()
{
    const LEGION_ARM_UI_DATA& ArmData = m_pViewModel->Get_ArmData(m_pViewModel->Get_EquippedArmIndex());

    m_pLegionArmNameText->Set_Text(ArmData.strName);
    m_pEquippedArmButton->Set_Selected(true);

    Set_ImageTexture(m_pEquippedArmIcon, L"EquippedLegionArmSlot.ItemIcon", ArmData.strIconTexturePath, m_strAppliedEquippedIconPath);

    for (_uint iAbilityIndex = 0; iAbilityIndex < LEGION_ARM_CHANGE_ABILITY_UI_COUNT; ++iAbilityIndex)
    {
        const _bool bVisible = m_pViewModel->Is_AbilityVisible(iAbilityIndex);
        m_pAbilityDescriptionGroups[iAbilityIndex]->Set_Visible(bVisible);

        if (!bVisible)
            continue;

        m_pAbilityNameTexts[iAbilityIndex]->Set_Text(ArmData.strAbilityNames[iAbilityIndex]);
        m_pAbilityDetailTexts[iAbilityIndex]->Set_Text(ArmData.strAbilityDescriptions[iAbilityIndex]);
    }
}

void CUILegionArmChangeController::Apply_SelectPage()
{
    const _uint iFocusedArmIndex = m_pViewModel->Get_FocusedArmIndex();
    const _uint iEquippedArmIndex = m_pViewModel->Get_EquippedArmIndex();
    const LEGION_ARM_UI_DATA& FocusedArmData = m_pViewModel->Get_ArmData(iFocusedArmIndex);

    m_pSelectDetailTitle->Set_Text(FocusedArmData.strName);
    m_pSelectDetailCategory->Set_Text(L"리전 암");
    m_pSelectDetailBodyText->Set_Text(FocusedArmData.strAbilityDescriptions[0]);

    Set_ImageTexture(m_pSelectDetailIcon, L"DetailIcon", FocusedArmData.strIconTexturePath, m_strAppliedDetailIconPath);

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_CHANGE_ARM_COUNT; ++iArmIndex)
    {
        const LEGION_ARM_UI_DATA& ArmData = m_pViewModel->Get_ArmData(iArmIndex);
        const _bool bOwned = ArmData.bOwned;

        m_pLegionArmSlots[iArmIndex]->Set_Visible(bOwned);
        m_pLegionArmSlotButtons[iArmIndex]->Set_Interactable(bOwned);
        m_pLegionArmSlotButtons[iArmIndex]->Set_Selected(bOwned && iArmIndex == iFocusedArmIndex);
        m_pInstallingIcons[iArmIndex]->Set_Visible(bOwned && iArmIndex == iEquippedArmIndex);

        if (!bOwned)
            continue;

        const _wstring strSlotTag = L"LegionArmSlot0" + to_wstring(iArmIndex + 1);
        Set_ImageTexture(m_pLegionArmSlotIcons[iArmIndex], strSlotTag + L".ItemIcon", ArmData.strIconTexturePath, m_strAppliedSlotIconPaths[iArmIndex]);
    }
}

void CUILegionArmChangeController::Configure_Buttons()
{
    m_pEquippedArmIcon->Set_Size(_float2(95.f, 95.f));

    m_pEquippedArmButton->Set_CommandKey("OpenLegionArmSelectPage");
    m_pEquippedArmButton->Set_FocusGroup("LegionArmDetailAction");
    m_pEquippedArmButton->Set_FocusOnHover(true);
    m_pEquippedArmButton->Set_SelectOnFocus(true);
    m_pEquippedArmButton->Set_FocusOrder(0);

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_CHANGE_ARM_COUNT; ++iArmIndex)
    {
        m_pLegionArmSlotIcons[iArmIndex]->Set_Size(_float2(95.f, 95.f));

        m_pLegionArmSlotButtons[iArmIndex]->Set_CommandKey("SelectLegionArm");
        m_pLegionArmSlotButtons[iArmIndex]->Set_FocusGroup("LegionArmSlot");
        m_pLegionArmSlotButtons[iArmIndex]->Set_FocusOnHover(true);
        m_pLegionArmSlotButtons[iArmIndex]->Set_SelectOnFocus(true);
        m_pLegionArmSlotButtons[iArmIndex]->Set_FocusOrder(static_cast<_int>(iArmIndex));
    }
}

HRESULT CUILegionArmChangeController::Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath)
{
    if (pImage == nullptr)
        return E_FAIL;

    if (strTexturePath.empty())
    {
        pImage->Set_UseTexture(false);
        pImage->Set_Visible(false);
        strAppliedTexturePath.clear();
        return S_OK;
    }

    if (strAppliedTexturePath != strTexturePath)
    {
        if (FAILED(m_pGameInstance->Set_UIImageTexture(m_hUI, strImageTag, strTexturePath)))
            return E_FAIL;

        strAppliedTexturePath = strTexturePath;
    }

    pImage->Set_UseTexture(true);
    pImage->Set_Visible(true);

    return S_OK;
}

HRESULT CUILegionArmChangeController::Bind_PreviewCharacter()
{
    if (m_pPreviewCharacter != nullptr)
        return S_OK;

    CLayer* pPreviewLayer = m_pGameInstance->Get_CurrentLevel_Layer(L"Layer_EquipmentPreview");

    if (pPreviewLayer == nullptr)
        return E_FAIL;

    for (CGameObject* pGameObject : pPreviewLayer->Get_Objects())
    {
        CEquipmentPreviewCharacter* pPreviewCharacter = dynamic_cast<CEquipmentPreviewCharacter*>(pGameObject);

        if (pPreviewCharacter == nullptr)
            continue;

        m_pPreviewCharacter = pPreviewCharacter;
        Safe_AddRef(m_pPreviewCharacter);
        return S_OK;
    }

    return E_FAIL;
}

void CUILegionArmChangeController::Begin_LegionArmPreview()
{
    if (m_pPreviewCharacter == nullptr && FAILED(Bind_PreviewCharacter()))
        return;

    const _vector vPreviewEye = XMVectorSet(0.20f, -998.5f, -2.65f, 1.f);
    const _vector vPreviewAt = XMVectorSet(-0.05f, -998.55f, 0.f, 1.f);
    const _float fPreviewFovy = XMConvertToRadians(16.5f);

    m_pGameInstance->Begin_UIPreviewCamera(vPreviewEye, vPreviewAt, fPreviewFovy);

    const LegionArmType eArmType = Get_PreviewLegionArmType();

    if (eArmType != LegionArmType::End)
        m_pPreviewCharacter->Apply_LegionArmPreset(eArmType);
}

void CUILegionArmChangeController::Restore_EquipmentPreview()
{
    if (m_pPreviewCharacter == nullptr)
        return;

    const _vector vPreviewEye = XMVectorSet(0.f, -999.f, -5.5f, 1.f);
    const _vector vPreviewAt = XMVectorSet(0.f, -999.f, 0.f, 1.f);
    const _float fPreviewFovy = XMConvertToRadians(22.f);

    m_pGameInstance->Begin_UIPreviewCamera(vPreviewEye, vPreviewAt, fPreviewFovy);
    m_pPreviewCharacter->Apply_EquipmentPreset();
}

LegionArmType CUILegionArmChangeController::Get_PreviewLegionArmType() const
{
    if (m_pViewModel == nullptr)
        return LegionArmType::End;

    const LEGION_ARM_UI_DATA& ArmData = m_pViewModel->Get_ArmData(m_pViewModel->Get_EquippedArmIndex());
    const ITEM_DATA* pItemData = CItemTable::Find(ArmData.iItemID);

    if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::LEGION_ARM)
        return LegionArmType::End;

    return pItemData->LegionArmData.eArmType;
}

HRESULT CUILegionArmChangeController::Bind_PlayerInventory()
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

    if (pPlayer == nullptr)
        return E_FAIL;

    m_pInventory = pPlayer->Get_Inventory();

    if (m_pInventory == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUILegionArmChangeController::Sync_FromInventory()
{
    if (m_pInventory == nullptr || m_pViewModel == nullptr)
        return;

    _uint iEquippedArmIndex = 0;

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_CHANGE_ARM_COUNT; ++iArmIndex)
    {
        const LEGION_ARM_UI_DATA& ArmData = m_pViewModel->Get_ArmData(iArmIndex);

        m_pViewModel->Set_ArmOwned(iArmIndex, m_pInventory->Is_Acquired(ArmData.iItemID));

        const _uint iUpgradeLevel = m_pInventory->Is_LegionArm_Upgraded(ArmData.iItemID) ? LEGION_ARM_CHANGE_UPGRADED_LEVEL : LEGION_ARM_CHANGE_DEFAULT_LEVEL;
        m_pViewModel->Set_UpgradeLevel(iArmIndex, iUpgradeLevel);

        if (m_pInventory->Is_Equipped(ArmData.iItemID))
            iEquippedArmIndex = iArmIndex;
    }

    m_pViewModel->Set_EquippedArmIndex(iEquippedArmIndex);
}

_bool CUILegionArmChangeController::Equip_FocusedLegionArm()
{
    if (m_pInventory == nullptr || m_pViewModel == nullptr)
        return false;

    const _uint iFocusedArmIndex = m_pViewModel->Get_FocusedArmIndex();
    const LEGION_ARM_UI_DATA& ArmData = m_pViewModel->Get_ArmData(iFocusedArmIndex);

    if (!ArmData.bOwned)
        return false;

    // 실제 플레이어의 LegionArmManager까지 변경한다.
    if (!m_pInventory->Equip(ArmData.iItemID))
        return false;

    // 장착 성공 후 인벤토리 상태를 다시 읽어 UI를 동기화한다.
    Sync_FromInventory();

    return true;
}

void CUILegionArmChangeController::Begin_OpenSelectPage()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_DetailPage() || m_ePageTransitionState != PAGE_TRANSITION_STATE::NONE || m_bCloseAnimationPending)
        return;

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("CloseDetail"))
    {
        // 현재 디테일 콘텐츠를 먼저 왼쪽으로 퇴장시킨다.
        m_ePageTransitionState = PAGE_TRANSITION_STATE::CLOSING_DETAIL;
        m_pUIObject->Play_Animation("CloseDetail", true);
        return;
    }

    m_pViewModel->Open_SelectPage();
}

void CUILegionArmChangeController::Begin_OpenDetailPage()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_SelectPage() || m_ePageTransitionState != PAGE_TRANSITION_STATE::NONE || m_bCloseAnimationPending)
        return;

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("CloseSelect"))
    {
        // 현재 슬롯 목록을 먼저 왼쪽으로 퇴장시킨다.
        m_ePageTransitionState = PAGE_TRANSITION_STATE::CLOSING_SELECT;
        m_pUIObject->Play_Animation("CloseSelect", true);
        return;
    }

    m_pViewModel->Open_DetailPage();
}

void CUILegionArmChangeController::Update_PageTransition()
{
    switch (m_ePageTransitionState)
    {
    case PAGE_TRANSITION_STATE::CLOSING_DETAIL:
        // Detail 퇴장 완료 후 Select 페이지를 표시한다.
        m_pViewModel->Open_SelectPage();
        m_pDetailPageRoot->Set_Visible(false);
        m_pSelectPageRoot->Set_Visible(true);

        if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("OpenSelect"))
        {
            m_ePageTransitionState = PAGE_TRANSITION_STATE::OPENING_SELECT;
            m_pUIObject->Play_Animation("OpenSelect", true);
        }
        else
        {
            m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;
        }
        break;

    case PAGE_TRANSITION_STATE::OPENING_SELECT:
        m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;
        break;

    case PAGE_TRANSITION_STATE::CLOSING_SELECT:
        // Select 퇴장 완료 후 Detail 페이지를 표시한다.
        m_pViewModel->Open_DetailPage();
        m_pSelectPageRoot->Set_Visible(false);
        m_pDetailPageRoot->Set_Visible(true);

        if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("OpenDetail"))
        {
            m_ePageTransitionState = PAGE_TRANSITION_STATE::OPENING_DETAIL;
            m_pUIObject->Play_Animation("OpenDetail", true);
        }
        else
        {
            m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;
        }
        break;

    case PAGE_TRANSITION_STATE::OPENING_DETAIL:
        m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;
        break;

    default:
        break;
    }
}

void CUILegionArmChangeController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    const char* pCloseClipName = m_pViewModel->Is_SelectPage() ? "CloseSelect" : "CloseDetail";

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation(pCloseClipName))
    {
        // 현재 보이는 페이지의 콘텐츠를 퇴장시킨 뒤 전체 Modal을 닫는다.
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation(pCloseClipName, true);
        return;
    }

    Finalize_Close();
}

void CUILegionArmChangeController::Finalize_Close()
{
    if (m_pGameInstance->Is_OpenUIObject(m_hUI))
    {
        // 하위 Modal이 닫히므로 기존 장비창용 카메라와 프리뷰 상태로 복구한다.
        Restore_EquipmentPreview();

        if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
            return;

        if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
            return;
    }

    m_bCloseAnimationPending = false;
    m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;

    if (m_pViewModel != nullptr)
        m_pViewModel->Clear_Dirty();
}

CUILegionArmChangeController* CUILegionArmChangeController::Create()
{
    return new CUILegionArmChangeController;
}

void CUILegionArmChangeController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    m_pInventory = nullptr;

    Safe_Release(m_pPreviewCharacter);
    Safe_Release(m_pViewModel);

    m_pDetailPageRoot = nullptr;
    m_pSelectPageRoot = nullptr;

    m_pEquippedArmButton = nullptr;
    m_pEquippedArmIcon = nullptr;
    m_pLegionArmNameText = nullptr;

    m_pSelectDetailPanel = nullptr;
    m_pSelectDetailTitle = nullptr;
    m_pSelectDetailIcon = nullptr;
    m_pSelectDetailCategory = nullptr;
    m_pSelectDetailBodyText = nullptr;

    m_pDetailPrimaryActionText = nullptr;
    m_pDetailCloseActionText = nullptr;
    m_pSelectCloseActionText = nullptr;

    for (_uint iIndex = 0; iIndex < LEGION_ARM_CHANGE_ABILITY_UI_COUNT; ++iIndex)
    {
        m_pAbilityDescriptionGroups[iIndex] = nullptr;
        m_pAbilityNameTexts[iIndex] = nullptr;
        m_pAbilityDetailTexts[iIndex] = nullptr;
    }

    for (_uint iIndex = 0; iIndex < LEGION_ARM_CHANGE_ARM_COUNT; ++iIndex)
    {
        m_pLegionArmSlots[iIndex] = nullptr;
        m_pLegionArmSlotButtons[iIndex] = nullptr;
        m_pLegionArmSlotIcons[iIndex] = nullptr;
        m_pInstallingIcons[iIndex] = nullptr;
    }

    __super::Free();
}