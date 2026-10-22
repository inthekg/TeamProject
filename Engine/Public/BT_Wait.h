#pragma once
#include "BT_Node.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Wait final : public CBT_Node
{
private :
	CBT_Wait(const _wstring& strName);
	virtual ~CBT_Wait() = default;

protected :
	virtual void OnEnter() override;
	virtual Status OnUpdate() override;

private :
	_float m_fDuration = { 0.f };
	_float m_fElapsed = { 0.f };

public :
	static CBT_Wait* Create(const _wstring strName, _float fSeconds);
	virtual void Free() override;
};

NS_END