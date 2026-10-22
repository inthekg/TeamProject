#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CComponent abstract : public CBase
{
protected:
	CComponent(ID3D11Device * pDevice, ID3D11DeviceContext * pContext);
	CComponent(const CComponent& Prototype);
	virtual ~CComponent() = default;

public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);

public:
	virtual void			Clear() {};

protected:
	ID3D11Device*			m_pDevice		= { nullptr };
	ID3D11DeviceContext*	m_pContext		= { nullptr };
	class CGameInstance*	m_pGameInstance = { nullptr };

	_bool					m_bClone		= { false };

public:
	virtual CComponent*		Clone(void* pArg) { return nullptr;};
	virtual void			Free() override;
};

NS_END