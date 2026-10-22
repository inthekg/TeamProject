#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

class CUIHUDViewModelBase abstract : public CUIViewModelBase
{
protected:
    CUIHUDViewModelBase() = default;
    virtual ~CUIHUDViewModelBase() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    void                Set_Visible(_bool bVisible);
    void                Set_Position(const _float2& vPosition);

    void                Set_HPRatio(_float fRatio);

    void                Set_GroggyVisible(_bool bVisible);
    void                Set_GroggyPosition(const _float2& vPosition);

    void                Set_FatalVisible(_bool bVisible);
    void                Set_FatalPosition(const _float2& vPosition);

public:
    _bool               Is_Visible() const { return m_bVisible; }
    const _float2&      Get_Position() const { return m_vPosition; }

    _float              Get_HPRatio() const { return m_fHPRatio; }

    _bool               Is_GroggyVisible() const { return m_bGroggyVisible; }
    const _float2&      Get_GroggyPosition() const { return m_vGroggyPosition; }

    _bool               Is_FatalVisible() const { return m_bFatalVisible; }
    const _float2&      Get_FatalPosition() const { return m_vFatalPosition; }

private:
    _bool               m_bVisible = { false };
    _float2             m_vPosition = {};

    _float              m_fHPRatio = { 1.f };

    _bool               m_bGroggyVisible = { false };
    _float2             m_vGroggyPosition = {};

    _bool               m_bFatalVisible = { false };
    _float2             m_vFatalPosition = {};

public:
    virtual void Free() override;
};

NS_END