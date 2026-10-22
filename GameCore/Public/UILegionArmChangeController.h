#pragma once
#include "UIControllerBase.h"
#include "IListener.h"
#include "Core_Enums.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CLegionArmChangeViewModel;
class CEquipmentPreviewCharacter;
class CInventory;

class CUILegionArmChangeController final : public CUIControllerBase, public IListener
{
private:
    enum class PAGE_TRANSITION_STATE
    {
        NONE,
        CLOSING_DETAIL,
        OPENING_SELECT,
        CLOSING_SELECT,
        OPENING_DETAIL
    };

private:
    CUILegionArmChangeController() = default;
    virtual ~CUILegionArmChangeController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    virtual void On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;
    virtual void OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    void Close_LegionArmChange();
    void Apply_DetailPage();
    void Apply_SelectPage();
    void Configure_Buttons();

    HRESULT Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath);

private:
    HRESULT Bind_PreviewCharacter();
    void Begin_LegionArmPreview();
    void Restore_EquipmentPreview();
    LegionArmType Get_PreviewLegionArmType() const;

private:
    HRESULT Bind_PlayerInventory();
    void Sync_FromInventory();
    _bool Equip_FocusedLegionArm();

private:
    void Begin_OpenSelectPage(); // Detail 퇴장 애니메이션부터 시작한다.
    void Begin_OpenDetailPage(); // Select 퇴장 애니메이션부터 시작한다.
    void Update_PageTransition(); // 현재 애니메이션 종료 후 다음 전환 단계를 수행한다.

    void Begin_CloseAnimation(); // 현재 페이지의 Close 애니메이션을 재생한다.
    void Finalize_Close(); // 리전 암 UI를 실제로 닫고 장비창 프리셋으로 복구한다.

private:
    CLegionArmChangeViewModel* m_pViewModel = { nullptr };
    CEquipmentPreviewCharacter* m_pPreviewCharacter = { nullptr };
    CInventory* m_pInventory = { nullptr };

private:
    CUIElement* m_pDetailPageRoot = { nullptr };
    CUIElement* m_pSelectPageRoot = { nullptr };

private:
    CUIButton* m_pEquippedArmButton = { nullptr };
    CUIImage* m_pEquippedArmIcon = { nullptr };
    CUIText* m_pLegionArmNameText = { nullptr };

    CUIElement* m_pAbilityDescriptionGroups[4] = {};
    CUIText* m_pAbilityNameTexts[4] = {};
    CUIText* m_pAbilityDetailTexts[4] = {};

private:
    CUIElement* m_pSelectDetailPanel = { nullptr };
    CUIText* m_pSelectDetailTitle = { nullptr };
    CUIImage* m_pSelectDetailIcon = { nullptr };
    CUIText* m_pSelectDetailCategory = { nullptr };
    CUIText* m_pSelectDetailBodyText = { nullptr };

    CUIElement* m_pLegionArmSlots[3] = {};
    CUIButton* m_pLegionArmSlotButtons[3] = {};
    CUIImage* m_pLegionArmSlotIcons[3] = {};
    CUIImage* m_pInstallingIcons[3] = {};

private:
    CUIText* m_pDetailPrimaryActionText = { nullptr };
    CUIText* m_pDetailCloseActionText = { nullptr };
    CUIText* m_pSelectCloseActionText = { nullptr };

private:
    string m_strAppliedEquippedIconPath;
    string m_strAppliedDetailIconPath;
    string m_strAppliedSlotIconPaths[3];

private:
    PAGE_TRANSITION_STATE m_ePageTransitionState = PAGE_TRANSITION_STATE::NONE;
    _bool m_bCloseAnimationPending = false;

public:
    static CUILegionArmChangeController* Create();
    virtual void Free() override;
};

NS_END