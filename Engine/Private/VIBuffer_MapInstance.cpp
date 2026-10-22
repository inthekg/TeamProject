#include "VIBuffer_MapInstance.h"
#include "ComputeShader.h"

CVIBuffer_MapInstance::CVIBuffer_MapInstance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer{ pDevice, pContext }
{
}

HRESULT CVIBuffer_MapInstance::Initialize_Prototype(const MeshPassData& passData, const vector<MapInstanceData>& instances)
{
    if (passData.hasAnimation)
        return E_FAIL;

    if (0 == passData.vertexCount || 0 == passData.indexCount)
        return E_FAIL;

    if (instances.empty())
        return E_FAIL;

    m_iNumVertexBuffers = 2;

    m_iNumVertices = passData.vertexCount;
    m_iVertexStride = sizeof(VTXMESH);

    m_iNumIndices = passData.indexCount;
    m_iIndexStride = sizeof(_uint);

    m_iNumInstances = static_cast<_uint>(instances.size());
    m_iVisibleInstanceCount = m_iNumInstances;
    m_iInstanceStride = sizeof(VTXMAP_INSTANCE);
    m_iIndexCountPerInstance = passData.indexCount;

    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    if (FAILED(Create_VertexBuffer(passData)))
        return E_FAIL;

    if (FAILED(Create_IndexBuffer(passData)))
        return E_FAIL;

    if (FAILED(Create_InstanceBuffer(instances)))
        return E_FAIL;

    return S_OK;
}


