#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Service final : public CBT_Decorator
{
public :
	using ServiceFunc = function<void(class CBT_BlackBoard*)>;
	
private :
	CBT_Service(const _wstring& strName, CBT_Node* pChild, ServiceFunc func, _float fInterval);
	virtual ~CBT_Service() = default;

public :
	virtual Status	Tick() override;
	virtual void	Reset() override;
	virtual void	Abort() override;

private :
	ServiceFunc	m_func;
	_float		m_fInterval = { 0.f };
	_float		m_fAccum	= { 0.f };

public :
	static CBT_Service* Create(const _wstring& strName, CBT_Node* pChild, ServiceFunc func, _float fInterval);
	virtual void Free() override;
};

NS_END