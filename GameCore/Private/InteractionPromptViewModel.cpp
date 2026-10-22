#include "InteractionPromptViewModel.h"

void CInteractionPromptViewModel::Set_Visible(_bool bVisible)
{
    if (m_bVisible == bVisible)
        return;

    m_bVisible = bVisible;

    Make_Dirty();
}

void CInteractionPromptViewModel::Set_Prompt(OBJECT_TYPE eObjectType, const _wstring& strPromptText)
{
    if (m_eObjectType == eObjectType && m_strPromptText == strPromptText)
        return;

    m_eObjectType = eObjectType;
    m_strPromptText = strPromptText;

    Make_Dirty();
}

CInteractionPromptViewModel* CInteractionPromptViewModel::Create()
{
    return new CInteractionPromptViewModel;
}

void CInteractionPromptViewModel::Free()
{
    __super::Free();
}