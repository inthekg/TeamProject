#include "UIItemPopupController.h"

#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIImage.h"
#include "UIText.h"
#include "Item.h"
#include "ItemTable.h"

HRESULT CUIItemPopupController::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CItemPopupViewModel::Create();
    if (nullptr == m_pViewModel)
        return E_FAIL;

    /* 플레이어가 받는 것과 동일한 아이템 획득 이벤트를 UI도 구독한다. */
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::GET_ITEM, this);

    return S_OK;
}

HRESULT CUIItemPopupController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pItemImage = Find_ElementAs<CUIImage>(L"Item_Image");
    m_pItemText = Find_ElementAs<CUIText>(L"Item_Text");
    m_pWeaponImageGroup = Find_Element(L"Weapon_ImageGroup");

    if (nullptr == m_pItemImage || nullptr == m_pItemText || nullptr == m_pWeaponImageGroup)
        return E_FAIL;

    m_pItemImage->Set_UseTexture(false);
    m_pItemImage->Set_Visible(false);
    m_pWeaponImageGroup->Set_Visible(false);

    return S_OK;
}

void CUIItemPopupController::OnEvent(EVENT_TYPE_MAP_OBJECT eEventType, Engine::Event_Desc* pEventDesc)
{
    if (eEventType != EVENT_TYPE_MAP_OBJECT::GET_ITEM || nullptr == pEventDesc)
        return;

    const GET_ITEM_EVENT_DESC* pGetItemDesc = static_cast<const GET_ITEM_EVENT_DESC*>(pEventDesc);

    ITEM_POPUP_DATA PopupData{};

    if (false == Build_PopupData(*pGetItemDesc, PopupData))
        return;

    /* 연속으로 획득한 아이템이 기존 팝업을 덮어쓰지 않도록 순서대로 보관한다. */
    m_PendingItems.push_back(PopupData);

    if (m_ePopupState == ITEM_POPUP_STATE::HIDDEN)
        Begin_NextPopup();
}

void CUIItemPopupController::Update_ViewModel(_float fTimeDelta)
{
    switch (m_ePopupState)
    {
    case ITEM_POPUP_STATE::HIDDEN:
        if (false == m_PendingItems.empty())
            Begin_NextPopup();
        break;

    case ITEM_POPUP_STATE::OPENING:
        if (m_pUIObject->Is_AnimationFinished())
        {
            m_ePopupState = ITEM_POPUP_STATE::HOLDING;
            m_fHoldTime = POPUP_HOLD_DURATION;
        }
        break;

    case ITEM_POPUP_STATE::HOLDING:
        m_fHoldTime -= fTimeDelta;

        if (m_fHoldTime <= 0.f)
            Begin_Inactive();
        break;

    case ITEM_POPUP_STATE::CLOSING:
        if (m_pUIObject->Is_AnimationFinished())
            Finish_CurrentPopup();
        break;
    }
}

void CUIItemPopupController::Apply_ViewModel()
{
    if (nullptr == m_pViewModel || false == m_pViewModel->Is_Dirty())
        return;

    if (false == m_pViewModel->Has_Item())
    {
        m_pViewModel->Clear_Dirty();
        return;
    }

    const ITEM_POPUP_DATA& ItemData = m_pViewModel->Get_ItemData();
    const _bool bWeapon = m_pViewModel->Is_Weapon();

    m_pItemText->Set_Text(ItemData.strDisplayText);

    /* 무기는 JSON에 배치한 고정 무기 슬롯을 사용한다. */
    m_pWeaponImageGroup->Set_Visible(bWeapon);

    if (bWeapon)
    {
        m_pItemImage->Set_UseTexture(false);
        m_pItemImage->Set_Visible(false);
    }
    else
    {
        Set_ItemImageTexture(ItemData.strIconTexturePath);
    }

    m_pViewModel->Clear_Dirty();
}

