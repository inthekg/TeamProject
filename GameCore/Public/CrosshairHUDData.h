#pragma once
#include "Engine_Struct.h"

NS_BEGIN(GameCore)

typedef struct tagCrosshairVisibilityDesc final : public Engine::Event_Desc
{
    _bool bVisible = { false };
} CROSSHAIR_VISIBILITY_DESC;

NS_END