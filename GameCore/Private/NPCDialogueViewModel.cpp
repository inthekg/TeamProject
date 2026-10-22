#include "NPCDialogueViewModel.h"

void CNPCDialogueViewModel::Begin_Dialogue(const _wstring& strSpeakerName, const _wstring& strDialogueText)
{
    m_bVisible = true;
    m_eViewMode = NPC_DIALOGUE_VIEW_MODE::DIALOGUE;

    m_strSpeakerName = strSpeakerName;
    m_strDialogueText = strDialogueText;
    m_strPrimaryActionText = L"다음";

    Clear_ServiceOptions();

    Make_Dirty();
}

void CNPCDialogueViewModel::Set_DialogueLine(const _wstring& strSpeakerName, const _wstring& strDialogueText)
{
    m_eViewMode = NPC_DIALOGUE_VIEW_MODE::DIALOGUE;

    m_strSpeakerName = strSpeakerName;
    m_strDialogueText = strDialogueText;
    m_strPrimaryActionText = L"다음";

    Clear_ServiceOptions();

    Make_Dirty();
}

void CNPCDialogueViewModel::Show_ServiceOptions(const vector<_wstring>& ServiceLabels)
{
    Clear_ServiceOptions();

    const size_t iCount = min(ServiceLabels.size(), static_cast<size_t>(NPC_DIALOGUE_MAX_SERVICE_COUNT));

    m_iServiceCount = static_cast<_uint>(iCount);

    for (_uint i = 0; i < m_iServiceCount; ++i)
        m_strServiceLabels[i] = ServiceLabels[i];

    m_eViewMode = NPC_DIALOGUE_VIEW_MODE::SERVICE_SELECTION;
    m_strPrimaryActionText = L"선택";

    Make_Dirty();
}

void CNPCDialogueViewModel::Close_Dialogue()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    m_eViewMode = NPC_DIALOGUE_VIEW_MODE::NONE;

    Clear_ServiceOptions();

    Make_Dirty();
}

void CNPCDialogueViewModel::Clear_ServiceOptions()
{
    m_iServiceCount = 0;

    for (_uint i = 0; i < NPC_DIALOGUE_MAX_SERVICE_COUNT; ++i)
        m_strServiceLabels[i].clear();
}

CNPCDialogueViewModel* CNPCDialogueViewModel::Create()
{
    return new CNPCDialogueViewModel;
}

void CNPCDialogueViewModel::Free()
{
    __super::Free();
}