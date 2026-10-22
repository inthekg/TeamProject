#include "UINPCDialogueController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIButton.h"
#include "UIText.h"
#include "NPC.h"
#include "NPCDialogueViewModel.h"

HRESULT CUINPCDialogueController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CNPCDialogueViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    const _wstring strDialoguePath = m_pGameInstance->Get_ResourcePath() + L"\\Data\\Dialogue\\NPCDialogue.json";

    if (FAILED(m_DialogueRepository.Initialize(strDialoguePath)))
        return E_FAIL;

    // NPC가 E 입력을 받아 발송하는 대화 시작 이벤트를 구독한다.
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_TALK_START, this);

    return S_OK;
}

void CUINPCDialogueController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI || !m_pViewModel->Is_Visible())
        return;

    // 현재 최상단 Input Owner가 아닌 대화창의 Command는 처리하지 않는다.
    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"PrimaryInputHint.HintButtonSpace")
            Handle_PrimaryAction();
        else if (Event.strRuntimeElementTag == L"CloseInputHint.HintButton")
            End_Dialogue();

        return;
    }

    if (Event.strCommandKey == "SelectDialogueService")
    {
        const _int iSlotIndex = Resolve_ServiceSlot(Event.strRuntimeElementTag);

        if (iSlotIndex >= 0)
            Select_ServiceSlot(static_cast<_uint>(iSlotIndex));
    }
}

void CUINPCDialogueController::On_UIFocusChanged(const UI_FOCUS_EVENT& Event)
{
    if (Event.hUI != m_hUI)
        return;

    if (Event.strFocusGroup != "DialogueService")
        return;

    const _int iSlotIndex = Resolve_ServiceSlot(Event.strRuntimeElementTag);

    if (iSlotIndex < 0)
        return;

    if (Event.bFocused)
    {
        m_iFocusedServiceSlot = iSlotIndex;
    }
    else if (m_iFocusedServiceSlot == iSlotIndex)
    {
        m_iFocusedServiceSlot = -1;
    }
}

void CUINPCDialogueController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_MAP_OBJECT::NPC_TALK_START)
        return;

    if (pData == nullptr || m_pViewModel == nullptr)
        return;

    const CNPC::NPC_DIALOGUE_EVENT_DESC* pDesc = static_cast<const CNPC::NPC_DIALOGUE_EVENT_DESC*>(pData);
    
    // 유효한 NPC와 DialogueID가 포함된 시작 이벤트만 처리한다.
    if (pDesc->iTargetSpawnID == UINT_MAX || pDesc->eNPCType == NPC_TYPE::END || pDesc->strDialogueID.empty())
        return;


    const NPC_DIALOGUE_SCRIPT_DESC* pScript = m_DialogueRepository.Find_Script(pDesc->strDialogueID);

    // 현재 다른 NPC와 대화 중이면 중복 시작 이벤트를 무시한다. 
    if (m_pViewModel->Is_Visible() || pScript == nullptr)
    {
        CNPC::NPC_DIALOGUE_EVENT_DESC EndDesc{};
        EndDesc.iTargetSpawnID = pDesc->iTargetSpawnID;
        EndDesc.eNPCType = pDesc->eNPCType;

        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, &EndDesc);
        return;
    }

    m_iTargetSpawnID = pDesc->iTargetSpawnID;
    m_eTargetNPCType = pDesc->eNPCType;

    m_pCurrentScript = pScript;
    m_iCurrentLineIndex = 0;

    m_iFocusedServiceSlot = -1;

    // Repository의 첫 번째 실제 대사 줄을 화면에 전달한다.
    Apply_CurrentDialogueLine();
}

