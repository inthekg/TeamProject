#include "Mesh.h"
#include "Model.h"
#include "Bone.h"
#include "Shader.h"
#include "Engine_Utility.h"


CMesh::CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer{ pDevice, pContext }
{
}


HRESULT CMesh::Initialize_Prototype(const MeshPassData& passData)
{
	Set_Name(CharArrayToWstring(passData.name));
	m_strMaterialName = CharArrayToWstring(passData.materialName);

	m_iNumVertexBuffers = 1;
	m_iNumVertices = passData.vertexCount;
	m_iNumIndices = passData.indexCount;

	m_iNumBones = passData.boneCount;
	m_iIndexStride = sizeof(_uint);
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	m_eType = (passData.hasAnimation ? MODEL::ANIM : MODEL::NONANIM);
	HRESULT hr = {};

	if (m_eType == MODEL::NONANIM)
		hr = Ready_NonAnimMesh(passData.NoAnimVertices);
	else if (m_eType == MODEL::ANIM)
		hr = Ready_AnimMesh(passData.AnimVertices,passData.BoneIndices, passData.OffsetMatrices, passData.boneCount);

	if (FAILED(hr)) return E_FAIL;

	D3D11_BUFFER_DESC IndexBufferDesc{};
	IndexBufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
	IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = 0;

	_uint* pIndices = new _uint[m_iNumIndices];
	ZeroMemory(pIndices, sizeof(_uint) * m_iNumIndices);

	for (_uint i = 0; i < m_iNumIndices ; i++)
	{
		pIndices[i] = passData.Indices[i];
	}

	D3D11_SUBRESOURCE_DATA      IndexInitialData{};
	IndexInitialData.pSysMem = pIndices;

	if (FAILED(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB)))
		return E_FAIL;

	Safe_Delete_Array(pIndices);

	return S_OK;
}


const vector<_uint>& CMesh::Get_BoneIndices() const
{
	return m_BoneIndices;
}

const vector<_float4x4>& CMesh::Get_OffsetMatrices() const
{
	return m_OffsetMatrices;
}

void CMesh::Update_BoneMatrices(const vector<CBone*>& Bones, _float4x4* pOutTransposed)
{
 	for (size_t i = 0; i < m_iNumBones; i++)
	{
		_matrix tempMat = XMLoadFloat4x4(&m_OffsetMatrices[i]) * XMLoadFloat4x4(Bones[m_BoneIndices[i]]->Get_CombinedTransformationMatrixPtr());
		//TODO : 셰이더로 행렬을 전달할 때는 전치해야 함 
		XMStoreFloat4x4(&pOutTransposed[i], XMMatrixTranspose(tempMat));
	}

}
HRESULT CMesh::Ready_NonAnimMesh(const vector<VTXMESH>& Vertices)
{
	m_iVertexStride	= sizeof(VTXMESH);

	D3D11_BUFFER_DESC						VertexBufferDesc{};			

	VertexBufferDesc.ByteWidth				= m_iVertexStride * m_iNumVertices;
	VertexBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
	VertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags			= 0;
	VertexBufferDesc.MiscFlags				= 0;
	VertexBufferDesc.StructureByteStride	= 0;

	VTXMESH* pVertices = new VTXMESH[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXMESH) * m_iNumVertices);

	for (_uint i = 0; i < m_iNumVertices; i++)
	{
		memcpy(&pVertices[i], &Vertices[i], sizeof(VTXMESH));
	}

	D3D11_SUBRESOURCE_DATA      VertexInitialData{};
	VertexInitialData.pSysMem = pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
		return E_FAIL;

	Safe_Delete_Array(pVertices);

	return S_OK;
}

HRESULT CMesh::Ready_AnimMesh(const vector<VTXANIMMESH>& Vertices,const vector<uint32_t> BoneIndices,const  vector<_float4x4>& OffsetMatrices,const _uint& iBoneCount)
{
	m_iVertexStride = sizeof(VTXANIMMESH);
	D3D11_BUFFER_DESC           VertexBufferDesc{};

	VertexBufferDesc.ByteWidth				= m_iVertexStride * m_iNumVertices;
	VertexBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
	VertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags			= 0;
	VertexBufferDesc.MiscFlags				= 0;
	VertexBufferDesc.StructureByteStride	= 0;

	VTXANIMMESH* pVertices = new VTXANIMMESH[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXANIMMESH) * m_iNumVertices);

	for (size_t i = 0; i < m_iNumVertices; i++)
	{
		memcpy(&pVertices[i], &Vertices[i], sizeof(VTXANIMMESH));
	}

	//정점마다 영향을 받는 Bone 정보 저장 
	m_iNumBones = iBoneCount;

	m_BoneIndices.reserve(m_iNumBones);
	m_OffsetMatrices.reserve(m_iNumBones);

	m_BoneIndices = BoneIndices;
	m_OffsetMatrices = OffsetMatrices;

	D3D11_SUBRESOURCE_DATA      VertexInitialData{};
	VertexInitialData.pSysMem = pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
		return E_FAIL;

	Safe_Delete_Array(pVertices);

	return S_OK;
	
}


CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const MeshPassData& passData)
{
	CMesh* pInstance = new CMesh(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(passData)))
	{
		MSG_BOX("Failed to Created : CMesh");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMesh::Free()
{
	__super::Free();

}
