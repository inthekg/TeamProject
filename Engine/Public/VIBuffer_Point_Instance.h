#pragma once

#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Point_Instance final : public CVIBuffer_Instance
{
public:
	typedef struct tagPointInstanceDesc final : public CVIBuffer_Instance::INSTANCE_DESC
	{
		_float2			vSpeed;
		_float2			vLifeTime;
		_bool			isLoop;

		_float3			vPivot;

	}POINT_INSTANCE_DESC;
private:
	CVIBuffer_Point_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Point_Instance(const CVIBuffer_Point_Instance& Prototype);
	virtual ~CVIBuffer_Point_Instance() = default;

public:
	virtual HRESULT Initialize_Prototype(void* pInitialDesc);
	virtual HRESULT Initialize(void* pArg);
	virtual HRESULT Bind_Resources() override;
	virtual HRESULT Render() override;

public:
	void Drop(_float fTimeDelta);
	void Spread(_float fTimeDelta);

private:
	_float*			m_pSpeeds = { nullptr };
	_bool			m_isLoop = { false };

	_uint			m_iVertexCountPerInstance = {};
	_float3			m_vPivot = { };


public:
	static CVIBuffer_Point_Instance* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pInitialDesc);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;
	
};

NS_END