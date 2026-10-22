#pragma once
#include "Component.h"
#include "RenderStruct.h"

NS_BEGIN(Engine)

class ENGINE_DLL CModel : public CComponent
{
protected:
	//메시마다 연관되는 본행렬 집합체 
	struct BoneMatrixSet
	{
		vector<_float4x4> m_TransposedBoneMatrices = {};
	};

protected:
	CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CModel(const CModel& Prototype);
	virtual ~CModel() = default;

public:
	_uint Get_NumBones() const
	{
		return (_uint)m_Bones.size();
	}
	_uint Get_NumMeshes() const 
	{
		return m_iNumMeshes;
	}
	_uint Get_NumMaterials() const
	{
		return m_iNumMaterials;
	}
	_uint Get_BoneVersion() const
	{
		return m_iBoneVersion;
	}

	const vector<class CBone*>&			Get_Bones() const { return m_Bones; }
	_int								Get_BoneIndex(const _char* pBoneName);

	const _float4x4*					Get_Bone_CombinedMatPtr(_uint iBoneIndex);
	const _float4x4*					Get_Bone_CombinedMatPtr(const _char* pBoneName);
	const _char*						Get_BoneName(_uint iBoneIndex) const;

	const _float4x4*					Get_TransposedBoneMatrices(_uint iMeshIndex) const;

	_uint								Get_MeshBoneMatrix_Count(_uint iMeshIndex) const
	{
		return (_uint)m_BoneMatrixSet[iMeshIndex].m_TransposedBoneMatrices.size();
	}

	class CMesh*						Get_Mesh(_uint iMeshIndex);
	class CMaterial*					Get_Material(_uint iMeshIndex);
	MODEL								Get_ModelType() const { return m_eType; }

	vector<TransformData>*				Get_BindPosePtr() { return &m_BasePose; }
	const _float4x4&					Get_PreTransformMatrix() const { return m_PreTransformMatrix; }
	const _wstring&						Get_MeshBoneFilePath() const { return m_strMeshBoneFilePath; }
	//오직 피직스 보정용으로만 사용!
	void								Set_PreTransformMatrix(_fmatrix NewPreTransformMatrix) { XMStoreFloat4x4(&m_PreTransformMatrix, NewPreTransformMatrix); }

	// Animation_CS
	ID3D11ShaderResourceView*			Get_BoneMatrixSRV() const;
	ID3D11UnorderedAccessView*			Get_BoneMatrixUAV() const;
		
	_uint								Get_MeshBoneOffset(_uint iMeshIndex) const;
	_uint								Get_TotalPackedMeshBoneCount() const;

	_uint								Get_GPUFinalBoneOffset() const { return m_iGPUFinalBoneOffset; }
	_uint								Get_GPUFinalBoneCount() const { return m_iGPUFinalBoneCount; }
	_bool								Has_GPUFinalBoneRange() const { return m_bHasGPUFinalBoneRange; }
	_bool								Is_UseComputeMeshBone() const { return m_bUseComputeMeshBone; }
	
	_uint								Get_GPUCombinedBoneOffset() const { return m_iGPUCombinedBoneOffset; }
	_uint								Get_GPUCombinedBoneCount() const { return m_iGPUCombinedBoneCount; }
	_bool								Has_GPUCombinedBoneRange() const { return m_bHasGPUCombinedBoneRange; }

	HRESULT								Fill_SkinnedCommandBoneInfo(SkinnedCommand& command, _uint iMeshIndex) const;
	HRESULT								Fill_MeshSkinPalette(_uint iMeshIndex, vector<_float4x4>& OutPalette) const;
	HRESULT								Fill_ModelSkinPalette(vector<_float4x4>& OutPalette) const;

