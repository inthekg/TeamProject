#pragma once
#include "UIHUDViewModelBase.h"

NS_BEGIN(GameCore)

class CCrosshairHUDViewModel final : public CUIHUDViewModelBase
{
private:
    CCrosshairHUDViewModel() = default;
    virtual ~CCrosshairHUDViewModel() = default;

public:
    static CCrosshairHUDViewModel* Create();
    virtual void Free() override;
};

NS_END