HRESULT CVIBuffer_MapInstance::Render()
{
    if (m_bGlobalIndirectVisiblePath)
    {
        if (nullptr == m_pExternalIndirectArgsBuffer)
            return E_FAIL;

        m_pContext->DrawIndexedInstancedIndirect(m_pExternalIndirectArgsBuffer, m_iExternalIndirectArgsOffset);
        return S_OK;
    }

    if (0 == m_iVisibleInstanceCount)
        return S_OK;

    m_pContext->DrawIndexedInstanced(m_iIndexCountPerInstance, m_iVisibleInstanceCount, 0, 0, 0);
    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Update_InstanceWorldMatrix(_uint iInstanceIndex, const _float4x4& WorldMatrix)
{
    if (nullptr == m_pVBInstance)
        return E_FAIL;

    if (iInstanceIndex >= m_iNumInstances)
        return E_FAIL;

    VTXMAP_INSTANCE InstanceVertex{};

    InstanceVertex.vRight =
        _float4(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13, WorldMatrix._14);

    InstanceVertex.vUp =
        _float4(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23, WorldMatrix._24);

    InstanceVertex.vLook =
        _float4(WorldMatrix._31, WorldMatrix._32, WorldMatrix._33, WorldMatrix._34);

    InstanceVertex.vTranslation =
        _float4(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43, WorldMatrix._44);

    D3D11_BOX Box{};
    Box.left = iInstanceIndex * m_iInstanceStride;
    Box.right = Box.left + m_iInstanceStride;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pVBInstance, 0, &Box, &InstanceVertex, 0, 0);

    if (nullptr != m_pInstanceWorldBuffer)
    {
        Unbind_IndexedInstanceSRVs();

        // 에디터에서 개별 transform 변경 시 indexed path의 world table도 같이 갱신한다.
        m_pContext->UpdateSubresource(m_pInstanceWorldBuffer, 0, &Box, &InstanceVertex, 0, 0);
    }


    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Update_VisibleInstances(const vector<_float4x4>& VisibleWorldMatrices)
{
    m_bIndexedVisiblePath = false;


    if (nullptr == m_pVBInstance)
        return E_FAIL;

    if (VisibleWorldMatrices.size() > m_iNumInstances)
        return E_FAIL;

    m_iVisibleInstanceCount = static_cast<_uint>(VisibleWorldMatrices.size());

    if (0 == m_iVisibleInstanceCount)
        return S_OK;

    vector<VTXMAP_INSTANCE> VisibleInstances;
    VisibleInstances.resize(m_iVisibleInstanceCount);

    for (_uint i = 0; i < m_iVisibleInstanceCount; ++i)
    {
        const _float4x4& WorldMatrix = VisibleWorldMatrices[i];

        VisibleInstances[i].vRight =
            _float4(WorldMatrix._11, WorldMatrix._12,
                    WorldMatrix._13, WorldMatrix._14);

        VisibleInstances[i].vUp =
            _float4(WorldMatrix._21, WorldMatrix._22,
                    WorldMatrix._23, WorldMatrix._24);

        VisibleInstances[i].vLook =
            _float4(WorldMatrix._31, WorldMatrix._32,
                    WorldMatrix._33, WorldMatrix._34);

        VisibleInstances[i].vTranslation =
            _float4(WorldMatrix._41, WorldMatrix._42,
                    WorldMatrix._43, WorldMatrix._44);
    }

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = m_iVisibleInstanceCount * m_iInstanceStride;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pVBInstance, 0, &Box, VisibleInstances.data(), 0, 0);

    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Update_VisibleInstanceIndices(const vector<_uint>& VisibleInstanceIndices)
{
    m_bIndexedVisiblePath = true;

    if (nullptr == m_pInstanceWorldBuffer || nullptr == m_pInstanceWorldSRV)
        return E_FAIL;

    if (VisibleInstanceIndices.size() > m_iNumInstances)
        return E_FAIL;

    for (_uint iInstanceIndex : VisibleInstanceIndices)
    {
        if (iInstanceIndex >= m_iNumInstances)
            return E_FAIL;
    }

    m_iVisibleInstanceCount = static_cast<_uint>(VisibleInstanceIndices.size());

    if (0 == m_iVisibleInstanceCount)
        return S_OK;

    if (nullptr == m_pVisibleIndexBuffer || m_iVisibleIndexCapacity < m_iVisibleInstanceCount)
    {
        Unbind_IndexedInstanceSRVs();

        Safe_Release(m_pVisibleIndexSRV);
        Safe_Release(m_pVisibleIndexBuffer);

        if (FAILED(Create_VisibleIndexBuffer(m_iVisibleInstanceCount)))
            return E_FAIL;
    }

    Unbind_IndexedInstanceSRVs();

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = m_iVisibleInstanceCount * sizeof(_uint);
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pVisibleIndexBuffer, 0, &Box, VisibleInstanceIndices.data(), 0, 0);

    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Rebuild_InstanceBuffer(const vector<_float4x4>& WorldMatrices)
{
    Unbind_IndexedInstanceSRVs();

    Safe_Release(m_pVisibleIndexSRV);
    Safe_Release(m_pVisibleIndexBuffer);

    Safe_Release(m_pInstanceWorldSRV);
    Safe_Release(m_pInstanceWorldBuffer);
    Safe_Release(m_pVBInstance);

    m_iVisibleIndexCapacity = 0;
    m_bIndexedVisiblePath = false;

    if (WorldMatrices.empty())
    {
        m_iNumInstances = 0;
        m_iVisibleInstanceCount = 0;
        return S_OK;
    }

    vector<MapInstanceData> Instances;
    Instances.resize(WorldMatrices.size());

    _uint Size = static_cast<_uint>(WorldMatrices.size());

    for (_uint i = 0; i < Size; ++i)
    {
        strcpy_s(Instances[i].instanceName, "FloorTile");
        Instances[i].worldMatrix = WorldMatrices[i];
    }

    m_iNumInstances = static_cast<_uint>(Instances.size());
    m_iVisibleInstanceCount = m_iNumInstances;

    return Create_InstanceBuffer(Instances);
}

HRESULT CVIBuffer_MapInstance::Set_GlobalIndirectResources(ID3D11ShaderResourceView* pGlobalWorldSRV, ID3D11ShaderResourceView* pVisibleIndexSRV, ID3D11Buffer* pIndirectArgsBuffer, _uint iIndirectArgsOffset, _uint iVisibleBase)
{
    if (nullptr == pGlobalWorldSRV || nullptr == pVisibleIndexSRV || nullptr == pIndirectArgsBuffer)
        return E_FAIL;

    Clear_GlobalIndirectResources();

    // 외부 global indirect 리소스는 CMapRenderInstancePool이 소유한다.
    // CVIBuffer_MapInstance는 이번 draw에 사용할 포인터만 빌려서 저장한다.
    m_pExternalGlobalWorldSRV = pGlobalWorldSRV;
    m_pExternalVisibleIndexSRV = pVisibleIndexSRV;
    m_pExternalIndirectArgsBuffer = pIndirectArgsBuffer;
    m_iExternalIndirectArgsOffset = iIndirectArgsOffset;
    m_iExternalVisibleBase = iVisibleBase;
    m_bGlobalIndirectVisiblePath = true;

    return Ensure_MapInstanceRenderDescBuffer();
}

HRESULT CVIBuffer_MapInstance::Prepare_RenderResources()
{
    // 첫 간접 드로우에서 생성되던 오너별 상수 버퍼를 섹터 진입 전에 준비한다.
    return Ensure_MapInstanceRenderDescBuffer();
}

void CVIBuffer_MapInstance::Clear_GlobalIndirectResources()
{
    m_bGlobalIndirectVisiblePath = false;
    m_iExternalIndirectArgsOffset = 0;
    m_iExternalVisibleBase = 0;

    // 외부 포인터는 소유하지 않으므로 Release하지 않고 참조만 끊는다.
    m_pExternalGlobalWorldSRV = nullptr;
    m_pExternalVisibleIndexSRV = nullptr;
    m_pExternalIndirectArgsBuffer = nullptr;
}

HRESULT CVIBuffer_MapInstance::Ensure_MapInstanceRenderDescBuffer()
{
    if (nullptr != m_pMapInstanceRenderDescBuffer)
        return S_OK;

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(MAP_INSTANCE_RENDER_DESC);
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = 0;
    BufferDesc.StructureByteStride = 0;

    return m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pMapInstanceRenderDescBuffer);
}