	_uint								Get_GPUMeshBoneInputOffset() const { return m_iGPUMeshBoneInputOffset; }
	_uint								Get_GPUMeshBoneInputCount() const { return m_iGPUMeshBoneInputCount; }
	_bool								Has_GPUMeshBoneInputRange() const { return m_bHasGPUMeshBoneInputRange; }

public:
	virtual HRESULT						Initialize_Prototype(_wstring strModelName, _fmatrix PreTransformMatrix);
	virtual HRESULT						Initialize(void* pArg);
	
public: 	
	//Bone행렬 Update + 누적
	void								Update_Bones(const vector<TransformData>* pFinalPose = nullptr);
	const vector<RenderUnit>&			Get_RenderUnits();
	
	//Pose 데이터로 특정 본의 결합 행렬을 계산함. 멤버 Bone 배열과 대응되는 Pose여야함 
	_matrix								Compute_CombinedFromPose(const vector<TransformData>& Pose, _int iBoneIndex) const;

protected:
	virtual void						Update_MeshBones();
	virtual HRESULT						Ready_MeshBones(_wstring& strMeshBoneFilePath);
	HRESULT								Ready_Materials(_wstring& strMaterialFilePath);
	HRESULT								Ready_BoneMatrixBuffer();

	// Animation_CS
	HRESULT								Ready_PackedMeshBoneData();

	HRESULT								Ready_CombinedBoneMatrixBuffer();
	void								Update_CombinedBoneMatrixBuffer(const vector<class CBone*>& Bones);
	
	void								Dispatch_ComputeMeshBones();
 
	HRESULT								Ready_GPUCombinedBoneRange(_uint iBoneCount);
	void								Free_GPUCombinedBoneRange();

	HRESULT								Ready_GPUAnimationBuffers();
	HRESULT								Ensure_GPUAnimationBuffers();

protected:
	class CGameInstance*				m_pInstance = { nullptr };
	vector<RenderUnit>					m_RenderUnits;

	MODEL								m_eType = { MODEL::END };

	_uint								m_iNumMeshes = {};
	vector<class CMesh*>				m_Meshes;

	_uint								m_iNumMaterials = {};
	vector<class CMaterial*>			m_Materials;

	_float4x4							m_PreTransformMatrix = {};
	_wstring							m_strMeshBoneFilePath = {};
	
	//Parts 모델때문에 본 업데이트 플래그를 버전으로 관리함
	_uint								m_iBoneVersion = {0};
	_uint								m_iLastBoneVersion = {0};


	_uint								m_iNumBones = {};
	vector<class CBone*>				m_Bones;

	//애니메이션 없는 기본 포즈 데이터 
	vector<TransformData>				m_BasePose;
	
	//메시당 BoneMatrix배열 
	//기존 Mesh에서 관리하던 BoneMatrix를 Model로 옮김
	vector<BoneMatrixSet>				m_BoneMatrixSet;


	//Animation 파트 추가 
	class CStructuredBuffer*			m_pBoneMatrixBuffer = { nullptr };
	vector<_uint>						m_MeshBoneOffsets;
	vector<_float4x4>					m_GPUUploadBoneMatrices;

	_uint								m_iGPUFinalBoneOffset = {};
	_uint								m_iGPUFinalBoneCount = {};
	_bool								m_bHasGPUFinalBoneRange = { false };

	vector<_uint>						m_PackedMeshBoneIndices;
	vector<_float4x4>					m_PackedMeshOffsetMatrices;

	_uint								m_iGPUMeshBoneInputOffset = {};
	_uint								m_iGPUMeshBoneInputCount = {};
	_bool								m_bHasGPUMeshBoneInputRange = { false };

	vector<_float4x4>					m_CombinedBoneMatrices;

	_uint								m_iGPUCombinedBoneOffset = {};
	_uint								m_iGPUCombinedBoneCount = {};
	_bool								m_bHasGPUCombinedBoneRange = { false };

	_bool								m_bUseComputeMeshBone = { true };
	_bool								m_bGPUAnimationBuffersReady = { false };

public:
	static CModel*						Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strModelName, _fmatrix PreTransformMatrix = XMMatrixIdentity());
	virtual CComponent*					Clone(void* pArg) override;
	virtual void						Free() override;
};

NS_END