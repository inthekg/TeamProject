#include "VIBuffer_GPUMesh.h"
#include "ModelDataStruct.h"

CVIBuffer_GPUMesh::CVIBuffer_GPUMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{

}

HRESULT CVIBuffer_GPUMesh::Initialize_Prototype()
{
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_iVertexStride = sizeof(VTXMESH);
	m_iIndexStride = sizeof(_uint);
	
	return S_OK;
}

HRESULT CVIBuffer_GPUMesh::Build_Quad()
{
	// 큐브 형태의 Mesh를 에디터에 출력하기 위한 임시 객체.
	// Mesh 데이터 읽기에 실패했을 때도 이 함수를 호출하여 큐브를 생성한다 (크래시 방지)

	Safe_Release(m_pVB);
	Safe_Release(m_pIB);

	// XY 평면 단위 쿼드, tangent/binormal은 사용하지 않음. (이펙트는 노말맵 사용하지 않을 예정)
	VTXMESH verts[4] =
	{
		{ {-0.5f,  0.5f, 0.f}, {0.f,0.f,1.f}, {0.f,0.f}, {}, {} },
		{ { 0.5f,  0.5f, 0.f}, {0.f,0.f,1.f}, {1.f,0.f}, {}, {} },
		{ { 0.5f, -0.5f, 0.f}, {0.f,0.f,1.f}, {1.f,1.f}, {}, {} },
		{ {-0.5f, -0.5f, 0.f}, {0.f,0.f,1.f}, {0.f,1.f}, {}, {} },
	};

	_uint indices[6] = { 0, 1, 2, 0, 2, 3 };

	m_iNumVertices = 4;
	m_iNumIndices = 6;
	m_iNumVertexBuffers = 1;

	D3D11_BUFFER_DESC vbd{};
	vbd.ByteWidth = sizeof(verts);
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vinit{ verts, 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&vbd, &vinit, &m_pVB)))
		return E_FAIL;

	D3D11_BUFFER_DESC ibd{};
	ibd.ByteWidth = sizeof(indices);
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iinit{ indices, 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&ibd, &iinit, &m_pIB)))
		return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_GPUMesh::Load_StaticMesh(const _wstring& strFilePath, _uint iMeshIndex)
{
	// .data 포맷의 파일을 읽어온다. (Model의 Mesh와 통합된 데이터 구조 유지를 위해)
	// 순차적으로 파일의 정보를 읽어오되, GPUMesh에서 쓰지 않을 데이터들은 seekg로 스킵
	// Skeletal 모델의 경우는 읽어오지 않음. Static Mesh만 읽어올 것
	ifstream fin(strFilePath, ios::binary);
	if (!fin.is_open())
		return E_FAIL;

	uint32_t iBoneCount = 0, iMeshCount = 0;
	fin.read(reinterpret_cast<char*>(&iBoneCount), sizeof(uint32_t));
	fin.read(reinterpret_cast<char*>(&iMeshCount), sizeof(uint32_t));
	if (iMeshCount == 0)
		return E_FAIL;
	if (iMeshIndex >= iMeshCount)
		iMeshIndex = 0;

	// 본 데이터 스킵
	fin.seekg(static_cast<streamoff>(iBoneCount) * sizeof(BoneData), ios::cur);

	vector<VTXMESH> verts;
	vector<_uint> indices;
	
	bool bFound = false;

	for (uint32_t i = 0; i < iMeshCount; ++i)
	{
		MeshData header{};
		fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));
		if (!fin)
			return E_FAIL;

		if (i == iMeshIndex)
		{
			if (header.HasAnimation) // Skeletal 애니메이션은 배제
				return E_FAIL;

			verts.resize(header.vertexCount);
			fin.read(reinterpret_cast<char*>(verts.data()), static_cast<streamsize>(sizeof(VTXMESH)) * header.vertexCount);

			indices.resize(header.indexCount);
			fin.read(reinterpret_cast<char*>(indices.data()), static_cast<streamsize>(sizeof(_uint)) * header.indexCount);

			bFound = true;
			break;
		}

		// 사용하지 않는 정보 생략.
		fin.seekg(static_cast<streamoff>(header.vertexStride) * header.vertexCount, ios::cur);
		fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.indexCount, ios::cur);
		fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.boneCount, ios::cur);
		fin.seekg(static_cast<streamoff>(sizeof(_float4x4)) * header.boneCount, ios::cur);
	}

	if (!bFound || verts.empty() || indices.empty() || !fin)
		return E_FAIL;

	Safe_Release(m_pVB);
	Safe_Release(m_pIB);

	m_iNumVertices = static_cast<_uint>(verts.size());
	m_iNumIndices = static_cast<_uint>(indices.size());
	m_iNumVertexBuffers = 1;

	D3D11_BUFFER_DESC vbd{};
	vbd.ByteWidth = static_cast<_uint>(sizeof(VTXMESH) * verts.size());
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vinit{ verts.data(), 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&vbd, &vinit, &m_pVB)))
		return E_FAIL;

	D3D11_BUFFER_DESC ibd{};
	ibd.ByteWidth = static_cast<_uint>(sizeof(_uint) * indices.size());
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iinit{ indices.data(), 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&ibd, &iinit, &m_pIB)))
		return E_FAIL;

	return S_OK;
}

