#pragma once
#include "UIHUDViewModelBase.h"

NS_BEGIN(GameCore)

class CBossHUDViewModel final : public CUIHUDViewModelBase
{
private:
    CBossHUDViewModel() = default;
    virtual ~CBossHUDViewModel() = default;

public:
    static CBossHUDViewModel* Create();
    virtual void Free() override;
};

NS_END
