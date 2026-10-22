#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CLegionArmUpgradeViewModel;
class CInventory;

class CUILegionArmUpgradeController final : public CUIControllerBase, public IListener
{
private:
    CUILegionArmUpgradeController() = default;
    virtual ~CUILegionArmUpgradeController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    virtual void On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;
    virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
    virtual void OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    void Close_LegionArmUpgrade();
    void Apply_ArmCategory();
    void Apply_UpgradeTree();
    void Apply_UpgradeDetail();
    void Apply_UpgradeCost();

    _int Resolve_ArmIndex(const _wstring& strRuntimeElementTag) const;
    _int Resolve_StageIndex(const _wstring& strRuntimeElementTag) const;

    void Try_UpgradeFocusedStage();
    HRESULT Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath);

    HRESULT Bind_PlayerInventory();
    void    Apply_ErgoProfile();
    _wstring Format_Number(_uint iValue) const;

    void Sync_FromInventory(); // 리전 암 보유·장착·강화 bool과 에르고를 ViewModel에 반영한다.

    void Begin_CloseAnimation(); // 실제 UI를 닫기 전에 Close 애니메이션을 시작한다.
    void Finalize_Close(); // Close 애니메이션 종료 후 Input Owner와 UI를 제거한다.

private:
    CLegionArmUpgradeViewModel* m_pViewModel = { nullptr };

    CUIButton* m_pArmCategoryButtons[2] = {};
    CUIImage* m_pArmCategoryIcons[2] = {};
    CUIImage* m_pArmCategoryEquippedMarks[2] = {};

    CUIButton* m_pPreviousCategoryButton = { nullptr };
    CUIButton* m_pNextCategoryButton = { nullptr };

    CUIElement* m_pUpgradeStages[4] = {};
    CUIButton* m_pUpgradeAbilityButtons[4] = {};
    CUIImage* m_pUpgradeAbilityLocks[4] = {};
    CUIText* m_pUpgradeAbilityNames[4] = {};

    CUIElement* m_pUpgradeStageConnectors[3] = {};
    CUIImage* m_pUpgradeStageConnectorActives[3] = {};

    CUIImage* m_pUpgradeStageRings[4] = {};
    CUIImage* m_pUpgradeStageNumbers[4] = {};
    CUIImage* m_pUpgradeAbilityBackgrounds[4] = {};

    CUIText* m_pUpgradePreviewTitle = { nullptr };
    CUIText* m_pUpgradePreviewDescription = { nullptr };

    CUIElement* m_pUpgradeCostRoot = { nullptr };
    CUIText* m_pPriceText = { nullptr };

    CUIElement* m_pUpgradeCompleteRoot = { nullptr };
    CUIText* m_pUpgradeCompleteTitle = { nullptr };
    CUIText* m_pUpgradeCompleteDescription = { nullptr };

    CUIElement* m_pUpgradeCloseKey = { nullptr };
    CUIText* m_pUpgradeCloseActionText = { nullptr };
    CUIElement* m_pUpgradeSpaceKey = { nullptr };
    CUIText* m_pUpgradeSpaceActionText = { nullptr };

    string m_strAppliedArmIconPaths[2];

    CInventory* m_pInventory = { nullptr }; 

    CUIElement* m_pErgoProfile = { nullptr };
    CUIText* m_pErgoCountText = { nullptr };
    CUIText* m_pErgoGetText = { nullptr };

    _bool m_bCloseAnimationPending = { false }; // Close 애니메이션 종료를 기다리는 상태인지 나타낸다.

public:
    static CUILegionArmUpgradeController* Create();
    virtual void Free() override;
};

NS_END