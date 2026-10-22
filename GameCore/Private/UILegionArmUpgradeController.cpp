#include "UILegionArmUpgradeController.h"
#include "GameInstance.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "LegionArmUpgradeViewModel.h"
#include "DialogueData.h"
#include "Player.h"
#include "Inventory.h"
#include "ConfirmationWindowData.h"
#include "UIObject.h"

HRESULT CUILegionArmUpgradeController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CLegionArmUpgradeViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::CONFIRMATION_RESULT, this);

    m_pUpgradeCloseActionText->Set_Text(L"닫기");
    m_pUpgradeSpaceActionText->Set_Text(L"레벨 업");

    return S_OK;
}

void CUILegionArmUpgradeController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_bCloseAnimationPending)
        return;

    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "SelectLegionArmUpgradeCategory")
    {
        const _int iArmIndex = Resolve_ArmIndex(Event.strRuntimeElementTag);

        if (iArmIndex >= 0)
            m_pViewModel->Set_FocusedArmIndex(static_cast<_uint>(iArmIndex));
    }
    else if (Event.strCommandKey == "SelectLegionArmUpgradeAbility")
    {
        const _int iStageIndex = Resolve_StageIndex(Event.strRuntimeElementTag);

        if (iStageIndex >= 0)
            m_pViewModel->Set_FocusedStageIndex(static_cast<_uint>(iStageIndex));
    }
    else if (Event.strCommandKey == "PreviousLegionArmUpgradeCategory")
    {
        m_pViewModel->Move_FocusedArm(-1);
    }
    else if (Event.strCommandKey == "NextLegionArmUpgradeCategory")
    {
        m_pViewModel->Move_FocusedArm(1);
    }
    else if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"UpgradeSpaceKey.HintButtonSpace")
            Try_UpgradeFocusedStage();
        else if (Event.strRuntimeElementTag == L"UpgradeCloseKey.HintButton")
            Close_LegionArmUpgrade();
    }
}

void CUILegionArmUpgradeController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI || !Event.bFocused)
        return;

    if (Event.strFocusGroup == "LegionArmUpgradeCategory")
    {
        const _int iArmIndex = Resolve_ArmIndex(Event.strRuntimeElementTag);

        if (iArmIndex >= 0)
            m_pViewModel->Set_FocusedArmIndex(static_cast<_uint>(iArmIndex));
    }
}

void CUILegionArmUpgradeController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (m_bCloseAnimationPending || m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    NPC_DIALOGUE_SERVICE_REQUEST_DESC* pRequestDesc = static_cast<NPC_DIALOGUE_SERVICE_REQUEST_DESC*>(pData);

    if (pRequestDesc->eServiceType != NPC_DIALOGUE_SERVICE_TYPE::LEGION_ARM_UPGRADE)
        return;

    if (m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    // NPC 대화창 위에 전체 화면 Modal로 리전 암 강화 UI를 올린다.
    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;

    if (FAILED(Bind_PlayerInventory()))
        return;

    Sync_FromInventory();
    m_pViewModel->Open_LegionArmUpgrade();
}

void CUILegionArmUpgradeController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_UI::CONFIRMATION_RESULT || pData == nullptr || m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    CONFIRMATION_RESULT_DESC* pResultDesc = static_cast<CONFIRMATION_RESULT_DESC*>(pData);

    if (pResultDesc->eActionType != CONFIRMATION_ACTION_TYPE::LEGION_ARM_UPGRADE || !pResultDesc->bConfirmed)
        return;

    // 확인창을 열었던 리전 암과 강화 단계가 여전히 선택되어 있는지 검증한다.
    if (m_pViewModel->Get_FocusedArmIndex() != pResultDesc->iArmIndex || m_pViewModel->Get_FocusedStageIndex() != pResultDesc->iUpgradeStage)
        return;

    if (!m_pViewModel->Can_UpgradeFocusedStage())
        return;

    const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_ArmData(pResultDesc->iArmIndex);
    const _uint iUpgradeCost = m_pViewModel->Get_FocusedUpgradeCost();

    if (pResultDesc->iUpgradeStage != LEGION_ARM_UPGRADED_LEVEL)
        return;

    if (!m_pInventory->Upgrade_LegionArm(ArmData.iItemID, iUpgradeCost))
    {
        Sync_FromInventory();
        return;
    }

    m_pGameInstance->Play_Sound("SE_UI_SlaveArmSlot_Active", SOUND_SFX, 1.f, false, false);

    Sync_FromInventory();
    m_pViewModel->Set_FocusedStageIndex(LEGION_ARM_UPGRADED_LEVEL);
}

