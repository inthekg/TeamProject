#pragma once
#include "UIHUDViewModelBase.h"

NS_BEGIN(GameCore)

class CLockOnViewModel final : public CUIHUDViewModelBase
{
private:
    CLockOnViewModel() = default;
    virtual ~CLockOnViewModel() = default;

public:
    static CLockOnViewModel* Create();
    virtual void Free() override;
};

NS_END