_bool CUIItemPopupController::Build_PopupData(const GET_ITEM_EVENT_DESC& EventDesc, ITEM_POPUP_DATA& OutData) const
{
    /* 에르고는 PlayerHUD의 ErgoGet 연출에서 별도로 표시한다. */
    if (EventDesc.eItemType == ITEM_TYPE::ERGO || EventDesc.eItemType == ITEM_TYPE::END)
        return false;

    const ITEM_DATA* pItemData = CItemTable::Find(EventDesc.iItemID);
    if (nullptr == pItemData)
        return false;

    OutData.eItemType = pItemData->eType;
    OutData.iItemID = EventDesc.iItemID;
    OutData.iItemCount = max<_uint>(1, EventDesc.iItemCount);

    OutData.strDisplayText = pItemData->strDisplayName;

    if (OutData.strDisplayText.empty())
        OutData.strDisplayText = EventDesc.strItemName;

    if (OutData.iItemCount > 1)
    {
        OutData.strDisplayText += L" x ";
        OutData.strDisplayText += to_wstring(OutData.iItemCount);
    }

    if (OutData.eItemType != ITEM_TYPE::WEAPON)
        OutData.strIconTexturePath = Resolve_ItemIconPath(OutData.iItemID);

    return true;
}

string CUIItemPopupController::Resolve_ItemIconPath(_uint64 iItemID) const
{
    if (iItemID == ItemID::LEGION_BASE)
        return "UI/Item/Arm/UIT_Item_Arm_1.dds";

    if (iItemID == ItemID::LEGION_GRAPPLE)
        return "UI/Item/Arm/UIT_Item_Arm_GG_01.dds";

    if (iItemID == ItemID::LEGION_CHARGEGUN)
        return "UI/Item/Arm/UIT_Item_Arm_SniperCannon.dds";

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

    return {};
}

void CUIItemPopupController::Begin_NextPopup()
{
    if (m_PendingItems.empty())
        return;

    const ITEM_POPUP_DATA PopupData = m_PendingItems.front();
    m_PendingItems.pop_front();

    if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
        return;

    m_pViewModel->Set_Item(PopupData);
    Apply_ViewModel();

    /* Active 애니메이션이 없다면 바로 유지 상태로 넘어간다. */
    if (m_pUIObject->Play_Animation("Active", true))
    {
        m_ePopupState = ITEM_POPUP_STATE::OPENING;
    }
    else
    {
        m_ePopupState = ITEM_POPUP_STATE::HOLDING;
        m_fHoldTime = POPUP_HOLD_DURATION;
    }
}

void CUIItemPopupController::Begin_Inactive()
{
    /* Inactive 애니메이션이 없다면 즉시 팝업을 닫는다. */
    if (m_pUIObject->Play_Animation("Inactive", true))
    {
        m_ePopupState = ITEM_POPUP_STATE::CLOSING;
    }
    else
    {
        Finish_CurrentPopup();
    }
}

void CUIItemPopupController::Finish_CurrentPopup()
{
    m_pUIObject->Stop_Animation();

    m_pGameInstance->Close_UIObject(m_hUI);

    m_pViewModel->Clear_Item();
    m_pViewModel->Clear_Dirty();

    m_ePopupState = ITEM_POPUP_STATE::HIDDEN;
    m_fHoldTime = 0.f;
}

HRESULT CUIItemPopupController::Set_ItemImageTexture(const string& strTexturePath)
{
    if (strTexturePath.empty())
    {
        m_pItemImage->Set_UseTexture(false);
        m_pItemImage->Set_Visible(false);
        return S_OK;
    }

    if (m_strAppliedTexturePath != strTexturePath)
    {
        if (FAILED(m_pGameInstance->Set_UIImageTexture(m_hUI, L"Item_Image", strTexturePath)))
            return E_FAIL;

        m_strAppliedTexturePath = strTexturePath;
    }

    m_pItemImage->Set_UseTexture(true);
    m_pItemImage->Set_Visible(true);

    return S_OK;
}

CUIItemPopupController* CUIItemPopupController::Create()
{
    return new CUIItemPopupController;
}

void CUIItemPopupController::Free()
{
    if (nullptr != m_pGameInstance)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    Safe_Release(m_pViewModel);

    __super::Free();
}