HRESULT CUILegionArmUpgradeController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pPreviousCategoryButton = Find_ElementAs<CUIButton>(L"ArmCategoryPrevKey");
    
    m_pNextCategoryButton = Find_ElementAs<CUIButton>(L"ArmCategoryNextKey");

    m_pUpgradePreviewTitle = Find_ElementAs<CUIText>(L"UpgradePreviewTitle");
    m_pUpgradePreviewDescription = Find_ElementAs<CUIText>(L"UpgradePreviewDescription");

    m_pUpgradeCostRoot = Find_Element(L"UpgradeCostRoot");
    m_pPriceText = Find_ElementAs<CUIText>(L"Price");

    m_pUpgradeCompleteRoot = Find_Element(L"UpgradeCompleteRoot");
    m_pUpgradeCompleteTitle = Find_ElementAs<CUIText>(L"UpgradeCompleteTitle");
    m_pUpgradeCompleteDescription = Find_ElementAs<CUIText>(L"UpgradeCompleteDescription");

    m_pUpgradeCloseKey = Find_Element(L"UpgradeCloseKey");
    m_pUpgradeCloseActionText = Find_ElementAs<CUIText>(L"UpgradeCloseKey.ActionText");
    m_pUpgradeSpaceKey = Find_Element(L"UpgradeSpaceKey");
    m_pUpgradeSpaceActionText = Find_ElementAs<CUIText>(L"UpgradeSpaceKey.ActionText");

    m_pErgoProfile = Find_Element(L"ErgoProfile");
    m_pErgoCountText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoCount");
    m_pErgoGetText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoGet");

    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return E_FAIL;

    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);
    if (m_pPreviousCategoryButton == nullptr || m_pNextCategoryButton == nullptr || m_pUpgradePreviewTitle == nullptr || m_pUpgradePreviewDescription == nullptr)
        return E_FAIL;

    if (m_pUpgradeCostRoot == nullptr || m_pPriceText == nullptr || m_pUpgradeCompleteRoot == nullptr || m_pUpgradeCompleteTitle == nullptr || m_pUpgradeCompleteDescription == nullptr)
        return E_FAIL;

    if (m_pUpgradeCloseKey == nullptr || m_pUpgradeCloseActionText == nullptr || m_pUpgradeSpaceKey == nullptr || m_pUpgradeSpaceActionText == nullptr)
        return E_FAIL;

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_UPGRADE_ARM_COUNT; ++iArmIndex)
    {
        const _wstring strIndex = to_wstring(iArmIndex + 1);

        m_pArmCategoryButtons[iArmIndex] = Find_ElementAs<CUIButton>(L"ArmCategoryButton" + strIndex);
        m_pArmCategoryIcons[iArmIndex] = Find_ElementAs<CUIImage>(L"ArmCategoryIcon" + strIndex);
        m_pArmCategoryEquippedMarks[iArmIndex] = Find_ElementAs<CUIImage>(L"ArmCategoryEquippedMark" + strIndex);

        if (m_pArmCategoryButtons[iArmIndex] == nullptr || m_pArmCategoryIcons[iArmIndex] == nullptr || m_pArmCategoryEquippedMarks[iArmIndex] == nullptr)
            return E_FAIL;
    }

    for (_uint iStageIndex = 0; iStageIndex < LEGION_ARM_UPGRADE_STAGE_COUNT; ++iStageIndex)
    {
        const _wstring strIndex = to_wstring(iStageIndex);

        // 좌측 강화 단계 마크
        m_pUpgradeStages[iStageIndex] = Find_Element(L"UpgradeStage" + strIndex);
        m_pUpgradeStageRings[iStageIndex] = Find_ElementAs<CUIImage>(L"UpgradeStageRing" + strIndex);
        m_pUpgradeStageNumbers[iStageIndex] = Find_ElementAs<CUIImage>(L"UpgradeStageNumber" + strIndex);

        // 중앙 강화 능력 버튼
        m_pUpgradeAbilityButtons[iStageIndex] = Find_ElementAs<CUIButton>(L"UpgradeAbilityButton" + strIndex);
        m_pUpgradeAbilityBackgrounds[iStageIndex] = Find_ElementAs<CUIImage>(L"UpgradeAbilityBackground" + strIndex);
        m_pUpgradeAbilityLocks[iStageIndex] = Find_ElementAs<CUIImage>(L"UpgradeAbilityLock" + strIndex);
        m_pUpgradeAbilityNames[iStageIndex] = Find_ElementAs<CUIText>(L"UpgradeAbilityName" + strIndex);

        if (m_pUpgradeStages[iStageIndex] == nullptr || m_pUpgradeStageRings[iStageIndex] == nullptr || m_pUpgradeStageNumbers[iStageIndex] == nullptr)
            return E_FAIL;

        if (m_pUpgradeAbilityButtons[iStageIndex] == nullptr || m_pUpgradeAbilityBackgrounds[iStageIndex] == nullptr || m_pUpgradeAbilityLocks[iStageIndex] == nullptr || m_pUpgradeAbilityNames[iStageIndex] == nullptr)
            return E_FAIL;
    }
    for (_uint iConnectorIndex = 0; iConnectorIndex < LEGION_ARM_UPGRADE_STAGE_COUNT - 1; ++iConnectorIndex)
    {
        const _wstring strIndex = to_wstring(iConnectorIndex);

        m_pUpgradeStageConnectors[iConnectorIndex] = Find_Element(L"UpgradeStageConnector" + strIndex);
        m_pUpgradeStageConnectorActives[iConnectorIndex] = Find_ElementAs<CUIImage>(L"UpgradeStageConnectorActive" + strIndex);

        if (m_pUpgradeStageConnectors[iConnectorIndex] == nullptr || m_pUpgradeStageConnectorActives[iConnectorIndex] == nullptr)
            return E_FAIL;
    }

    return S_OK;
}

