#include "UIShopController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "DialogueData.h"
#include "Player.h"
#include "Inventory.h"
#include "ItemTable.h"
#include "ConfirmationWindowData.h"

HRESULT CUIShopController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CShopViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    // NPC 대화창에서 상점 서비스를 선택했을 때 전달되는 이벤트를 구독한다.
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_UI::CONFIRMATION_RESULT, this);

    m_pPurchaseActionText->Set_Text(L"구매");
    m_pCloseActionText->Set_Text(L"닫기");
    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);

    return S_OK;
}

void CUIShopController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "ShowMaskShopPage")
    {
        m_pViewModel->Set_Category(SHOP_CATEGORY::MASK);
        Refresh_Stock();
    }
    else if (Event.strCommandKey == "ShowAccessoryShopPage")
    {
        m_pViewModel->Set_Category(SHOP_CATEGORY::ACCESSORY);
        Refresh_Stock();
    }
    else if (Event.strCommandKey == "ShowCostumeShopPage")
    {
        m_pViewModel->Set_Category(SHOP_CATEGORY::COSTUME);
        Refresh_Stock();
    }
    else if (Event.strCommandKey == "PrevShopCategory")
    {
        m_pViewModel->Move_Category(-1);
        Refresh_Stock();
    }
    else if (Event.strCommandKey == "NextShopCategory")
    {
        m_pViewModel->Move_Category(1);
        Refresh_Stock();
    }
    else if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"PurchaseHint.HintButtonSpace")
            Request_PurchaseConfirmation();
        else if (Event.strRuntimeElementTag == L"CloseHint.HintButton")
            Close_Shop();
    }
    else if (Event.strCommandKey == "ShopItemSlot")
    {
        const _int iSlotIndex = Resolve_SlotIndex(Event.strRuntimeElementTag);

        if (iSlotIndex >= 0)
            m_pViewModel->Select_Slot(iSlotIndex);
    }
}

void CUIShopController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    UNREFERENCED_PARAMETER(Event);

    // 호버 이미지는 CUIButton이 자체 처리한다.
    // 실제 상품 선택은 On_UICommand에서만 처리한다.
}

void CUIShopController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST || pData == nullptr)
        return;

    NPC_DIALOGUE_SERVICE_REQUEST_DESC* pRequestDesc = static_cast<NPC_DIALOGUE_SERVICE_REQUEST_DESC*>(pData);

    if (pRequestDesc->eServiceType != NPC_DIALOGUE_SERVICE_TYPE::SHOP_BUY)
        return;

    Open_Shop();
}

void CUIShopController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_UI::CONFIRMATION_RESULT || pData == nullptr || m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    CONFIRMATION_RESULT_DESC* pResultDesc = static_cast<CONFIRMATION_RESULT_DESC*>(pData);

    if (pResultDesc->eActionType != CONFIRMATION_ACTION_TYPE::SHOP_PURCHASE || !pResultDesc->bConfirmed)
        return;

    const SHOP_SLOT_DATA* pSelectedSlotData = m_pViewModel->Get_SelectedSlotData();

    // 확인창을 연 상품과 현재 선택 상품이 같은지 다시 검증한다.
    if (pSelectedSlotData == nullptr || pSelectedSlotData->iItemID != pResultDesc->iItemID)
        return;

    // Buy 내부에서 가격 확인, 에르고 차감, 아이템 획득까지 처리한다.
    if (!m_pInventory->Buy(pResultDesc->iItemID))
    {
        m_pViewModel->Set_CurrentErgo(m_pInventory->Get_Ergo());
        return;
    }

    // 구매된 상품을 품절 처리하고 에르고 프로필도 갱신한다.
    Refresh_Stock();
}

