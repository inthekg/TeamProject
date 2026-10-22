#pragma once
#include "UIViewModelBase.h"
#include "StargazerTravelData.h"

NS_BEGIN(GameCore)

class CStargazerViewModel final : public CUIViewModelBase
{
private:
    CStargazerViewModel() = default;
    virtual ~CStargazerViewModel() = default;

public:
    void                            Open_Stargazer(const STARGAZER_UI_OPEN_DESC& OpenDesc);
    void                            Close_Stargazer();

public:
    _bool                           Is_Visible() const { return m_bVisible; }
    const _wstring&                 Get_ServiceText() const { return m_strServiceText; }
    const STARGAZER_UI_OPEN_DESC&   Get_OpenDesc() const { return m_OpenDesc; }

private:
    _bool                           m_bVisible = { false };
    _wstring                        m_strServiceText = { L"다른 별바라기로 이동한다" };
    // 현재 상호작용한 별바라기와 활성화된 이동 목적지를 보관한다.
    STARGAZER_UI_OPEN_DESC          m_OpenDesc = {};

public:
    static CStargazerViewModel* Create();
    virtual void Free() override;
};

NS_END