HRESULT CUINPCDialogueController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pSpeakerNameText = Find_ElementAs<CUIText>(L"SpeakerNameText");
    m_pDialogueBodyText = Find_ElementAs<CUIText>(L"DialogueBodyText");
    m_pServiceMenuGroup = Find_Element(L"ServiceMenuGroup");

    for (_uint i = 0; i < NPC_DIALOGUE_MAX_SERVICE_COUNT; ++i)
    {
        const _wstring strInstanceTag = L"ServiceOption" + to_wstring(i);

        m_pServiceOptions[i] = Find_Element(strInstanceTag);
        m_pServiceButtons[i] = Find_ElementAs<CUIButton>(strInstanceTag + L".ServiceOptionButton");
        m_pServiceLabels[i] = Find_ElementAs<CUIText>(strInstanceTag + L".ServiceOptionLabel");

        if (m_pServiceOptions[i] == nullptr ||
            m_pServiceButtons[i] == nullptr ||
            m_pServiceLabels[i] == nullptr)
        {
            return E_FAIL;
        }
    }

    m_pPrimaryInputButton = Find_ElementAs<CUIButton>(L"PrimaryInputHint.HintButtonSpace");
    m_pPrimaryActionText = Find_ElementAs<CUIText>(L"PrimaryInputHint.ActionText");

    m_pCloseInputButton = Find_ElementAs<CUIButton>(L"CloseInputHint.HintButton");
    m_pCloseActionText = Find_ElementAs<CUIText>(L"CloseInputHint.ActionText");

    if (m_pSpeakerNameText == nullptr ||
        m_pDialogueBodyText == nullptr ||
        m_pServiceMenuGroup == nullptr ||
        m_pPrimaryInputButton == nullptr ||
        m_pPrimaryActionText == nullptr ||
        m_pCloseInputButton == nullptr ||
        m_pCloseActionText == nullptr)
    {
        return E_FAIL;
    }

    return S_OK;
}

void CUINPCDialogueController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr)
        return;

    // Close 애니메이션 재생 중에는 대화 입력을 받지 않고 완료만 기다립니다.
    if (m_bCloseAnimationPending)
    {
        if (m_pUIObject != nullptr && m_pUIObject->Is_AnimationFinished())
            Finalize_Close();

        return;
    }

    if (!m_pViewModel->Is_Visible())
        return;

    // 대화창이 최상단 Input Owner일 때만 입력을 처리합니다.
    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        End_Dialogue();
        return;
    }

    if (m_pGameInstance->Key_Down(DIK_SPACE))
        Handle_PrimaryAction();
}

void CUINPCDialogueController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (bShouldOpen)
    {
        if (!m_bHUDLayerSuppressed)
        {
            // 대화 중에는 일반 게임 HUD를 숨긴다.
            m_pGameInstance->Acquire_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = true;
        }

        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                // ViewModel의 Visible/Dirty 상태와 NPC의 TALK_START 상태까지 함께 취소한다.
                End_Dialogue();
                return;
            }

            // 실제로 열린 NPC 대화창을 Screen Input Owner로 등록한다.
            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::SCREEN)))
            {
                // Input Owner 등록 실패 시 열린 UI와 HUD 억제를 모두 원상복구한다.
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                End_Dialogue();
                return;
            }

            // 실제 UI와 Input Owner 등록이 끝난 뒤 Open 알파 애니메이션을 시작합니다.
            if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Open"))
                m_pUIObject->Play_Animation("Open", true);
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            // 실제 UI를 즉시 끄지 않고 Close 알파 애니메이션부터 시작합니다.
            Begin_CloseAnimation();
            return;
        }

        if (m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Release_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = false;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    m_pSpeakerNameText->Set_Text(m_pViewModel->Get_SpeakerName());
    m_pDialogueBodyText->Set_Text(m_pViewModel->Get_DialogueText());
    m_pPrimaryActionText->Set_Text(m_pViewModel->Get_PrimaryActionText());
    m_pCloseActionText->Set_Text(L"닫기");

    const _bool bShowServiceMenu = m_pViewModel->Get_ViewMode() == NPC_DIALOGUE_VIEW_MODE::SERVICE_SELECTION;

    m_pServiceMenuGroup->Set_Visible(bShowServiceMenu);

    const _uint iServiceCount = m_pViewModel->Get_ServiceCount();

    for (_uint iSlotIndex = 0; iSlotIndex < NPC_DIALOGUE_MAX_SERVICE_COUNT; ++iSlotIndex)
    {
        const _bool bVisible = bShowServiceMenu && iSlotIndex < iServiceCount;

        m_pServiceOptions[iSlotIndex]->Set_Visible(bVisible);
        m_pServiceButtons[iSlotIndex]->Set_Interactable(bVisible);
        m_pServiceButtons[iSlotIndex]->Set_Selected(false);

        if (!bVisible)
            continue;

        /*
         * ServiceOption0이 맨 아래이므로 데이터는 역순으로 연결한다.
         *
         * 데이터 0 → 가장 위 슬롯
         */
        const _uint iDataIndex = iServiceCount - 1 - iSlotIndex;

        m_pServiceLabels[iSlotIndex]->Set_Text(m_pViewModel->Get_ServiceLabel(iDataIndex));
    }

    if (bShowServiceMenu && m_iFocusedServiceSlot >= 0 && static_cast<_uint>(m_iFocusedServiceSlot) < iServiceCount)
    {
        m_pServiceButtons[m_iFocusedServiceSlot]->Set_Selected(true);
    }

    m_pViewModel->Clear_Dirty();
}

