#pragma once
#include "ILayer.h"
#include "LocoState.h"

NS_BEGIN(GameCore)
class CLocoLayer final : public ILayer
{
private:
	CLocoLayer();
	virtual ~CLocoLayer() = default;

public:
	PlayerLocoID Get_CurLocoState() const
	{
		if (m_pCurLocoState) return m_pCurLocoState->Get_ActionID();
		else return PlayerLocoID::End;
	}

public:
	virtual HRESULT		Initialize(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDelta) override;

private:
	CLocoState*			Find_LocoState(PlayerLocoID eLocoID);
	void				Change_LocoState(PlayerLocoID eNextLocoID, StateContext& Context);

private:
	unordered_map<PlayerLocoID, CLocoState*> m_LocoStateMap;
	CLocoState*			m_pCurLocoState = {};

public:
	static CLocoLayer* Create(StateContext& Context);
	virtual void Free() override;
};

NS_END
