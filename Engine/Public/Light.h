#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CLight final : public CBase
{
private:
	CLight(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLight() = default;

public:
	const LIGHT_DESC* Get_LightDesc() const 
	{
		return &m_LightDesc;
	}

	_bool Is_Enable() const { return m_bEnable; }
	void Set_Enable(_bool bEnable) { m_bEnable = bEnable; }

	void Set_LightDesc(const LIGHT_DESC& LightDesc) { m_LightDesc = LightDesc; }


public:
	HRESULT							Initialize(const LIGHT_DESC& LightDesc);

private:
	ID3D11Device*					m_pDevice = { };
	ID3D11DeviceContext*			m_pContext = { };

	LIGHT_DESC						m_LightDesc{};
	_bool							m_bEnable = true;

public:
	static CLight*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LIGHT_DESC& LightDesc);
	virtual void					Free() override;
};

NS_END