void CUINPCDialogueController::Handle_PrimaryAction()
{
    if (m_pViewModel == nullptr)
        return;

    const NPC_DIALOGUE_VIEW_MODE eMode = m_pViewModel->Get_ViewMode();

    if (eMode == NPC_DIALOGUE_VIEW_MODE::DIALOGUE)
    {
        Advance_Dialogue();
        return;
    }

    if (eMode == NPC_DIALOGUE_VIEW_MODE::SERVICE_SELECTION)
    {
        const _uint iServiceCount = m_pViewModel->Get_ServiceCount();

        if (iServiceCount == 0)
            return;

        _int iSlotIndex = m_iFocusedServiceSlot;

        // 아무 항목에도 호버하지 않았다면 가장 위 항목을 선택한다.
        if (iSlotIndex < 0)
            iSlotIndex = static_cast<_int>(iServiceCount) - 1;

        Select_ServiceSlot(static_cast<_uint>(iSlotIndex));
    }
}

void CUINPCDialogueController::Select_ServiceSlot(_uint iSlotIndex)
{
    if (m_pViewModel == nullptr || m_pCurrentScript == nullptr)
        return;

    const _uint iServiceCount = m_pViewModel->Get_ServiceCount();

    if (iServiceCount == 0 || iSlotIndex >= iServiceCount)
        return;

    // UI 슬롯은 아래부터 0번이고 데이터는 위부터 0번이므로 인덱스를 뒤집는다.
    const _uint iDataIndex = iServiceCount - 1 - iSlotIndex;
    if (iDataIndex >= m_pCurrentScript->ServiceOptions.size())
        return;

    const NPC_DIALOGUE_SERVICE_OPTION_DESC& Option = m_pCurrentScript->ServiceOptions[iDataIndex];
    const NPC_DIALOGUE_SERVICE_TYPE eServiceType = Option.eServiceType;

    // 선택된 서비스 종류와 NPC 정보를 실제 서비스 UI 쪽으로 전달
    Request_DialogueService(eServiceType);

    // 대화가 이어지는 종류는 선택 즉시 창을 닫아 NPC_TALK_END 를 발생시킨다.
    if (NPC_DIALOGUE_SERVICE_TYPE::DONKEY_GATE_ANSWER == eServiceType)
        End_Dialogue();
}

void CUINPCDialogueController::Request_DialogueService(NPC_DIALOGUE_SERVICE_TYPE eServiceType)
{
    if (m_pGameInstance == nullptr)
        return;

    if (m_iTargetSpawnID == UINT_MAX || m_eTargetNPCType == NPC_TYPE::END)
        return;

    if (eServiceType == NPC_DIALOGUE_SERVICE_TYPE::NONE || eServiceType == NPC_DIALOGUE_SERVICE_TYPE::END)
        return;

    NPC_DIALOGUE_SERVICE_REQUEST_DESC EventDesc{};
    EventDesc.iTargetSpawnID = m_iTargetSpawnID;
    EventDesc.eNPCType = m_eTargetNPCType;
    EventDesc.eServiceType = eServiceType;

    // EventManager의 동기 Broadcast를 이용해 해당 서비스 컨트롤러에 실행을 요청한다.
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST, &EventDesc);
}

void CUINPCDialogueController::End_Dialogue()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
        return;

    CNPC::NPC_DIALOGUE_EVENT_DESC EventDesc{};

    EventDesc.iTargetSpawnID = m_iTargetSpawnID;
    EventDesc.eNPCType = m_eTargetNPCType;

    m_pViewModel->Close_Dialogue();

    m_iTargetSpawnID = UINT_MAX;
    m_eTargetNPCType = NPC_TYPE::END;

    m_pCurrentScript = nullptr;
    m_iCurrentLineIndex = 0;

    m_iFocusedServiceSlot = -1;

    // 해당 NPC가 Talking 상태를 종료하도록 알린다.
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, &EventDesc);
}

