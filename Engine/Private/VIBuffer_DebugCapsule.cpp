#include "VIBuffer_DebugCapsule.h"

CVIBuffer_DebugCapsule::CVIBuffer_DebugCapsule(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CVIBuffer{pDevice, pContext}
{
}

HRESULT CVIBuffer_DebugCapsule::Initialize_Prototype(const DEBUGCAPSULE_DESC& Desc)
{
	m_Desc = Desc;

	const _float r = m_Desc.fRadius;
	const _float h = m_Desc.fHalfHeight;
	const _uint  SEG = (m_Desc.iSegment < 4) ? 16 : m_Desc.iSegment;
	const _float4 col = m_Desc.vColor;

	vector<VTXCOLOR> vertices;
	auto AddLine = [&](float ax, float ay, float az, float bx, float by, float bz)
		{
			VTXCOLOR v0{}, v1{};
			v0.vPosition = _float3(ax, ay, az); v0.vColor = col;
			v1.vPosition = _float3(bx, by, bz); v1.vColor = col;
			vertices.push_back(v0); vertices.push_back(v1);
		};


	for (_uint s = 0; s < SEG; ++s)
	{
		float t0 = XM_2PI * s / SEG, t1 = XM_2PI * (s + 1) / SEG;
		float c0 = cosf(t0), s0 = sinf(t0), c1 = cosf(t1), s1 = sinf(t1);
		AddLine(+h, r * c0, r * s0, +h, r * c1, r * s1);
		AddLine(-h, r * c0, r * s0, -h, r * c1, r * s1);
	}

	for (_uint k = 0; k < 4; ++k)
	{
		float t = XM_2PI * k / 4, c = cosf(t), s = sinf(t);
		AddLine(+h, r * c, r * s, -h, r * c, r * s);
	}

	for (_uint s = 0; s < SEG; ++s)
	{
		float a0 = XM_PI * s / SEG, a1 = XM_PI * (s + 1) / SEG;
		float sa0 = sinf(a0), ca0 = cosf(a0), sa1 = sinf(a1), ca1 = cosf(a1);
		AddLine(+h + r * sa0, r * ca0, 0.f, +h + r * sa1, r * ca1, 0.f);
		AddLine(+h + r * sa0, 0.f, r * ca0, +h + r * sa1, 0.f, r * ca1);
		AddLine(-h - r * sa0, r * ca0, 0.f, -h - r * sa1, r * ca1, 0.f);
		AddLine(-h - r * sa0, 0.f, r * ca0, -h - r * sa1, 0.f, r * ca1);
	}

	m_iNumVertexBuffers = 1;
	m_iNumVertices = (_uint)vertices.size();
	m_iVertexStride = sizeof(VTXCOLOR);

	//WireFrame 대신 LineList 
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = vertices.data();

	if (FAILED(m_pDevice->CreateBuffer(&vbDesc, &init, &m_pVB)))
		return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_DebugCapsule::Bind_Resources()
{
	ID3D11Buffer* pVertexBuffers[] = {
		m_pVB,
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride,
	};

	_uint		iOffsets[] = {
		0,
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	return S_OK;
}

HRESULT CVIBuffer_DebugCapsule::Render()
{
	m_pContext->Draw(m_iNumVertices, 0);
	return S_OK;
}

CVIBuffer_DebugCapsule* CVIBuffer_DebugCapsule::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const DEBUGCAPSULE_DESC& Desc)
{
	CVIBuffer_DebugCapsule* pInstance = new CVIBuffer_DebugCapsule(pDevice, pContext);
	if(FAILED(pInstance->Initialize_Prototype(Desc)))
	{
		MSG_BOX("Failed to Created : CVIBuffer_DebugCapsule");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CVIBuffer_DebugCapsule::Free()
{
	__super::Free();
}
