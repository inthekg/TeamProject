#include "VIBuffer_Cube.h"

CVIBuffer_Cube::CVIBuffer_Cube(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{

}

CVIBuffer_Cube::CVIBuffer_Cube(const CVIBuffer_Cube& Prototype)
	: CVIBuffer { Prototype }	
{
	
}

HRESULT CVIBuffer_Cube::Initialize_Prototype()
{
    Set_Name(TEXT("VIBUFFER_CUBE"));

    m_iNumVertexBuffers = 1;
    m_iNumVertices = 8;
    m_iVertexStride = sizeof(VTXCUBE);

    m_iNumIndices = 36;
    m_iIndexStride = 2;
    m_eIndexFormat = DXGI_FORMAT_R16_UINT;

    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;



    /* 네모를 표현해주기 위한 정점, 인덱스를 생성한다. */
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;

    VTXCUBE* pVertices = new VTXCUBE[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXCUBE) * m_iNumVertices);

    pVertices[0].vPosition = _float3(-0.5f, 0.5f, -0.5f);
    pVertices[0].vTexcoord = pVertices[0].vPosition;

    pVertices[1].vPosition = _float3(0.5f, 0.5f, -0.5f);
    pVertices[1].vTexcoord = pVertices[1].vPosition;

    pVertices[2].vPosition = _float3(0.5f, -0.5f, -0.5f);
    pVertices[2].vTexcoord = pVertices[2].vPosition;

    pVertices[3].vPosition = _float3(-0.5f, -0.5f, -0.5f);
    pVertices[3].vTexcoord = pVertices[3].vPosition;

    pVertices[4].vPosition = _float3(-0.5f, 0.5f, 0.5f);
    pVertices[4].vTexcoord = pVertices[4].vPosition;

    pVertices[5].vPosition = _float3(0.5f, 0.5f, 0.5f);
    pVertices[5].vTexcoord = pVertices[5].vPosition;

    pVertices[6].vPosition = _float3(0.5f, -0.5f, 0.5f);
    pVertices[6].vTexcoord = pVertices[6].vPosition;

    pVertices[7].vPosition = _float3(-0.5f, -0.5f, 0.5f);
    pVertices[7].vTexcoord = pVertices[7].vPosition;

    D3D11_SUBRESOURCE_DATA      VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
        return E_FAIL;

    Safe_Delete_Array(pVertices);



    D3D11_BUFFER_DESC           IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags = 0;
    IndexBufferDesc.MiscFlags = 0;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;


    _ushort* pIndices = new _ushort[m_iNumIndices];
    ZeroMemory(pIndices, sizeof(_ushort) * m_iNumIndices);

    // X+	
    pIndices[0] = 1; pIndices[1] = 5; pIndices[2] = 6;
    pIndices[3] = 1; pIndices[4] = 6; pIndices[5] = 2;

    pIndices[6] = 4; pIndices[7] = 0; pIndices[8] = 3;
    pIndices[9] = 4; pIndices[10] = 3; pIndices[11] = 7;	// Y+

    pIndices[12] = 4; pIndices[13] = 5; pIndices[14] = 1;
    pIndices[15] = 4; pIndices[16] = 1; pIndices[17] = 0;

    pIndices[18] = 3; pIndices[19] = 2; pIndices[20] = 6;
    pIndices[21] = 3; pIndices[22] = 6; pIndices[23] = 7;

    pIndices[24] = 7; pIndices[25] = 6; pIndices[26] = 5;
    pIndices[27] = 7; pIndices[28] = 5; pIndices[29] = 4;

    pIndices[30] = 0; pIndices[31] = 1; pIndices[32] = 2;
    pIndices[33] = 0; pIndices[34] = 2; pIndices[35] = 3;


    D3D11_SUBRESOURCE_DATA      IndexInitialData{};
    IndexInitialData.pSysMem = pIndices;

    if (FAILED(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB)))
        return E_FAIL;

    Safe_Delete_Array(pIndices);

	return S_OK;
}

HRESULT CVIBuffer_Cube::Initialize(void* pArg)
{
	return S_OK;
}

CVIBuffer_Cube* CVIBuffer_Cube::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CVIBuffer_Cube* pInstance = new CVIBuffer_Cube(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CVIBuffer_Cube");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVIBuffer_Cube::Free()
{
	__super::Free();



}
