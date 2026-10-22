#pragma once
#include "Component.h"
#include "Core_Defines.h"
#include "State.h"

NS_BEGIN(GameCore)
class CPlayer;
class CActionLayer;
class CLocoLayer;
class CStanceLayer;

class GAMECORE_DLL CPlayerStateMachine : public CComponent
{
private:
	CPlayerStateMachine(CPlayer* pOwner);
	virtual ~CPlayerStateMachine() = default;

	CPlayerStateMachine(const CPlayerStateMachine& Prototype) = delete;
	CPlayerStateMachine& operator=(const CPlayerStateMachine& other) = delete;

public:
	HRESULT			Initialize();
	void			Update(_float fTimeDelta);
	//void			OnNotify();

private:
	StateContext	m_Context;
	
	CActionLayer*	m_pActionLayer = {};
	CLocoLayer*		m_pLocoLayer = {};
	CStanceLayer*	m_pStanceLayer = {};

	CPlayer*		m_pOwner = {};
public:
	static CPlayerStateMachine* Create(CPlayer* pOwner);
	virtual void Free() override;
};

NS_END