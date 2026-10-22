#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_DebugCapsule : public CVIBuffer
{
public:
	typedef struct tagDebugCapsuleDesc
	{
		_float	fRadius;
		_float	fHalfHeight;
		_float4	vColor;
		_uint	iSegment;
	} DEBUGCAPSULE_DESC;

private:
	CVIBuffer_DebugCapsule(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CVIBuffer_DebugCapsule() = default;

public:
	virtual HRESULT		Initialize_Prototype(const DEBUGCAPSULE_DESC& Desc);
	virtual HRESULT		Bind_Resources() override;
	virtual HRESULT		Render() override;

private:
	DEBUGCAPSULE_DESC	m_Desc = {};

public:
	static CVIBuffer_DebugCapsule* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const DEBUGCAPSULE_DESC& Desc);
	virtual CComponent* Clone(void* pArg) override { return nullptr; }
	virtual void		Free() override;
};

NS_END