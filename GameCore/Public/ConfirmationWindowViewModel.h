#pragma once
#include "UIViewModelBase.h"
#include "ConfirmationWindowData.h"

NS_BEGIN(GameCore)

class CConfirmationWindowViewModel final : public CUIViewModelBase
{
private:
    CConfirmationWindowViewModel() = default;
    virtual ~CConfirmationWindowViewModel() = default;

public:
    void Open_Confirmation(const CONFIRMATION_REQUEST_DESC& RequestDesc);
    void Close_Confirmation();

public:
    _bool Is_Visible() const { return m_bVisible; }
    const CONFIRMATION_REQUEST_DESC& Get_RequestDesc() const { return m_RequestDesc; }

    // 현재 요청 정보를 유지한 채 확인/취소 결과 데이터를 만든다.
    CONFIRMATION_RESULT_DESC Make_Result(_bool bConfirmed) const;

private:
    _bool m_bVisible = { false };
    CONFIRMATION_REQUEST_DESC m_RequestDesc = {};

public:
    static CConfirmationWindowViewModel* Create();
    virtual void Free() override;
};

NS_END