void CVIBuffer_GPUMesh::Set_DrawSource(ID3D11ShaderResourceView* pPoolSRV,
	ID3D11ShaderResourceView* pAliveSRV, ID3D11Buffer* pIndirectArgs,
	_uint iFixedCount, ID3D11Buffer* pCameraCB)
{
	m_pPoolSRV = pPoolSRV;
	m_pAliveSRV = pAliveSRV;
	m_pIndirectArgs = pIndirectArgs;
	m_iFixedCount = iFixedCount;
	m_pCameraCB = pCameraCB;
}

HRESULT CVIBuffer_GPUMesh::Bind_Resources()
{
	// InputLayout은 렌더러에서 바인딩
	_uint iStride = m_iVertexStride, iOffset = 0;
	m_pContext->IASetVertexBuffers(0, 1, &m_pVB, &iStride, &iOffset);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	m_pContext->VSSetShaderResources(0, 1, &m_pPoolSRV);
	if (m_pAliveSRV)
		m_pContext->VSSetShaderResources(1, 1, &m_pAliveSRV);
	m_pContext->VSSetShaderResources(3, 1, &m_pScale3DSRV);
	if (m_pCameraCB)
	{
		m_pContext->VSSetConstantBuffers(7, 1, &m_pCameraCB);
		m_pContext->PSSetConstantBuffers(7, 1, &m_pCameraCB);
	}
	if (m_pRenderCB)
	{
		m_pContext->VSSetConstantBuffers(6, 1, &m_pRenderCB);
		m_pContext->PSSetConstantBuffers(6, 1, &m_pRenderCB);
	}

	m_pContext->PSSetShaderResources(2, 1, &m_pDiffuseSRV);
	m_pContext->PSSetShaderResources(4, 1, &m_pNoiseASRV);
	m_pContext->PSSetShaderResources(5, 1, &m_pNoiseBSRV);
	m_pContext->PSSetShaderResources(6, 1, &m_pShapeMaskSRV);
	m_pContext->PSSetShaderResources(7, 1, &m_pEmissiveBSRV);
	m_pContext->PSSetShaderResources(8, 1, &m_pDissolveSRV);
	m_pContext->PSSetShaderResources(9, 1, &m_pCrackSRV);
	m_pContext->PSSetShaderResources(10, 1, &m_pSceneSRV);
	m_pContext->PSSetShaderResources(11, 1, &m_pDistortNoiseSRV);
	m_pContext->PSSetShaderResources(12, 1, &m_pEmissiveASRV);

	return S_OK;
}

HRESULT CVIBuffer_GPUMesh::Render()
{
	if (m_pIndirectArgs)
		m_pContext->DrawIndexedInstancedIndirect(m_pIndirectArgs, 0);

	ID3D11ShaderResourceView* nullSRV[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pContext->VSSetShaderResources(0, 2, nullSRV);
	m_pContext->VSSetShaderResources(3, 1, nullSRV);
	m_pContext->PSSetShaderResources(2, 1, nullSRV);
	m_pContext->PSSetShaderResources(4, 6, nullSRV);
	m_pContext->PSSetShaderResources(10, 1, nullSRV);
	m_pContext->PSSetShaderResources(11, 1, nullSRV);
	m_pContext->PSSetShaderResources(12, 1, nullSRV);
	return S_OK;
}

CVIBuffer_GPUMesh* CVIBuffer_GPUMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVIBuffer_GPUMesh* pInstance = new CVIBuffer_GPUMesh(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVIBuffer_GPUMesh");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_GPUMesh::Free()
{
	__super::Free();
}