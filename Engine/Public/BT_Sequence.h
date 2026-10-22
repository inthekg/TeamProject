#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Sequence final : public CBT_Composite
{
private :
	CBT_Sequence(const _wstring& strName);
	virtual ~CBT_Sequence() = default;

public :
	virtual Status	Tick() override;

public :
	static CBT_Sequence* Create(const _wstring& strName);
	virtual void Free() override;
};

NS_END