HRESULT CUIShopController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pCategoryButtons[0] = Find_ElementAs<CUIButton>(L"CategoryMaskButton");
    m_pCategoryButtons[1] = Find_ElementAs<CUIButton>(L"CategoryAccessoryButton");
    m_pCategoryButtons[2] = Find_ElementAs<CUIButton>(L"CategoryCostumeButton");

    for (_uint iSlotIndex = 0; iSlotIndex < SHOP_SLOT_COUNT; ++iSlotIndex)
    {
        const _wstring strSlotRootTag = L"ShopSlot" + to_wstring(iSlotIndex + 1);

        m_pSlotRoots[iSlotIndex] = Find_Element(strSlotRootTag);
        m_pSlotButtons[iSlotIndex] = Find_ElementAs<CUIButton>(strSlotRootTag + L".ShopSlotButton");
        m_pSlotIcons[iSlotIndex] = Find_ElementAs<CUIImage>(strSlotRootTag + L".ShopItemIcon");
        m_pSlotTexts[iSlotIndex] = Find_ElementAs<CUIText>(strSlotRootTag + L".ShopSlotText");
        m_pSoldOutImages[iSlotIndex] = Find_ElementAs<CUIImage>(strSlotRootTag + L".SoldOut");

        if (m_pSlotRoots[iSlotIndex] == nullptr || m_pSlotButtons[iSlotIndex] == nullptr || m_pSlotIcons[iSlotIndex] == nullptr || m_pSlotTexts[iSlotIndex] == nullptr || m_pSoldOutImages[iSlotIndex] == nullptr)
            return E_FAIL;
    }

    m_pDetailPanel = Find_Element(L"DetailPanel");
    m_pDetailTitle = Find_ElementAs<CUIText>(L"DetailTitle");
    m_pDetailIcon = Find_ElementAs<CUIImage>(L"DetailIcon");
    m_pDetailCategory = Find_ElementAs<CUIText>(L"DetailCategory");
    m_pDetailBodyText = Find_ElementAs<CUIText>(L"DetailBodyText");

    m_pPurchaseHint = Find_Element(L"PurchaseHint");
    m_pPurchaseActionText = Find_ElementAs<CUIText>(L"PurchaseHint.ActionText");
    m_pCloseHint = Find_Element(L"CloseHint");
    m_pCloseActionText = Find_ElementAs<CUIText>(L"CloseHint.ActionText");

    m_pErgoProfile = Find_Element(L"ErgoProfile");
    m_pErgoCountText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoCount");
    m_pErgoGetText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoGet");

    for (_uint iCategoryIndex = 0; iCategoryIndex < 3; ++iCategoryIndex)
    {
        if (m_pCategoryButtons[iCategoryIndex] == nullptr)
            return E_FAIL;
    }

    if (m_pDetailPanel == nullptr || m_pDetailTitle == nullptr || m_pDetailIcon == nullptr || m_pDetailCategory == nullptr || m_pDetailBodyText == nullptr)
        return E_FAIL;

    if (m_pPurchaseHint == nullptr || m_pPurchaseActionText == nullptr || m_pCloseHint == nullptr || m_pCloseActionText == nullptr)
        return E_FAIL;

    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUIShopController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    // Close 재생 중에는 상점 입력을 막고 종료 시점만 검사한다.
    if (m_bCloseAnimationPending)
    {
        if (m_pUIObject != nullptr && m_pUIObject->Is_AnimationFinished())
            Finalize_Close();

        return;
    }

    if (!m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Close_Shop();
    }
    else if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        Request_PurchaseConfirmation();
    }
    else if (m_pGameInstance->Key_Down(DIK_Q))
    {
        m_pViewModel->Move_Category(-1);
        Refresh_Stock();
    }
    else if (m_pGameInstance->Key_Down(DIK_E))
    {
        m_pViewModel->Move_Category(1);
        Refresh_Stock();
    }
}

void CUIShopController::Apply_ViewModel()
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
                m_pViewModel->Close_Shop();
                return;
            }

            // NPC 대화창 위에 전체 화면 Modal로 상점 UI를 올린다.
            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pViewModel->Close_Shop();
                return;
            }

            if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            // UI를 즉시 끄지 않고 Close 애니메이션 완료를 기다린다.
            Begin_CloseAnimation();
            return;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    Apply_Category();
    Apply_ErgoProfile();
    Apply_Slots();
    Apply_Detail();
    Apply_InputHints();

    m_pViewModel->Clear_Dirty();
}

HRESULT CUIShopController::Bind_PlayerInventory()
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

    if (pPlayer == nullptr)
        return E_FAIL;

    m_pInventory = pPlayer->Get_Inventory();

    if (m_pInventory == nullptr)
        return E_FAIL;

    return S_OK;
}

