#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Instance abstract : public CVIBuffer
{
public:
	typedef struct tagInstanceDesc
	{
		_uint		iNumInstance = {};
		_float3		vCenter;
		_float3		vRange;
		_float2		vSize;

	}INSTANCE_DESC;
protected:
	CVIBuffer_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Instance(const CVIBuffer_Instance& Prototype);
	virtual ~CVIBuffer_Instance() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);

	virtual HRESULT Bind_Resources() override;
	virtual HRESULT Render() override;

protected:
	ID3D11Buffer*			m_pVBInstance = { nullptr };	
	_uint					m_iNumInstances = { };
	_uint					m_iInstanceStride = { };
	_uint					m_iIndexCountPerInstance = {};

	D3D11_BUFFER_DESC		m_InstanceBufferDesc = {};
	VTXPARTICLE_INSTANCE*	m_pInstanceVertices = { nullptr };


public:
	virtual CComponent* Clone(void* pArg) = 0;
	virtual void Free() override;
	
};

NS_END