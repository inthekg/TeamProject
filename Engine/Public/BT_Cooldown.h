#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Cooldown final : public CBT_Decorator
{
private :
	CBT_Cooldown(const _wstring& strName, CBT_Node* pChild, _float fSeconds);
	virtual ~CBT_Cooldown() = default;

public :
	virtual Status Tick() override;

private :
	_float m_fCooldown = { 0.f };
	_float m_fRemain = { 0.f };

public : 
	static CBT_Cooldown* Create(const _wstring& strName, CBT_Node* pChild = nullptr, _float fSeconds = 0.f);
	virtual void Free() override;
};

NS_END