void CUIShopController::Open_Shop()
{
    if (m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;

    if (FAILED(Bind_PlayerInventory()))
        return;

    m_pViewModel->Open_Shop();
    Refresh_Stock();
}

void CUIShopController::Close_Shop()
{
    if (m_pViewModel != nullptr)
        m_pViewModel->Close_Shop();
}

void CUIShopController::Refresh_Stock()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    // 상점 오픈, 카테고리 변경, 구매 완료 후마다 실제 보유 에르고를 다시 읽는다.
    m_pViewModel->Set_CurrentErgo(m_pInventory->Get_Ergo());

    const SHOP_CATEGORY eCategory = m_pViewModel->Get_Category();
    const vector<_uint64> ProductIDs = Get_CategoryProductIDs(eCategory);

    m_pViewModel->Clear_Slots();

    const _uint iProductCount = min(static_cast<_uint>(ProductIDs.size()), SHOP_SLOT_COUNT);
    _int iFirstVisibleSlotIndex = -1;

    for (_uint iSlotIndex = 0; iSlotIndex < iProductCount; ++iSlotIndex)
    {
        const _uint64 iItemID = ProductIDs[iSlotIndex];
        const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

        if (pItemData == nullptr)
            continue;

        SHOP_SLOT_DATA SlotData = {};

        SlotData.iItemID = iItemID;
        SlotData.iPrice = pItemData->iPrice;
        SlotData.bVisible = true;
        SlotData.bSoldOut = m_pInventory->Is_Acquired(iItemID);
        SlotData.strName = pItemData->strDisplayName;
        SlotData.strCategory = Get_CategoryText(eCategory);
        SlotData.strDescription = pItemData->strDesc;
        SlotData.strIconTexturePath = Resolve_ItemIconPath(iItemID);

        m_pViewModel->Set_SlotData(iSlotIndex, SlotData);

        // 상품 테이블이 일부 비어 있어도 첫 번째로 실제 생성된 슬롯을 기억한다.
        if (iFirstVisibleSlotIndex < 0)
            iFirstVisibleSlotIndex = static_cast<_int>(iSlotIndex);
    }

    // 품절 여부와 관계없이 첫 번째로 표시되는 상품을 기본 선택한다.
    if (iFirstVisibleSlotIndex >= 0)
        m_pViewModel->Select_Slot(iFirstVisibleSlotIndex);
}

void CUIShopController::Apply_Category()
{
    const _uint iCurrentCategory = static_cast<_uint>(m_pViewModel->Get_Category());

    for (_uint iCategoryIndex = 0; iCategoryIndex < 3; ++iCategoryIndex)
        m_pCategoryButtons[iCategoryIndex]->Set_Selected(iCategoryIndex == iCurrentCategory);
}

void CUIShopController::Apply_Slots()
{
    const _int iSelectedSlotIndex = m_pViewModel->Get_SelectedSlotIndex();

    for (_uint iSlotIndex = 0; iSlotIndex < SHOP_SLOT_COUNT; ++iSlotIndex)
    {
        const SHOP_SLOT_DATA& SlotData = m_pViewModel->Get_SlotData(iSlotIndex);
        const _bool bVisible = SlotData.bVisible;

        m_pSlotRoots[iSlotIndex]->Set_Visible(bVisible);
        m_pSlotButtons[iSlotIndex]->Set_Interactable(bVisible);
        m_pSlotButtons[iSlotIndex]->Set_Selected(bVisible && static_cast<_int>(iSlotIndex) == iSelectedSlotIndex);

        if (!bVisible)
        {
            m_pSlotIcons[iSlotIndex]->Set_Visible(false);
            m_pSoldOutImages[iSlotIndex]->Set_Visible(false);
            m_pSlotTexts[iSlotIndex]->Set_Text(L"");
            continue;
        }

        const _wstring strIconTag = L"ShopSlot" + to_wstring(iSlotIndex + 1) + L".ShopItemIcon";

        Set_ImageTexture(m_pSlotIcons[iSlotIndex], strIconTag, SlotData.strIconTexturePath, m_strAppliedSlotIconPaths[iSlotIndex]);

        m_pSoldOutImages[iSlotIndex]->Set_Visible(SlotData.bSoldOut);

        if (SlotData.bSoldOut)
            m_pSlotTexts[iSlotIndex]->Set_Text(L"품절");
        else
            m_pSlotTexts[iSlotIndex]->Set_Text(to_wstring(SlotData.iPrice));

        m_pSlotIcons[iSlotIndex]->Set_Color(SlotData.bSoldOut ? _float4(1.f, 1.f, 1.f, 0.35f) : _float4(1.f, 1.f, 1.f, 1.f));
    }
}

void CUIShopController::Apply_Detail()
{
    // 상세 패널은 Hover 상품이 아니라 직접 선택한 상품만 사용한다.
    const SHOP_SLOT_DATA* pSelectedSlotData = m_pViewModel->Get_SelectedSlotData();
    m_pDetailPanel->Set_Visible(pSelectedSlotData != nullptr);

    if (pSelectedSlotData == nullptr)
        return;

    m_pDetailTitle->Set_Text(pSelectedSlotData->strName);
    m_pDetailCategory->Set_Text(pSelectedSlotData->strCategory);
    m_pDetailBodyText->Set_Text(pSelectedSlotData->strDescription);

    Set_ImageTexture(m_pDetailIcon, L"DetailIcon", pSelectedSlotData->strIconTexturePath, m_strAppliedDetailIconPath);
}

