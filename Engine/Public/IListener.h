#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL IListener abstract
{
public:
    virtual ~IListener() {}
    virtual void OnEvent(EVENT_TYPE etype, Event_Desc* pData) {};
    virtual void OnEvent(EVENT_TYPE_MAP_OBJECT etype, Event_Desc* pData) {};
    virtual void OnEvent(EVENT_TYPE_UI etype, Event_Desc* pData) {};
};

NS_END