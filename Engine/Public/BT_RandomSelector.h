#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_RandomSelector final : public CBT_Composite
{
private :
	CBT_RandomSelector(const _wstring& strName);
	virtual ~CBT_RandomSelector() = default;

public :
	void			Set_Weight(const vector<_float>& Weights) { m_Weights = Weights; }
	void			Set_RepeatPenalty(_float fPenalty) { m_fRepeatPenalty = fPenalty; }

public :
	virtual Status	Tick() override;
	virtual void	Reset() override;
	virtual void	Abort() override;

private :
	void			Build_Order();

private :
	vector<_uint>	m_Orders;
	vector<_float>	m_Weights;
	_bool			m_bShuffled = { false };
	_int			m_iLastPicked = { -1 };
	_float			m_fRepeatPenalty = { 0.25f };

public :
	static CBT_RandomSelector* Create(const _wstring& strName);
	virtual void Free() override;
};

NS_END