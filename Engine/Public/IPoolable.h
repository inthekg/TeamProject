#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class IObjectPool;

class ENGINE_DLL IPoolable
{
protected:
	IPoolable() = default;
	virtual ~IPoolable() = default;
public:
	void			Set_OwnerPool(IObjectPool* pPool) { m_pOwnerPool = pPool; }
	size_t			Get_PoolIndex() const { return m_iPoolIndex; }
	void			Set_PoolIndex(size_t iPoolIndex) { m_iPoolIndex = iPoolIndex; }

public:
	virtual void	Activate(void* pArg = nullptr) PURE;
	virtual void	InActivate() PURE;	

protected:
	_bool			Has_OwnerPool() const { return m_pOwnerPool != nullptr; }
	_uint 			Return_ToPool();

protected:
	IObjectPool*	m_pOwnerPool = { nullptr };
	size_t			m_iPoolIndex = {};

};

NS_END