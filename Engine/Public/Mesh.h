#pragma once

#include "VIBuffer.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)


class ENGINE_DLL CMesh final : public CVIBuffer
{
private:
	CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMesh() = default;

public:
	_wstring					Get_MaterialName() const				{ return m_strMaterialName; }
	_uint				        Get_BoneCount() const					{ return m_iNumBones; }

	// Animation_CS
	const vector<_uint>& 		Get_BoneIndices() const;
	const vector<_float4x4>&	Get_OffsetMatrices() const;

	void				        Update_BoneMatrices(const vector<class CBone*>& Bones, _float4x4* pOutTransposed);
public:
	virtual HRESULT		        Initialize_Prototype(const MeshPassData& passData);

private:
	MODEL				        m_eType									= { MODEL::END };


	_wstring					m_strMaterialName						= {};
	_uint				        m_iNumBones								= {};

	/*
	//Model 로 옮김 
	_float4x4			        m_BoneMatrices[g_iNumMeshBones]			= {};
	_float4x4			        m_TransposedBoneMatrices[g_iNumMeshBones] = {};
	*/


	vector<_uint>		        m_BoneIndices;
	vector<_float4x4>	        m_OffsetMatrices;

private:
	HRESULT				        Ready_NonAnimMesh(const vector<VTXMESH>& Vertices);
	HRESULT				        Ready_AnimMesh(const vector<VTXANIMMESH>& Vertices, const vector<uint32_t> BoneIndices, const  vector<_float4x4>& OffsetMatrices, const _uint& iBoneCount);


public:
	static CMesh*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const MeshPassData& passData);
	virtual void				Free() override;
};

NS_END