void CUILegionArmUpgradeController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    // Close 재생 중에는 다른 입력을 막고 애니메이션 완료만 검사한다.
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
        Close_LegionArmUpgrade();
    }
    else if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        Try_UpgradeFocusedStage();
    }
    else if (m_pGameInstance->Key_Down(DIK_Q))
    {
        m_pViewModel->Move_FocusedArm(-1);
    }
    else if (m_pGameInstance->Key_Down(DIK_E))
    {
        m_pViewModel->Move_FocusedArm(1);
    }
}

void CUILegionArmUpgradeController::Try_UpgradeFocusedStage()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    if (!m_pViewModel->Can_UpgradeFocusedStage())
        return;

    const _uint iArmIndex = m_pViewModel->Get_FocusedArmIndex();
    const _uint iStageIndex = m_pViewModel->Get_FocusedStageIndex();
    const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_FocusedArmData();

    CONFIRMATION_REQUEST_DESC RequestDesc = {};
    RequestDesc.eActionType = CONFIRMATION_ACTION_TYPE::LEGION_ARM_UPGRADE;
    RequestDesc.strTitle = L"강화 확인";
    RequestDesc.strGoodsName = ArmData.strAbilityNames[iStageIndex];
    RequestDesc.strConfirmText = L"선택한 리전 암 능력을 강화하시겠습니까?";
    RequestDesc.strConfirmActionText = L"강화";
    RequestDesc.strCancelActionText = L"취소";
    RequestDesc.strPriceLabel = L"필요 에르고";
    RequestDesc.iCurrentErgo = m_pInventory->Get_Ergo();
    RequestDesc.iPrice = m_pViewModel->Get_FocusedUpgradeCost();
    RequestDesc.bShowPrice = true;
    RequestDesc.iArmIndex = iArmIndex;
    RequestDesc.iUpgradeStage = iStageIndex;

    // 에르고 차감 없이 확인창만 연다.
    m_pGameInstance->Broadcast(EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST, &RequestDesc);
}

