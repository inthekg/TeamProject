#pragma once

#include "Resource.h"

NS_BEGIN(Engine)
class ENGINE_DLL CVIBuffer abstract : public CResource
{
protected:
	CVIBuffer(ID3D11Device * pDevice, ID3D11DeviceContext * pContext);
	CVIBuffer(const CVIBuffer& Prototype);
	virtual ~CVIBuffer() = default;

public:
	virtual HRESULT             Initialize_Prototype();
	virtual HRESULT             Initialize(void* pArg);

	virtual HRESULT             Bind_Resources();
	virtual HRESULT             Render();

protected:
	ID3D11Buffer*               m_pVB = { nullptr };
	ID3D11Buffer*               m_pIB = { nullptr };

	_uint					    m_iNumVertexBuffers = {};
	DXGI_FORMAT				    m_eIndexFormat = { };

	_uint					    m_iNumVertices = { };
	_uint					    m_iVertexStride = { };

	_uint					    m_iNumIndices = {};
	_uint					    m_iIndexStride = {};

	D3D11_PRIMITIVE_TOPOLOGY	m_ePrimitiveType = {};

public:
	virtual void Free() override;

};

NS_END