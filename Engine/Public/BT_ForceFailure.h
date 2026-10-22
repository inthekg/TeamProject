#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_ForceFailure final : public CBT_Decorator
{
private :
	CBT_ForceFailure(const _wstring& strName, CBT_Node* pChild);
	virtual ~CBT_ForceFailure() = default;

public :
	virtual Status Tick() override;

public :
	static CBT_ForceFailure* Create(const _wstring& strName, CBT_Node* pChild = nullptr);
	virtual void Free() override;
};

NS_END