void CUIShopController::Apply_InputHints()
{
    const SHOP_SLOT_DATA* pSelectedSlotData = m_pViewModel->Get_SelectedSlotData();

    // 아직 선택하지 않았거나 품절된 상품에는 구매 입력을 표시하지 않는다.
    const _bool bCanRequestPurchase = pSelectedSlotData != nullptr && !pSelectedSlotData->bSoldOut;

    m_pPurchaseHint->Set_Visible(bCanRequestPurchase);
    m_pCloseHint->Set_Visible(true);
}

void CUIShopController::Apply_ErgoProfile()
{
    if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
        return;

    m_pErgoProfile->Set_Visible(m_pViewModel->Is_Visible());
    m_pErgoCountText->Set_Text(Format_Number(m_pViewModel->Get_CurrentErgo()));

    // 상점에서는 에르고 획득량 표시를 사용하지 않는다.
    m_pErgoGetText->Set_Text(L"");
    m_pErgoGetText->Set_Visible(false);
}

void CUIShopController::Request_PurchaseConfirmation()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    const SHOP_SLOT_DATA* pSelectedSlotData = m_pViewModel->Get_SelectedSlotData();

    if (pSelectedSlotData == nullptr || pSelectedSlotData->bSoldOut)
        return;

    CONFIRMATION_REQUEST_DESC RequestDesc = {};
    RequestDesc.eActionType = CONFIRMATION_ACTION_TYPE::SHOP_PURCHASE;
    RequestDesc.strTitle = L"구매 확인";
    RequestDesc.strGoodsName = pSelectedSlotData->strName;
    RequestDesc.strConfirmText = L"해당 상품을 구매하시겠습니까?";
    RequestDesc.strConfirmActionText = L"구매";
    RequestDesc.strCancelActionText = L"취소";
    RequestDesc.strPriceLabel = L"가격";
    RequestDesc.iCurrentErgo = m_pInventory->Get_Ergo();
    RequestDesc.iPrice = pSelectedSlotData->iPrice;
    RequestDesc.bShowPrice = true;
    RequestDesc.iItemID = pSelectedSlotData->iItemID;

    // 실제 구매하지 않고 공용 확인창만 요청한다.
    m_pGameInstance->Broadcast(EVENT_TYPE_UI::CONFIRMATION_OPEN_REQUEST, &RequestDesc);
}

_wstring CUIShopController::Format_Number(_uint iValue) const
{
    _wstring strValue = to_wstring(iValue);

    // 문자열 뒤에서부터 세 자리마다 쉼표를 삽입한다.
    for (_int iPosition = static_cast<_int>(strValue.length()) - 3; iPosition > 0; iPosition -= 3)
        strValue.insert(static_cast<size_t>(iPosition), 1, L',');

    return strValue;
}

_int CUIShopController::Resolve_SlotIndex(const _wstring& strRuntimeElementTag) const
{
    for (_uint iSlotIndex = 0; iSlotIndex < SHOP_SLOT_COUNT; ++iSlotIndex)
    {
        const _wstring strExpectedTag = L"ShopSlot" + to_wstring(iSlotIndex + 1) + L".ShopSlotButton";

        if (strRuntimeElementTag == strExpectedTag)
            return static_cast<_int>(iSlotIndex);
    }

    return -1;
}

_wstring CUIShopController::Get_CategoryText(SHOP_CATEGORY eCategory) const
{
    switch (eCategory)
    {
    case SHOP_CATEGORY::MASK:
        return L"가면";

    case SHOP_CATEGORY::ACCESSORY:
        return L"악세서리";

    case SHOP_CATEGORY::COSTUME:
        return L"의상";

    default:
        return L"";
    }
}

string CUIShopController::Resolve_ItemIconPath(_uint64 iItemID) const
{
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

vector<_uint64> CUIShopController::Get_CategoryProductIDs(SHOP_CATEGORY eCategory) const
{
    switch (eCategory)
    {
    case SHOP_CATEGORY::MASK:
        return
        {
            ItemID::HELMET_ALCHEMIST,
            ItemID::HELMET_MADDONKEY,
            ItemID::HELMET_SANTA
        };

    case SHOP_CATEGORY::ACCESSORY:
        return
        {
            ItemID::ACC_EYEPATCH,
            ItemID::ACC_RUDOLF_NOSE
        };

    case SHOP_CATEGORY::COSTUME:
        return
        {
            ItemID::BODY_WHITE
        };

    default:
        return {};
    }
}

HRESULT CUIShopController::Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath)
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

void CUIShopController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Close"))
    {
        // ViewModel은 닫힌 상태지만 실제 UI는 유지하고 페이드 아웃을 재생한다.
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    Finalize_Close();
}

void CUIShopController::Finalize_Close()
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

CUIShopController* CUIShopController::Create()
{
    return new CUIShopController;
}

void CUIShopController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    Safe_Release(m_pViewModel);

    __super::Free();
}