void CUILegionArmUpgradeController::Apply_ViewModel()
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
                m_pViewModel->Close_LegionArmUpgrade();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pViewModel->Close_LegionArmUpgrade();
                return;
            }

            // UI가 실제로 열리고 Input Owner 등록까지 성공한 뒤 Open을 재생한다.
            if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            // UI를 즉시 닫지 않고 Close 애니메이션 종료를 기다린다.
            Begin_CloseAnimation();
            return;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    Apply_ArmCategory();
    Apply_UpgradeTree();
    Apply_UpgradeDetail();
    Apply_UpgradeCost();
    Apply_ErgoProfile();

    m_pViewModel->Clear_Dirty();
}

void CUILegionArmUpgradeController::Apply_ArmCategory()
{
    const _uint iFocusedArmIndex = m_pViewModel->Get_FocusedArmIndex();

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_UPGRADE_ARM_COUNT; ++iArmIndex)
    {
        const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_ArmData(iArmIndex);
        const _wstring strIconTag = L"ArmCategoryIcon" + to_wstring(iArmIndex + 1);

        m_pArmCategoryButtons[iArmIndex]->Set_Visible(ArmData.bOwned);
        m_pArmCategoryButtons[iArmIndex]->Set_Interactable(ArmData.bOwned);
        m_pArmCategoryButtons[iArmIndex]->Set_Selected(ArmData.bOwned && iArmIndex == iFocusedArmIndex);
        m_pArmCategoryEquippedMarks[iArmIndex]->Set_Visible(ArmData.bOwned && ArmData.bEquipped);

        if (ArmData.bOwned)
            Set_ImageTexture(m_pArmCategoryIcons[iArmIndex], strIconTag, ArmData.strIconTexturePath, m_strAppliedArmIconPaths[iArmIndex]);
    }
}