HRESULT CVIBuffer_MapInstance::Create_VertexBuffer(const MeshPassData& passData)
{
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = passData.NoAnimVertices.data();

    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, &initialData, &m_pVB)))
        return E_FAIL;

    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Create_IndexBuffer(const MeshPassData& passData)
{
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = passData.Indices.data();

    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, &initialData, &m_pIB)))
        return E_FAIL;

    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Create_InstanceBuffer(const vector<MapInstanceData>& instances)
{
    vector<VTXMAP_INSTANCE> instanceVertices;
    instanceVertices.resize(instances.size());

    for (_uint i = 0; i < static_cast<_uint>(instances.size()); ++i)
    {
        const _float4x4& worldMatrix = instances[i].worldMatrix;

        instanceVertices[i].vRight =
            _float4(worldMatrix._11, worldMatrix._12, worldMatrix._13, worldMatrix._14);

        instanceVertices[i].vUp =
            _float4(worldMatrix._21, worldMatrix._22, worldMatrix._23, worldMatrix._24);

        instanceVertices[i].vLook =
            _float4(worldMatrix._31, worldMatrix._32, worldMatrix._33, worldMatrix._34);

        instanceVertices[i].vTranslation =
            _float4(worldMatrix._41, worldMatrix._42, worldMatrix._43, worldMatrix._44);
    }

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = m_iInstanceStride * m_iNumInstances;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = instanceVertices.data();

    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, &initialData, &m_pVBInstance)))
        return E_FAIL;

    if (FAILED(Create_InstanceWorldBuffer(instanceVertices)))
        return E_FAIL;

    return S_OK;
}

HRESULT CVIBuffer_MapInstance::Create_InstanceWorldBuffer(const vector<VTXMAP_INSTANCE>& InstanceVertices)
{
    if (InstanceVertices.empty())
        return E_FAIL;

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<_uint>(sizeof(VTXMAP_INSTANCE) * InstanceVertices.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(VTXMAP_INSTANCE);

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = InstanceVertices.data();

    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, &initialData, &m_pInstanceWorldBuffer)))
        return E_FAIL;

    return Create_StructuredSRV(m_pInstanceWorldBuffer, static_cast<_uint>(InstanceVertices.size()), &m_pInstanceWorldSRV);
}

