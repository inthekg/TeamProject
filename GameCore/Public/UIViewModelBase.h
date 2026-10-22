#pragma once
#include "Core_Defines.h"
#include "Base.h"

NS_BEGIN(GameCore)

class CUIViewModelBase abstract : public CBase
{
protected:
    CUIViewModelBase() = default;
    virtual ~CUIViewModelBase() = default;

public:
    virtual HRESULT             Initialize(void* pArg);

public:
    _bool                       Is_Dirty() const;

public:
    virtual void                Clear_Dirty();

public:
    void                        Update_Damage(_float fCurrentHP, _float fTimeDelta);

    _bool                       Is_DamageVisible() const { return m_bDamageVisible; }
    _float                      Get_AccumulatedDamage() const { return m_fAccumulatedDamage; }

    void                        Set_FatalAvailable(_bool bAvailable); // 현재 플레이어 위치에서 페이탈 가능한지 설정
    _bool                       Is_FatalAvailable() const { return m_bFatalAvailable; }

protected:
    void                        Make_Dirty();
    _float	                    Clamp01(_float fValue);
    _bool                       Is_SamePosition(const _float2& a, const _float2& b);

protected:
    _bool                       m_bDirty = { true };

private:
    static constexpr _float     DAMAGE_VISIBLE_TIME = 5.f;

    _float                      m_fPreviousHP = { 0.f };
    _float                      m_fAccumulatedDamage = { 0.f };
    _float                      m_fDamageRemainTime = { 0.f };

    _bool                       m_bDamageTrackingInitialized = { false };
    _bool                       m_bDamageVisible = { false };
    _bool                       m_bFatalAvailable = { false };

public:
    virtual void Free() override;
};

NS_END