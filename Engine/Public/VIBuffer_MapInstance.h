#pragma once

#include "VIBuffer.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)

class CComputeShader;

class ENGINE_DLL CVIBuffer_MapInstance final : public CVIBuffer 
{
private:
	CVIBuffer_MapInstance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CVIBuffer_MapInstance() = default;

public:
	HRESULT Initialize_Prototype(const MeshPassData& passData, const vector<MapInstanceData>& instances);
	virtual HRESULT Bind_Resources()override;
	virtual HRESULT Render() override;

	HRESULT Update_InstanceWorldMatrix(_uint iInstanceIndex, const _float4x4& WorldMatrix);
	HRESULT Update_VisibleInstances(const vector<_float4x4>& VisibleWorldMatrices);
	HRESULT Update_VisibleInstanceIndices(const vector<_uint>& VisibleInstanceIndices);

	HRESULT Rebuild_InstanceBuffer(const vector<_float4x4>& WorldMatrices);

public:
	_uint Get_IndexCountPerInstance() const { return m_iIndexCountPerInstance; }
	HRESULT Prepare_RenderResources();
	HRESULT Set_GlobalIndirectResources(ID3D11ShaderResourceView* pGlobalWorldSRV, ID3D11ShaderResourceView* pVisibleIndexSRV, ID3D11Buffer* pIndirectArgsBuffer, _uint iIndirectArgsOffset, _uint iVisibleBase);
	void Clear_GlobalIndirectResources();

private:
	HRESULT Create_VertexBuffer(const MeshPassData& passData);
	HRESULT Create_IndexBuffer(const MeshPassData& passData);
	HRESULT Create_InstanceBuffer(const vector<MapInstanceData>& instances);

	HRESULT Create_InstanceWorldBuffer(const vector<VTXMAP_INSTANCE>& InstanceVertices);
	HRESULT Create_VisibleIndexBuffer(_uint iCapacity);
	HRESULT Create_StructuredSRV(ID3D11Buffer* pBuffer, _uint iElementCount, ID3D11ShaderResourceView** ppOutSRV);
	void Unbind_IndexedInstanceSRVs();

	HRESULT Ensure_MapInstanceRenderDescBuffer();

private:
	enum
	{
		MAP_INSTANCE_WORLD_SRV_SLOT = 24,
		MAP_VISIBLE_INDEX_SRV_SLOT = 25,
		MAP_INSTANCE_BOUNDS_SRV_SLOT = 26,
		MAP_CANDIDATE_INDEX_SRV_SLOT = 27
	};

	ID3D11Buffer*				m_pVBInstance = { nullptr };
	ID3D11Buffer*				m_pInstanceWorldBuffer = { nullptr };
	ID3D11ShaderResourceView*	m_pInstanceWorldSRV = { nullptr };

	ID3D11Buffer*				m_pVisibleIndexBuffer = { nullptr };
	ID3D11ShaderResourceView*	m_pVisibleIndexSRV = { nullptr };

	ID3D11Buffer*				m_pMapInstanceRenderDescBuffer = nullptr;

	ID3D11ShaderResourceView*	m_pExternalGlobalWorldSRV = nullptr;
	ID3D11ShaderResourceView*	m_pExternalVisibleIndexSRV = nullptr;
	ID3D11Buffer*				m_pExternalIndirectArgsBuffer = nullptr;


	_bool						m_bIndexedVisiblePath = {};
	_bool						m_bGlobalIndirectVisiblePath = { false };

	_uint                       m_iVisibleIndexCapacity = {};

	_uint						m_iNumInstances = {};
	_uint						m_iVisibleInstanceCount = {};
	_uint						m_iInstanceStride = {};
	_uint						m_iIndexCountPerInstance = {};

	_uint						m_iExternalIndirectArgsOffset = 0;
	_uint						m_iExternalVisibleBase = 0;



public:
	static CVIBuffer_MapInstance* Create(ID3D11Device* pDevice,ID3D11DeviceContext* pContext,const MeshPassData& passData,const vector<MapInstanceData>& instances);

	virtual void Free() override;

};

NS_END