HRESULT CVIBuffer_MapInstance::Create_VisibleIndexBuffer(_uint iCapacity)
{
    if (0 == iCapacity)
        iCapacity = 1;

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = sizeof(_uint) * iCapacity;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(_uint);

    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pVisibleIndexBuffer)))
        return E_FAIL;

    if (FAILED(Create_StructuredSRV(m_pVisibleIndexBuffer, iCapacity, &m_pVisibleIndexSRV)))
        return E_FAIL;

    m_iVisibleIndexCapacity = iCapacity;
    return S_OK;
}


HRESULT CVIBuffer_MapInstance::Create_StructuredSRV(ID3D11Buffer* pBuffer, _uint iElementCount, ID3D11ShaderResourceView** ppOutSRV)
{
    if (nullptr == pBuffer || nullptr == ppOutSRV || 0 == iElementCount)
        return E_FAIL;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = iElementCount;

    return m_pDevice->CreateShaderResourceView(pBuffer, &srvDesc, ppOutSRV);
}

void CVIBuffer_MapInstance::Unbind_IndexedInstanceSRVs()
{
    ID3D11ShaderResourceView* pNullSRVs[2] = { nullptr, nullptr };
    m_pContext->VSSetShaderResources(MAP_INSTANCE_WORLD_SRV_SLOT, 2, pNullSRVs);
}



HRESULT CVIBuffer_MapInstance::Bind_Resources()
{
    ID3D11Buffer* pVertexBuffers[] =
    {
        m_pVB,
        m_pVBInstance
    };

    _uint vertexStrides[] =
    {
        m_iVertexStride,
        m_iInstanceStride
    };

    _uint offsets[] =
    {
        0,
        0
    };

    m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, vertexStrides, offsets);
    m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
    m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

    if (m_bGlobalIndirectVisiblePath)
    {
        if (nullptr == m_pExternalGlobalWorldSRV || nullptr == m_pExternalVisibleIndexSRV || nullptr == m_pMapInstanceRenderDescBuffer)
            return E_FAIL;

        MAP_INSTANCE_RENDER_DESC RenderDesc{};
        RenderDesc.iVisibleBase = static_cast<int>(m_iExternalVisibleBase);

        m_pContext->UpdateSubresource(m_pMapInstanceRenderDescBuffer, 0, nullptr, &RenderDesc, 0, 0);
        m_pContext->VSSetConstantBuffers(8, 1, &m_pMapInstanceRenderDescBuffer);

        ID3D11ShaderResourceView* pSRVs[2] = { m_pExternalGlobalWorldSRV, m_pExternalVisibleIndexSRV };
        m_pContext->VSSetShaderResources(MAP_INSTANCE_WORLD_SRV_SLOT, 2, pSRVs);
    }
    else if (m_bIndexedVisiblePath)
    {
        ID3D11ShaderResourceView* pSRVs[2] = { m_pInstanceWorldSRV, m_pVisibleIndexSRV };
        m_pContext->VSSetShaderResources(MAP_INSTANCE_WORLD_SRV_SLOT, 2, pSRVs);
    }
    else
    {
        Unbind_IndexedInstanceSRVs();
    }


    return S_OK;
}


CVIBuffer_MapInstance* CVIBuffer_MapInstance::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const MeshPassData& passData, const vector<MapInstanceData>& instances)
{
    CVIBuffer_MapInstance* pInstance = new CVIBuffer_MapInstance(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(passData, instances)))
    {
        MSG_BOX("Failed to Created : CVIBuffer_MapInstance");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CVIBuffer_MapInstance::Free()
{
    Unbind_IndexedInstanceSRVs();

    Clear_GlobalIndirectResources();
    Safe_Release(m_pMapInstanceRenderDescBuffer);

    Safe_Release(m_pVisibleIndexSRV);
    Safe_Release(m_pVisibleIndexBuffer);


    Safe_Release(m_pInstanceWorldSRV);
    Safe_Release(m_pInstanceWorldBuffer);
    Safe_Release(m_pVBInstance);

    __super::Free();
}
