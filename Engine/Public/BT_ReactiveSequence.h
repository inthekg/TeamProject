#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_ReactiveSequence final : public CBT_Composite
{
private :
	CBT_ReactiveSequence(const _wstring& strName);
	virtual ~CBT_ReactiveSequence() = default;

public :
	virtual Status Tick() override;

public :
	static CBT_ReactiveSequence* Create(const _wstring& strName);
	virtual void Free() override;
};

NS_END