#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Inverter final : public CBT_Decorator
{
private :
	CBT_Inverter(const _wstring& strName, CBT_Node* pChild);
	virtual ~CBT_Inverter() = default;

public :
	virtual CBT_Node::Status Tick() override;	

public :
	static CBT_Inverter* Create(const _wstring strName, CBT_Node* pChild = nullptr);
	virtual void Free() override;
};

NS_END