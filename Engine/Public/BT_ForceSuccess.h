#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_ForceSuccess final : public CBT_Decorator
{
private :
	CBT_ForceSuccess(const _wstring& strName, CBT_Node* pChild);
	virtual ~CBT_ForceSuccess() = default;

public :
	virtual Status Tick() override;

public :
	static CBT_ForceSuccess* Create(const _wstring& strName, CBT_Node* pChild = nullptr);
	virtual void Free() override;
};

NS_END