#pragma once
#include "UIControllerBase.h"
#include "EquipmentViewModel.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CUIContainer;
class CUIElement;
class CUIButton;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CInventory;
class CEquipmentPreviewCharacter;
class CEquipmentPreviewBackground;

class CUIEquipmentController final : public CUIControllerBase, public IListener
{
private:
    CUIEquipmentController() = default;
    virtual ~CUIEquipmentController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    virtual void                    On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void                    OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;
    virtual void                    On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;

protected:
    virtual HRESULT                 Bind_UI() override;
    virtual void                    Update_ViewModel(_float fTimeDelta) override;
    virtual void                    Apply_ViewModel() override;

private:
    void                            Close_Equipment();

private:
    HRESULT                         Bind_PlayerInventory();

    void                            Open_CostumeSelect(COSTUME_CATEGORY eCategory);
    void                            Close_CostumeSelect();

    void                            Refresh_CostumeSlots();
    void                            Refresh_MainCostumeSlotIcons();

    void                            Select_CostumeSlot(_uint iSlotIndex);
    void                            Equip_SelectedCostume();
    void                            Unequip_SelectedCostume();

    void                            Apply_CostumeSelectViewModel();

private:
    PLAYER_PART                     Convert_ToPlayerPart(COSTUME_CATEGORY eCategory) const;
    _wstring                        Get_CostumeCategoryText(COSTUME_CATEGORY eCategory) const;
    string                          Resolve_CostumeIconPath(_uint64 iItemID) const;

private:
    HRESULT                         Apply_StaticPresentation();
    void                            Apply_SlotDetail(const _wstring& strRuntimeElementTag);

    HRESULT                         Set_Header(const _wstring& strHeaderTag, const _wstring& strHeaderText, const string& strIconTexturePath);
    HRESULT                         Set_InputHint(const _wstring& strHintTag, const _wstring& strActionText, const string& strKeyTexturePath);
    HRESULT                         Set_ImageTexture(const _wstring& strImageTag, const string& strTexturePath);

    HRESULT                         Bind_PreviewObjects();
    void                            Begin_EquipmentPreview();
    void                            End_EquipmentPreview();
    void                            Update_PreviewRotation();

    void                            Apply_ErgoProfile();
    _wstring                        Format_Number(_uint iValue) const;

    void                            Sync_PreviewCostumes(); // 실제 플레이어의 현재 코스튬 장착 상태를 프리뷰 캐릭터에 복사한다.

    void                            Select_DefaultSlotForCurrentCategory(); // 현재 장비 카테고리의 첫 유효 슬롯과 상세 패널을 기본 선택한다.


    void                            Refresh_MainEquipmentSlots(); // 실제 인벤토리의 2번 무기와 현재 장착 리전 암을 메인 장비 슬롯에 반영한다.
    void                            Select_MainDetailSlot(const _wstring& strRuntimeElementTag); // 클릭한 메인 슬롯만 선택하고 해당 상세 패널을 표시한다.

    string                          Resolve_WeaponIconPath(_uint64 iItemID) const; // 무기 ID를 UI 아이콘 경로로 변환한다.
    string                          Resolve_LegionArmIconPath(_uint64 iItemID) const; // 리전 암 ID를 UI 아이콘 경로로 변환한다.

private:
    CEquipmentViewModel*            m_pViewModel = { nullptr };

    CEquipmentPreviewCharacter*     m_pPreviewCharacter = { nullptr };
    CEquipmentPreviewBackground*    m_pPreviewBackground = { nullptr };

    CInventory*                     m_pInventory = { nullptr };

private:
    CUIElement*                     m_pPages[3] = {};
    CUIButton*                      m_pCategoryButtons[3] = {};

    CUIElement*                     m_pCategoryTabGroup = { nullptr };

    CUIElement*                     m_pDetailPanel = { nullptr };
    CUIElement*                     m_pInputHintActionGroup = { nullptr };
    CUIElement*                     m_pInputHintCompactGroup = { nullptr };
    CUIElement*                     m_pCharacterPreviewArea = { nullptr };

    CUIText*                        m_pDetailTitle = { nullptr };
    CUIImage*                       m_pDetailIcon = { nullptr };
    CUIText*                        m_pDetailCategory = { nullptr };
    CUIText*                        m_pDetailBodyText = { nullptr };

    CUIElement*                     m_pDetailText = { nullptr };
    CUIElement*                     m_pWeaponType1 = { nullptr };
    CUIElement*                     m_pWeaponType2 = { nullptr };
    CUIElement*                     m_pWeaponTypeDetailText = { nullptr };

    CUIImage*                       m_pDetailIconWeapon = { nullptr };
    CUIImage*                       m_pDetailIconGrip = { nullptr };
    CUIImage*                       m_pDetailIconWeapon2 = { nullptr };

    CUIText*                        m_pWeaponTitleText = { nullptr };
    CUIText*                        m_pWeaponBodyText = { nullptr };

private:
    CUIElement*                     m_pCostumeSelectPage = { nullptr };
    CUIElement*                     m_pCostumePreviewArea = { nullptr };

    CUIButton*                      m_pCostumeCategoryButtons[3] = {};

    CUIElement*                     m_pCostumeSlotRoots[3] = {};
    CUIButton*                      m_pCostumeSlotButtons[3] = {};
    CUIImage*                       m_pCostumeSlotIcons[3] = {};
    CUIImage*                       m_pCostumeEquippedIcons[3] = {};

    CUIElement*                     m_pCostumeDetailPanel = { nullptr };
    CUIText*                        m_pCostumeDetailTitle = { nullptr };
    CUIImage*                       m_pCostumeDetailIcon = { nullptr };
    CUIText*                        m_pCostumeDetailCategory = { nullptr };
    CUIText*                        m_pCostumeDetailBodyText = { nullptr };
    CUIText*                        m_pCostumeCategoryText = { nullptr };

    CUIElement*                     m_pCostumeBackHint = { nullptr };
    CUIElement*                     m_pCostumeEquipHint = { nullptr };
    CUIElement*                     m_pCostumeUnequipHint = { nullptr };

private:
    _bool                           m_bHUDLayerSuppressed = { false };
    _bool                           m_bPreviewDragging = { false };
    _bool                           m_bCloseAfterCameraRestore = { false };

    _bool                           m_bWasTopInputOwner = { false }; // 하위 Modal이 닫혀 장비창이 다시 최상위가 된 순간을 감지한다.

private:
    CUIElement*                     m_pErgoProfile = { nullptr };
    CUIText*                        m_pErgoCountText = { nullptr };
    CUIText*                        m_pErgoGetText = { nullptr };

private:
    CUIButton*                      m_pSecondaryWeaponButton = { nullptr };
    CUIImage*                       m_pSecondaryWeaponIcon = { nullptr };
        
    CUIButton*                      m_pLegionArmButton = { nullptr };
    CUIImage*                       m_pLegionArmIcon = { nullptr }; 

private:
    _bool                           m_bCostumeSelectOpening = { false }; // 기존 CostumePage의 퇴장 애니메이션을 기다리는 상태입니다.

public:
    static CUIEquipmentController* Create();
    virtual void Free() override;
};

NS_END