#pragma once
#include "Base.h"
#include "Core_Defines.h"
#include "State.h"

NS_BEGIN(GameCore)

class ILayer abstract : public CBase
{
public:
	virtual ~ILayer() = default;
	virtual HRESULT Initialize(StateContext& Context) { return S_OK; }
	virtual void Update(StateContext& Context, _float fTimeDelta) {}
	
	//virtual void OnNotify(EVENT_TYPE eType, Event_Desc* pData) {}
};

NS_END
