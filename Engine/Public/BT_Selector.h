#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Selector final : public CBT_Composite
{
private :
	CBT_Selector(const _wstring& strName);
	virtual ~CBT_Selector() = default;

public :
	virtual Status	Tick() override;

public :
	static CBT_Selector* Create(const _wstring& strName);
	virtual void Free() override;
};

NS_END