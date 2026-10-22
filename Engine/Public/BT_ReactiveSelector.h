#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_ReactiveSelector final : public CBT_Composite
{
private :
	CBT_ReactiveSelector(const _wstring& strName);
	virtual ~CBT_ReactiveSelector() = default;

public :
	virtual	Status	Tick() override;
	virtual void	Reset() override;
	virtual void	Abort() override;

private :
	_int	m_iRunningIndex = { -1 };

public :
	static CBT_ReactiveSelector* Create(const _wstring& strName);
	virtual void Free() override;
};

NS_END