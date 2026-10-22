#pragma once
#include "ILayer.h"
NS_BEGIN(GameCore)

class CActionFSM;
class CActionLayer final : public ILayer
{
private:
	CActionLayer();
	virtual ~CActionLayer() = default;
public:
	virtual HRESULT		Initialize(StateContext& Context) override;
	virtual void		Update(StateContext& Context, _float fTimeDelta) override;

private:
	CActionFSM*			m_pActionFSM = {};

public:
	static CActionLayer* Create(StateContext& Context);
	virtual void Free() override;
};

NS_END
