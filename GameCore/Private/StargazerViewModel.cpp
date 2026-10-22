#include "StargazerViewModel.h"

void CStargazerViewModel::Open_Stargazer(const STARGAZER_UI_OPEN_DESC& OpenDesc)
{
    if (m_bVisible)
        return;

    m_OpenDesc = OpenDesc;
    m_bVisible = true;
    Make_Dirty(); // 컨트롤러가 변경된 상태를 실제 UI에 반영하도록 표시한다.
}

void CStargazerViewModel::Close_Stargazer()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    m_OpenDesc = {};
    Make_Dirty(); // 컨트롤러가 UI를 닫도록 상태 변경을 알린다.
}

CStargazerViewModel* CStargazerViewModel::Create()
{
    return new CStargazerViewModel();
}

void CStargazerViewModel::Free()
{
    __super::Free();
}