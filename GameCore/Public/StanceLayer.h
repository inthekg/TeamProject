#pragma once
#include "ILayer.h"
#include "StanceState.h"

NS_BEGIN(GameCore)
class CStanceLayer : public ILayer
{
private:
	CStanceLayer();
	virtual ~CStanceLayer() = default;

public:
	PlayerStanceID Get_CurStanceState() const
	{
		if (m_pCurStanceState) return m_pCurStanceState->Get_Id();
		else return PlayerStanceID::End;
	}

public:
	virtual HRESULT		Initialize(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDeltas) override;

private:
	CStanceState*		Find_StanceState(PlayerStanceID eStanceID);
	void				Change_StanceState(PlayerStanceID eNextStanceID, StateContext& Context);

private:
	unordered_map<PlayerStanceID, CStanceState*> m_StanceStateMap;
	CStanceState* m_pCurStanceState = {};
public:
	static CStanceLayer* Create(StateContext& Context);
	virtual void Free() override;
};

NS_END
