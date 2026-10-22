#pragma once
#include "UIHUDViewModelBase.h"

NS_BEGIN(GameCore)

class CMonsterHUDViewModel final : public CUIHUDViewModelBase
{
private:
    CMonsterHUDViewModel() = default;
    virtual ~CMonsterHUDViewModel() = default;

public:
    void Set_HealthVisible(_bool bVisible);
    _bool Is_HealthVisible() const { return m_bHealthVisible; }

private:
    _bool m_bHealthVisible = { false }; // 체력바, 프레임, 데미지, 그로기 UI 표시 여부

public:
    static CMonsterHUDViewModel* Create();
    virtual void Free() override;
};

NS_END