// 현재 LineIndex의 대사를 ViewModel에 전달한다.
void CUINPCDialogueController::Apply_CurrentDialogueLine()
{
    if (m_pViewModel == nullptr || m_pCurrentScript == nullptr)
        return;

    if (m_iCurrentLineIndex >= m_pCurrentScript->DialogueLines.size())
    {
        End_Dialogue();
        return;
    }

    const NPC_DIALOGUE_LINE_DESC& Line = m_pCurrentScript->DialogueLines[m_iCurrentLineIndex];

    // 첫 줄은 대화창을 시작하고, 이후 줄은 열린 ViewModel 내용만 변경한다.
    if (!m_pViewModel->Is_Visible())
        m_pViewModel->Begin_Dialogue(Line.strSpeakerName, Line.strDialogueText);
    else
        m_pViewModel->Set_DialogueLine(Line.strSpeakerName, Line.strDialogueText);

    if (Line.iEventIndex >= 0)
    {
        CNPC::NPC_DIALOGUE_EVENT_DESC EventDesc{};
        EventDesc.iTargetSpawnID = m_iTargetSpawnID;
        EventDesc.eNPCType = m_eTargetNPCType;
        EventDesc.iEventIndex = Line.iEventIndex;

        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_LINE_EVENT, &EventDesc);
    }
}

// 다음 대사, 서비스 선택 또는 대화 종료로 진행한다.
void CUINPCDialogueController::Advance_Dialogue()
{
    if (m_pCurrentScript == nullptr)
    {
        End_Dialogue();
        return;
    }

    const size_t iLineCount = m_pCurrentScript->DialogueLines.size();

    if (m_iCurrentLineIndex + 1 < iLineCount)
    {
        // 아직 다음 대사가 남아 있으므로 인덱스를 증가시키고 화면을 갱신한다.
        ++m_iCurrentLineIndex;
        Apply_CurrentDialogueLine();
        return;
    }

    // 마지막 대사 뒤에 서비스가 있으면 선택지를 표시하고, 없으면 대화를 종료한다.
    if (!m_pCurrentScript->ServiceOptions.empty())
    {
        Enter_ServiceSelection();
        return;
    }

    End_Dialogue();
}

// 현재 스크립트의 서비스 선택지를 ViewModel에 전달한다.
void CUINPCDialogueController::Enter_ServiceSelection()
{
    if (m_pViewModel == nullptr || m_pCurrentScript == nullptr || m_pCurrentScript->ServiceOptions.empty())
    {
        End_Dialogue();
        return;
    }

    vector<_wstring> ServiceLabels;
    ServiceLabels.reserve(m_pCurrentScript->ServiceOptions.size());

    for (const NPC_DIALOGUE_SERVICE_OPTION_DESC& Option : m_pCurrentScript->ServiceOptions)
        ServiceLabels.push_back(Option.strLabel);

    m_pViewModel->Show_ServiceOptions(ServiceLabels);

    // UI 배치상 가장 위에 표시되는 슬롯을 기본 선택 대상으로 사용한다.
    m_iFocusedServiceSlot = static_cast<_int>(ServiceLabels.size()) - 1;
}

_int CUINPCDialogueController::Resolve_ServiceSlot(const _wstring& strRuntimeElementTag) const
{
    if (strRuntimeElementTag == L"ServiceOption0.ServiceOptionButton")
        return 0;

    if (strRuntimeElementTag == L"ServiceOption1.ServiceOptionButton")
        return 1;

    if (strRuntimeElementTag == L"ServiceOption2.ServiceOptionButton")
        return 2;

    return -1;
}

void CUINPCDialogueController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    // Close 클립이 있으면 UI를 유지한 채 알파 애니메이션을 재생합니다.
    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Close"))
    {
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    // Close 클립이 없으면 즉시 닫습니다.
    Finalize_Close();
}

void CUINPCDialogueController::Finalize_Close()
{
    if (m_pGameInstance->Is_OpenUIObject(m_hUI))
    {
        // Close 애니메이션이 끝난 후 Input Owner와 실제 UI를 제거합니다.
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

    m_bCloseAnimationPending = false;

    if (m_pViewModel != nullptr)
        m_pViewModel->Clear_Dirty();
}

CUINPCDialogueController* CUINPCDialogueController::Create()
{
    return new CUINPCDialogueController;
}

void CUINPCDialogueController::Free()
{
    if (m_pGameInstance != nullptr)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    Safe_Release(m_pViewModel);

    m_pCurrentScript = nullptr;

    m_pSpeakerNameText = nullptr;
    m_pDialogueBodyText = nullptr;

    m_pServiceMenuGroup = nullptr;

    for (_uint i = 0; i < NPC_DIALOGUE_MAX_SERVICE_COUNT; ++i)
    {
        m_pServiceOptions[i] = nullptr;
        m_pServiceButtons[i] = nullptr;
        m_pServiceLabels[i] = nullptr;
    }

    m_pPrimaryInputButton = nullptr;
    m_pPrimaryActionText = nullptr;

    m_pCloseInputButton = nullptr;
    m_pCloseActionText = nullptr;

    __super::Free();
}