void CUILegionArmUpgradeController::Apply_UpgradeTree()
{
    const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_FocusedArmData();
    const _uint iSelectedStageIndex = m_pViewModel->Get_FocusedStageIndex();

    // 원형 단계 마크 색상
    const _float4 vAcquiredMarkColor = _float4(31.f / 255.f, 184.f / 255.f, 199.f / 255.f, 1.f);
    const _float4 vUnacquiredMarkColor = _float4(107.f / 255.f, 107.f / 255.f, 107.f / 255.f, 1.f);

    const _float4 vCompletedBackgroundColor = _float4(175.f / 255.f, 175.f / 255.f, 175.f / 255.f, 1.f);
    const _float4 vAvailableBackgroundColor = _float4(135.f / 255.f, 135.f / 255.f, 135.f / 255.f, 1.f);
    const _float4 vLockedBackgroundColor = _float4(97.f / 255.f, 97.f / 255.f, 97.f / 255.f, 1.f);

    for (_uint iStageIndex = 0; iStageIndex < LEGION_ARM_UPGRADE_STAGE_COUNT; ++iStageIndex)
    {
        const _bool bAcquired = m_pViewModel->Is_StageAcquired(iStageIndex);
        const _bool bAvailable = m_pViewModel->Is_StageAvailable(iStageIndex);
        const _bool bLocked = m_pViewModel->Is_StageLocked(iStageIndex);

        m_pUpgradeStages[iStageIndex]->Set_Visible(ArmData.bOwned);
        m_pUpgradeAbilityButtons[iStageIndex]->Set_Interactable(ArmData.bOwned);
        m_pUpgradeAbilityButtons[iStageIndex]->Set_Selected(iStageIndex == iSelectedStageIndex);

        m_pUpgradeAbilityLocks[iStageIndex]->Set_Visible(bLocked);
        m_pUpgradeAbilityNames[iStageIndex]->Set_Text(ArmData.strAbilityNames[iStageIndex]);

        // 획득한 단계의 원형 마크와 숫자만 청록색으로 표시한다.
        const _float4& vMarkColor = bAcquired ? vAcquiredMarkColor : vUnacquiredMarkColor;
        m_pUpgradeStageRings[iStageIndex]->Set_Color(vMarkColor);
        m_pUpgradeStageNumbers[iStageIndex]->Set_Color(vMarkColor);

        // 완료, 잠금 해제, 잠금 상태에 맞는 배경 Tint를 적용한다.
        if (bAcquired)
            m_pUpgradeAbilityBackgrounds[iStageIndex]->Set_Color(vCompletedBackgroundColor);
        else if (bAvailable)
            m_pUpgradeAbilityBackgrounds[iStageIndex]->Set_Color(vAvailableBackgroundColor);
        else
            m_pUpgradeAbilityBackgrounds[iStageIndex]->Set_Color(vLockedBackgroundColor);
    }

    // 레벨 1이면 첫 번째 연결선, 레벨 2면 두 번째까지 활성화한다.
    for (_uint iConnectorIndex = 0; iConnectorIndex < LEGION_ARM_UPGRADE_STAGE_COUNT - 1; ++iConnectorIndex)
        m_pUpgradeStageConnectorActives[iConnectorIndex]->Set_Visible(ArmData.iUpgradeLevel > iConnectorIndex);
}

void CUILegionArmUpgradeController::Apply_UpgradeDetail()
{
    const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_FocusedArmData();
    const _uint iFocusedStageIndex = m_pViewModel->Get_FocusedStageIndex();

    m_pUpgradePreviewTitle->Set_Text(ArmData.strAbilityNames[iFocusedStageIndex]);
    m_pUpgradePreviewDescription->Set_Text(ArmData.strAbilityDescriptions[iFocusedStageIndex]);
}

void CUILegionArmUpgradeController::Apply_UpgradeCost()
{
    const _uint iSelectedStageIndex = m_pViewModel->Get_FocusedStageIndex();

    const _bool bAllUpgraded = m_pViewModel->Is_AllUpgraded();
    const _bool bHasUpgradeCost = iSelectedStageIndex > 0;
    const _bool bSelectedStageAvailable = m_pViewModel->Is_StageAvailable(iSelectedStageIndex);
    const _bool bCanUpgradeSelectedStage = m_pViewModel->Can_UpgradeFocusedStage();

    m_pUpgradeCompleteRoot->Set_Visible(bAllUpgraded);
    m_pUpgradeCostRoot->Set_Visible(!bAllUpgraded && bHasUpgradeCost);

    // 현재 선택한 단계가 바로 다음 강화 단계이고 에르고가 충분할 때만 표시한다.
    m_pUpgradeSpaceKey->Set_Visible(!bAllUpgraded && bSelectedStageAvailable && bCanUpgradeSelectedStage);
    m_pUpgradeCloseKey->Set_Visible(true);

    m_pUpgradeCompleteTitle->Set_Text(L"개조 완료");
    m_pUpgradeCompleteDescription->Set_Text(L"모든 개조가 완료되었습니다.");

    if (!bAllUpgraded && bHasUpgradeCost)
    {
        // 현재 선택한 슬롯에 해당하는 강화 가격을 표시한다.
        const _uint iSelectedUpgradeCost = m_pViewModel->Get_FocusedUpgradeCost();
        m_pPriceText->Set_Text(to_wstring(iSelectedUpgradeCost));
    }
}

