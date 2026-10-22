#pragma once
#include "UIControllerBase.h"
#include "IListener.h"
#include "Core_Enums.h"
#include "NPCDialogueRepository.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CNPCDialogueViewModel;

class CUINPCDialogueController final : public CUIControllerBase, public IListener
{
private:
    CUINPCDialogueController() = default;
    virtual ~CUINPCDialogueController() = default;

public:
    virtual HRESULT         Initialize(void* pArg) override;

public:
    virtual void            On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void            On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;
    virtual void            OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

protected:
    virtual HRESULT         Bind_UI() override;
    virtual void            Update_ViewModel(_float fTimeDelta) override;
    virtual void            Apply_ViewModel() override;

private:

    void                    Handle_PrimaryAction();
    void                    Select_ServiceSlot(_uint iSlotIndex);
    void                    Request_DialogueService(NPC_DIALOGUE_SERVICE_TYPE eServiceType);
    void                    End_Dialogue();

    void                    Apply_CurrentDialogueLine();
    void                    Advance_Dialogue();
    void                    Enter_ServiceSelection();

    _int                    Resolve_ServiceSlot(const _wstring& strRuntimeElementTag) const;

    void Begin_CloseAnimation(); // NPC 대화창의 Close 애니메이션을 시작합니다.
    void Finalize_Close(); // Close 애니메이션 종료 후 실제 UI와 Input Owner를 제거합니다.

private:
    CNPCDialogueViewModel*  m_pViewModel = { nullptr };

    // DialogueID로 대사 데이터를 조회하는 순수 데이터 저장소
    CNPCDialogueRepository  m_DialogueRepository;

    // 현재 재생 중인 스크립트
    const NPC_DIALOGUE_SCRIPT_DESC* m_pCurrentScript = { nullptr };

    // 현재 화면에 출력 중인 DialogueLines 인덱스
    _uint                   m_iCurrentLineIndex = { 0 };

private:
    CUIText*                m_pSpeakerNameText = { nullptr };
    CUIText*                m_pDialogueBodyText = { nullptr };

    CUIElement*             m_pServiceMenuGroup = { nullptr };
    CUIElement*             m_pServiceOptions[NPC_DIALOGUE_MAX_SERVICE_COUNT] = {};
    CUIButton*              m_pServiceButtons[NPC_DIALOGUE_MAX_SERVICE_COUNT] = {};
    CUIText*                m_pServiceLabels[NPC_DIALOGUE_MAX_SERVICE_COUNT] = {};

    CUIButton*              m_pPrimaryInputButton = { nullptr };
    CUIText*                m_pPrimaryActionText = { nullptr };

    CUIButton*              m_pCloseInputButton = { nullptr };
    CUIText*                m_pCloseActionText = { nullptr };

private:
    _uint                  m_iTargetSpawnID = { UINT_MAX };
    NPC_TYPE               m_eTargetNPCType = { NPC_TYPE::END };

    // 서비스 옵션은 화면 아래부터 Slot 0, 1, 2 순서다.
    _int                   m_iFocusedServiceSlot = { -1 };

    _bool                  m_bHUDLayerSuppressed = { false };

    _bool m_bCloseAnimationPending = { false }; // Close 애니메이션 완료를 기다리는 상태입니다.

public:
    static CUINPCDialogueController* Create();
    virtual void Free() override;
};

NS_END