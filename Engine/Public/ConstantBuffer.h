#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class CConstantBuffer : public CBase
{
protected:
	CConstantBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CConstantBuffer() = default;

	CConstantBuffer(const CConstantBuffer& rhs)				= delete;
	CConstantBuffer& operator= (const CConstantBuffer& rhs) = delete;

public:
	virtual HRESULT			Initialize(void* pData, _uint iDataSize);

	HRESULT					Update_Data(void* pData, _uint iDataSize);
	//외부에서 값을 채울 때 (material Binding 경우) 
	HRESULT					Update_Data_CallBack(function<void(void*)> writeCallBack);
public:
	void					BindVS(_uint iSlot);
	void					BindPS(_uint iSlot);
	void					BindGS(_uint iSlot);

protected:
	ID3D11Device*			m_pDevice			= { nullptr };
	ID3D11DeviceContext*	m_pContext			= { nullptr };

	ID3D11Buffer*			m_pConstantBuffer	= { nullptr };
	_uint					m_iDataSize			= {};

public:
	static CConstantBuffer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pInitData, _uint iDataSize);
	virtual void			Free() override;
};

NS_END
