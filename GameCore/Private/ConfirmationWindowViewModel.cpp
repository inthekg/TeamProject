#include "ConfirmationWindowViewModel.h"

void CConfirmationWindowViewModel::Open_Confirmation(const CONFIRMATION_REQUEST_DESC& RequestDesc)
{
    if (RequestDesc.eActionType == CONFIRMATION_ACTION_TYPE::END)
        return;

    m_RequestDesc = RequestDesc;
    m_bVisible = true;

    Make_Dirty();
}

void CConfirmationWindowViewModel::Close_Confirmation()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    Make_Dirty();
}

CONFIRMATION_RESULT_DESC CConfirmationWindowViewModel::Make_Result(_bool bConfirmed) const
{
    CONFIRMATION_RESULT_DESC ResultDesc = {};

    ResultDesc.eActionType = m_RequestDesc.eActionType;
    ResultDesc.bConfirmed = bConfirmed;
    ResultDesc.iItemID = m_RequestDesc.iItemID;
    ResultDesc.iArmIndex = m_RequestDesc.iArmIndex;
    ResultDesc.iUpgradeStage = m_RequestDesc.iUpgradeStage;
    ResultDesc.iTargetStargazerID = m_RequestDesc.iTargetStargazerID;

    return ResultDesc;
}

CConfirmationWindowViewModel* CConfirmationWindowViewModel::Create()
{
    return new CConfirmationWindowViewModel;
}

void CConfirmationWindowViewModel::Free()
{
    __super::Free();
}