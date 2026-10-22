#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class CComputeShader;
class CStructuredBuffer;
class CModel;

class ENGINE_DLL CGPUAnimationSystem final : public CBase
{
private:
	struct BoneComputeInfoData
	{
		_uint iJobCount = {};
		_uint iMaxMeshBoneCount = {};

		_uint iPadding0 = {};
		_uint iPadding1 = {};

		_uint iPadding2 = {};
		_uint iPadding3 = {};
		_uint iPadding4 = {};
		_uint iPadding5 = {};
	};

	struct AnimationSkinningJob
	{
		_uint iTotalMeshBoneCount = {};
		_uint iOutputBoneOffset = {};
		_uint iInputBoneOffset = {};
		_uint iCombinedBoneOffset = {};
	};

	struct BufferRange
	{
		_uint iOffset = {};
		_uint iCount = {};
	};

private:
	CGPUAnimationSystem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CGPUAnimationSystem() = default;

public:
	HRESULT							Initialize();

public:
	HRESULT							Allocate_FinalBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset);
	HRESULT							Free_FinalBoneMatrixRange(_uint iOffset, _uint iCount);

	HRESULT							Allocate_CombinedBoneMatrixRange(_uint iBoneCount, _uint* pOutOffset);
	HRESULT							Free_CombinedBoneMatrixRange(_uint iOffset, _uint iCount);

	ID3D11ShaderResourceView*		Get_FinalBoneMatrixSRV() const;
	_uint							Get_FinalBoneMatrixCapacity() const { return GLOBAL_FINAL_BONE_MATRIX_CAPACITY; }

	HRESULT							Update_GlobalCombinedBoneMatrixRange(_uint iOffset, const _float4x4* pCombinedMatrices, _uint iCount);

	HRESULT							Register_MeshBoneInputRange(const vector<_uint>& BoneIndices, const vector<_float4x4>& OffsetMatrices, _uint* pOutOffset, _uint* pOutCount);
	HRESULT							Free_MeshBoneInputRange(_uint iOffset, _uint iCount);


	HRESULT							Register_ModelSkinningJob(CModel* pModel);
	HRESULT							Flush_SkinningJobs();
	void							Clear_SkinningJobs();

	_uint							Get_SkinningJobCount() const;

private:
	HRESULT							Dispatch_BatchedSkinningJobs();

	HRESULT							Ready_AnimationComputeShader();
	HRESULT							Ready_BoneComputeConstantBuffer();
	HRESULT							Ready_GlobalFinalBoneMatrixBuffer();
	HRESULT							Ready_GlobalMeshBoneInputBuffers();
	HRESULT							Ready_SkinningJobBuffer();

	void							Update_BoneComputeConstantBuffer(const BoneComputeInfoData& Info);
	void							Merge_FreeFinalBoneMatrixRanges();

	HRESULT							Allocate_MeshBoneInputRange(_uint iCount, _uint* pOutOffset);
	void							Merge_FreeMeshBoneInputRanges();

	HRESULT							Ready_GlobalCombinedBoneMatrixBuffer();
	void							Merge_FreeCombinedBoneMatrixRanges();

	HRESULT							Upload_SkinningJobs();

	ID3D11UnorderedAccessView*		Get_FinalBoneMatrixUAV() const;
	ID3D11ShaderResourceView*		Get_GlobalCombinedBoneMatrixSRV() const;
	ID3D11ShaderResourceView*		Get_GlobalMeshBoneIndexSRV() const;
	ID3D11ShaderResourceView*		Get_GlobalMeshOffsetMatrixSRV() const;
	ID3D11ShaderResourceView*		Get_SkinningJobSRV() const;

private:
	static constexpr _uint			GLOBAL_FINAL_BONE_MATRIX_CAPACITY = 131072;
	static constexpr _uint			GLOBAL_COMBINED_BONE_MATRIX_CAPACITY = 131072;
	static constexpr _uint			GLOBAL_MESH_BONE_INPUT_CAPACITY = 262144;
	static constexpr _uint			GLOBAL_SKINNING_JOB_CAPACITY = 8192;

	ID3D11Device*					m_pDevice = { nullptr };
	ID3D11DeviceContext*			m_pContext = { nullptr };

	CComputeShader*					m_pAnimationMeshBoneCS = { nullptr };
	ID3D11Buffer*					m_pBoneComputeCB = { nullptr };

	CStructuredBuffer*				m_pGlobalFinalBoneMatrixBuffer = { nullptr };
	CStructuredBuffer*				m_pGlobalMeshBoneIndexBuffer = { nullptr };
	CStructuredBuffer*				m_pGlobalMeshOffsetMatrixBuffer = { nullptr };
	CStructuredBuffer*				m_pGlobalCombinedMatrixBuffer = { nullptr };
	CStructuredBuffer*				m_pSkinningJobBuffer = { nullptr };

	_uint							m_iNextFinalBoneMatrixOffset = {};
	vector<BufferRange>				m_FreeFinalBoneMatrixRanges;

	_uint							m_iNextMeshBoneInputOffset = {};
	vector<BufferRange>				m_FreeMeshBoneInputRanges;

	_uint							m_iNextCombinedBoneMatrixOffset = {};
	vector<BufferRange>				m_FreeCombinedBoneMatrixRanges;

	vector<AnimationSkinningJob>	m_SkinningJobs;
	_uint							m_iMaxSkinningJobMeshBoneCount = {};

public:
	static CGPUAnimationSystem* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END