void CUILegionArmUpgradeController::Close_LegionArmUpgrade()
{
    if (m_pViewModel != nullptr)
        m_pViewModel->Close_LegionArmUpgrade();
}

_int CUILegionArmUpgradeController::Resolve_ArmIndex(const _wstring& strRuntimeElementTag) const
{
    if (strRuntimeElementTag == L"ArmCategoryButton1")
        return 0;

    if (strRuntimeElementTag == L"ArmCategoryButton2")
        return 1;

    return -1;
}

_int CUILegionArmUpgradeController::Resolve_StageIndex(const _wstring& strRuntimeElementTag) const
{
    for (_uint iStageIndex = 0; iStageIndex < LEGION_ARM_UPGRADE_STAGE_COUNT; ++iStageIndex)
    {
        if (strRuntimeElementTag == L"UpgradeAbilityButton" + to_wstring(iStageIndex))
            return static_cast<_int>(iStageIndex);
    }

    return -1;
}

HRESULT CUILegionArmUpgradeController::Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath)
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

HRESULT CUILegionArmUpgradeController::Bind_PlayerInventory()
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

    if (pPlayer == nullptr)
        return E_FAIL;

    m_pInventory = pPlayer->Get_Inventory();

    if (m_pInventory == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUILegionArmUpgradeController::Apply_ErgoProfile()
{
    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return;

    m_pErgoProfile->Set_Visible(m_pViewModel->Is_Visible());
    m_pErgoCountText->Set_Text(Format_Number(m_pViewModel->Get_CurrentErgo()));

    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);
}

_wstring CUILegionArmUpgradeController::Format_Number(_uint iValue) const
{
    _wstring strValue = to_wstring(iValue);

    for (_int iPosition = static_cast<_int>(strValue.length()) - 3; iPosition > 0; iPosition -= 3)
        strValue.insert(static_cast<size_t>(iPosition), 1, L',');

    return strValue;
}

void CUILegionArmUpgradeController::Sync_FromInventory()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    for (_uint iArmIndex = 0; iArmIndex < LEGION_ARM_UPGRADE_ARM_COUNT; ++iArmIndex)
    {
        const LEGION_ARM_UPGRADE_UI_DATA& ArmData = m_pViewModel->Get_ArmData(iArmIndex);
        const _uint64 iItemID = ArmData.iItemID;

        m_pViewModel->Set_ArmOwned(iArmIndex, m_pInventory->Is_Acquired(iItemID));
        m_pViewModel->Set_ArmEquipped(iArmIndex, m_pInventory->Is_Equipped(iItemID));

        const _bool bUpgraded = m_pInventory->Is_LegionArm_Upgraded(ArmData.iItemID);
        m_pViewModel->Set_UpgradeLevel(iArmIndex, bUpgraded ? LEGION_ARM_UPGRADED_LEVEL : LEGION_ARM_DEFAULT_LEVEL);
    }

    m_pViewModel->Set_CurrentErgo(m_pInventory->Get_Ergo());
}

void CUILegionArmUpgradeController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Close"))
    {
        // ViewModel은 닫힌 상태지만 실제 UI는 유지하여 페이드 아웃을 보여준다.
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    Finalize_Close();
}

void CUILegionArmUpgradeController::Finalize_Close()
{
    if (m_pGameInstance->Is_OpenUIObject(m_hUI))
    {
        if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
            return;

        if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
            return;
    }

    m_bCloseAnimationPending = false;

    if (m_pViewModel != nullptr)
        m_pViewModel->Clear_Dirty();
}

CUILegionArmUpgradeController* CUILegionArmUpgradeController::Create()
{
    return new CUILegionArmUpgradeController;
}

void CUILegionArmUpgradeController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    m_pInventory = nullptr;
    m_pErgoProfile = nullptr;
    m_pErgoCountText = nullptr;
    m_pErgoGetText = nullptr;

    Safe_Release(m_pViewModel);
    __super::Free();
}