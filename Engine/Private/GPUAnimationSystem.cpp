#include "GPUAnimationSystem.h"
#include "ComputeShader.h"
#include "StructuredBuffer.h"
#include "Model.h"
#include "Engine_Utility.h"

CGPUAnimationSystem::CGPUAnimationSystem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CGPUAnimationSystem::Initialize()
{
    if (FAILED(Ready_AnimationComputeShader()))
        return E_FAIL;

    if (FAILED(Ready_BoneComputeConstantBuffer()))
        return E_FAIL;

    if (FAILED(Ready_GlobalFinalBoneMatrixBuffer()))
        return E_FAIL;

    if (FAILED(Ready_GlobalMeshBoneInputBuffers()))
        return E_FAIL;

    if (FAILED(Ready_GlobalCombinedBoneMatrixBuffer()))
        return E_FAIL;

    if (FAILED(Ready_SkinningJobBuffer()))
        return E_FAIL;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Allocate_FinalBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset)
{
    if (pOutOffset == nullptr)
        return E_FAIL;

    *pOutOffset = 0;

    if (iBoneMatrixCount == 0)
        return S_OK;

    for (auto iter = m_FreeFinalBoneMatrixRanges.begin(); iter != m_FreeFinalBoneMatrixRanges.end(); ++iter)
    {
        if (iter->iCount < iBoneMatrixCount)
            continue;

        *pOutOffset = iter->iOffset;

        iter->iOffset += iBoneMatrixCount;
        iter->iCount -= iBoneMatrixCount;

        if (iter->iCount == 0)
            m_FreeFinalBoneMatrixRanges.erase(iter);

        return S_OK;
    }

    if (m_iNextFinalBoneMatrixOffset + iBoneMatrixCount > GLOBAL_FINAL_BONE_MATRIX_CAPACITY)
    {
        MSG_BOX("Global Final Bone Matrix Buffer Capacity Exceeded");
        return E_FAIL;
    }

    *pOutOffset = m_iNextFinalBoneMatrixOffset;
     m_iNextFinalBoneMatrixOffset += iBoneMatrixCount;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Free_FinalBoneMatrixRange(_uint iOffset, _uint iCount)
{
    if (iCount == 0)
        return S_OK;

    if (iOffset + iCount > GLOBAL_FINAL_BONE_MATRIX_CAPACITY)
        return E_FAIL;

    BufferRange Range = {};
    Range.iOffset = iOffset;
    Range.iCount = iCount;

    m_FreeFinalBoneMatrixRanges.push_back(Range);
    Merge_FreeFinalBoneMatrixRanges();

    return S_OK;
}

HRESULT CGPUAnimationSystem::Allocate_CombinedBoneMatrixRange(_uint iBoneCount, _uint* pOutOffset)
{
    if (pOutOffset == nullptr)
        return E_FAIL;

    *pOutOffset = 0;

    if (iBoneCount == 0)
        return S_OK;

    for (auto iter = m_FreeCombinedBoneMatrixRanges.begin(); iter != m_FreeCombinedBoneMatrixRanges.end(); ++iter)
    {
        if (iter->iCount < iBoneCount)
            continue;

        *pOutOffset = iter->iOffset;

        iter->iOffset += iBoneCount;
        iter->iCount -= iBoneCount;

        if (iter->iCount == 0)
            m_FreeCombinedBoneMatrixRanges.erase(iter);

        return S_OK;
    }

    if (m_iNextCombinedBoneMatrixOffset + iBoneCount > GLOBAL_COMBINED_BONE_MATRIX_CAPACITY)
    {
        MSG_BOX("Global Combined Bone Matrix Buffer Capacity Exceeded");
        return E_FAIL;
    }

    *pOutOffset = m_iNextCombinedBoneMatrixOffset;
    m_iNextCombinedBoneMatrixOffset += iBoneCount;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Free_CombinedBoneMatrixRange(_uint iOffset, _uint iCount)
{
    if (iCount == 0)
        return S_OK;

    if (iOffset + iCount > GLOBAL_COMBINED_BONE_MATRIX_CAPACITY)
        return E_FAIL;

    BufferRange Range = {};
    Range.iOffset = iOffset;
    Range.iCount = iCount;

    m_FreeCombinedBoneMatrixRanges.push_back(Range);
    Merge_FreeCombinedBoneMatrixRanges();

    return S_OK;
}

ID3D11ShaderResourceView* CGPUAnimationSystem::Get_FinalBoneMatrixSRV() const
{
    if (m_pGlobalFinalBoneMatrixBuffer == nullptr)
        return nullptr;

    return m_pGlobalFinalBoneMatrixBuffer->Get_SRV();
}

HRESULT CGPUAnimationSystem::Update_GlobalCombinedBoneMatrixRange(_uint iOffset, const _float4x4* pCombinedMatrices, _uint iCount)
{
    if (pCombinedMatrices == nullptr || iCount == 0)
        return S_OK;

    if (m_pGlobalCombinedMatrixBuffer == nullptr)
        return E_FAIL;

    if (iOffset + iCount > GLOBAL_COMBINED_BONE_MATRIX_CAPACITY)
        return E_FAIL;

    m_pGlobalCombinedMatrixBuffer->Update_Buffer_Region(pCombinedMatrices, iOffset, iCount);

    return S_OK;
}

HRESULT CGPUAnimationSystem::Register_MeshBoneInputRange(const vector<_uint>& BoneIndices, const vector<_float4x4>& OffsetMatrices, _uint* pOutOffset, _uint* pOutCount)
{
    if (pOutOffset == nullptr || pOutCount == nullptr)
        return E_FAIL;

    *pOutOffset = 0;
    *pOutCount = 0;

    if (BoneIndices.empty() && OffsetMatrices.empty())
        return S_OK;

    if (BoneIndices.empty() || OffsetMatrices.empty())
        return E_FAIL;

    if (BoneIndices.size() != OffsetMatrices.size())
        return E_FAIL;

    const _uint iCount = static_cast<_uint>(BoneIndices.size());

    _uint iOffset = 0;
    if (FAILED(Allocate_MeshBoneInputRange(iCount, &iOffset)))
        return E_FAIL;

    if (m_pGlobalMeshBoneIndexBuffer == nullptr || m_pGlobalMeshOffsetMatrixBuffer == nullptr)
        return E_FAIL;

    m_pGlobalMeshBoneIndexBuffer->Update_Buffer_Region(BoneIndices.data(), iOffset, iCount);
    m_pGlobalMeshOffsetMatrixBuffer->Update_Buffer_Region(OffsetMatrices.data(), iOffset, iCount);

    *pOutOffset = iOffset;
    *pOutCount = iCount;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Free_MeshBoneInputRange(_uint iOffset, _uint iCount)
{
    if (iCount == 0)
        return S_OK;

    if (iOffset + iCount > GLOBAL_MESH_BONE_INPUT_CAPACITY)
        return E_FAIL;

    BufferRange Range = {};
    Range.iOffset = iOffset;
    Range.iCount = iCount;

    m_FreeMeshBoneInputRanges.push_back(Range);
    Merge_FreeMeshBoneInputRanges();

    return S_OK;
}

HRESULT CGPUAnimationSystem::Register_ModelSkinningJob(CModel* pModel)
{
    if (pModel == nullptr)
        return E_FAIL;

    const _uint iTotalMeshBoneCount = pModel->Get_TotalPackedMeshBoneCount();

    if (iTotalMeshBoneCount == 0)
        return S_OK;

    if (!pModel->Has_GPUFinalBoneRange())
        return E_FAIL;

    if (!pModel->Has_GPUMeshBoneInputRange())
        return E_FAIL;

    if (!pModel->Has_GPUCombinedBoneRange())
        return E_FAIL;

    AnimationSkinningJob Job = {};
    Job.iTotalMeshBoneCount = iTotalMeshBoneCount;
    Job.iOutputBoneOffset = pModel->Get_GPUFinalBoneOffset();
    Job.iInputBoneOffset = pModel->Get_GPUMeshBoneInputOffset();
    Job.iCombinedBoneOffset = pModel->Get_GPUCombinedBoneOffset();

    m_SkinningJobs.push_back(Job);

    if (m_iMaxSkinningJobMeshBoneCount < iTotalMeshBoneCount)
        m_iMaxSkinningJobMeshBoneCount = iTotalMeshBoneCount;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Flush_SkinningJobs()
{
    if (m_SkinningJobs.empty())
        return S_OK;

    if (FAILED(Upload_SkinningJobs()))
    {
        Clear_SkinningJobs();
        return E_FAIL;
    }

    HRESULT hr = Dispatch_BatchedSkinningJobs();

    Clear_SkinningJobs();

    return hr;
}

void CGPUAnimationSystem::Clear_SkinningJobs()
{
    m_SkinningJobs.clear();
    m_iMaxSkinningJobMeshBoneCount = 0;
}

_uint CGPUAnimationSystem::Get_SkinningJobCount() const
{
    return static_cast<_uint>(m_SkinningJobs.size());
}

HRESULT CGPUAnimationSystem::Dispatch_BatchedSkinningJobs()
{
    if (m_pAnimationMeshBoneCS == nullptr || m_pBoneComputeCB == nullptr)
        return E_FAIL;

    const _uint iJobCount = static_cast<_uint>(m_SkinningJobs.size());

    if (iJobCount == 0)
        return S_OK;

    if (m_iMaxSkinningJobMeshBoneCount == 0)
        return S_OK;

    ID3D11ShaderResourceView* pCombinedBoneSRV = Get_GlobalCombinedBoneMatrixSRV();
    ID3D11ShaderResourceView* pPackedBoneIndexSRV = Get_GlobalMeshBoneIndexSRV();
    ID3D11ShaderResourceView* pPackedOffsetMatrixSRV = Get_GlobalMeshOffsetMatrixSRV();
    ID3D11ShaderResourceView* pSkinningJobSRV = Get_SkinningJobSRV();

    ID3D11UnorderedAccessView* pFinalBoneUAV = Get_FinalBoneMatrixUAV();

    if (pCombinedBoneSRV == nullptr || pPackedBoneIndexSRV == nullptr || pPackedOffsetMatrixSRV == nullptr || pSkinningJobSRV == nullptr || pFinalBoneUAV == nullptr)
        return  E_FAIL;

    ID3D11ShaderResourceView* pNullSRV = nullptr;
    m_pContext->VSSetShaderResources(SRV_BONE_MATRIX_SLOT, 1, &pNullSRV);

    BoneComputeInfoData Info = {};
    Info.iJobCount = iJobCount;
    Info.iMaxMeshBoneCount = m_iMaxSkinningJobMeshBoneCount;

    Update_BoneComputeConstantBuffer(Info);

    m_pAnimationMeshBoneCS->Bind_ConstantBuffer(0, m_pBoneComputeCB);

    m_pAnimationMeshBoneCS->Bind_SRV(0, pCombinedBoneSRV);
    m_pAnimationMeshBoneCS->Bind_SRV(1, pPackedBoneIndexSRV);
    m_pAnimationMeshBoneCS->Bind_SRV(2, pPackedOffsetMatrixSRV);
    m_pAnimationMeshBoneCS->Bind_SRV(3, pSkinningJobSRV);

    m_pAnimationMeshBoneCS->Bind_UAV(0, pFinalBoneUAV);

    const _uint iGroupCountX = (m_iMaxSkinningJobMeshBoneCount + 63) / 64;
    const _uint iGroupCountY = iJobCount;

    m_pAnimationMeshBoneCS->Dispatch(iGroupCountX, iGroupCountY, 1);

    m_pAnimationMeshBoneCS->Unbind_UAV(0, 1);
    m_pAnimationMeshBoneCS->Unbind_SRV(0, 4);


    return S_OK;
}

HRESULT CGPUAnimationSystem::Ready_AnimationComputeShader()
{
    {
        CComputeShader::COMPUTE_SHADER_DESC Desc{};
        Desc.strShaderName = L"CS_AnimationMeshBone";
        Desc.strCSFilePath = _wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Compute_AnimationMeshBone.hlsl";
        Desc.strCSFunName = "CS_Main";

        m_pAnimationMeshBoneCS = CComputeShader::Create(m_pDevice, m_pContext, Desc);

        if (m_pAnimationMeshBoneCS == nullptr)
            return E_FAIL;
    }
 
    return S_OK;
}

HRESULT CGPUAnimationSystem::Ready_BoneComputeConstantBuffer()
{
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.ByteWidth = sizeof(BoneComputeInfoData);
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = 0;
    BufferDesc.StructureByteStride = 0;

    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pBoneComputeCB)))
        return E_FAIL;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Ready_GlobalFinalBoneMatrixBuffer()
{
    CStructuredBuffer::SB_DESC Desc = {};
    Desc.iElementStride = sizeof(_float4x4);
    Desc.iElementCount = GLOBAL_FINAL_BONE_MATRIX_CAPACITY;
    Desc.pInitialData = nullptr;
    Desc.bAppend = false;

    m_pGlobalFinalBoneMatrixBuffer = CStructuredBuffer::Create(m_pDevice, m_pContext, Desc);
    if (m_pGlobalFinalBoneMatrixBuffer == nullptr)
        return E_FAIL;

    return S_OK;
}

HRESULT CGPUAnimationSystem::Ready_GlobalMeshBoneInputBuffers()
{
    {
        CStructuredBuffer::SB_DESC Desc = {};
        Desc.iElementStride = sizeof(_uint);
        Desc.iElementCount = GLOBAL_MESH_BONE_INPUT_CAPACITY;
        Desc.pInitialData = nullptr;
        Desc.bAppend = false;

        m_pGlobalMeshBoneIndexBuffer = CStructuredBuffer::Create(m_pDevice, m_pContext, Desc);
        if (m_pGlobalMeshBoneIndexBuffer == nullptr)
            return E_FAIL;
    }
    
    {
        CStructuredBuffer::SB_DESC Desc = {};
        Desc.iElementStride = sizeof(_float4x4);
        Desc.iElementCount = GLOBAL_MESH_BONE_INPUT_CAPACITY;
        Desc.pInitialData = nullptr;
        Desc.bAppend = false;

        m_pGlobalMeshOffsetMatrixBuffer = CStructuredBuffer::Create(m_pDevice, m_pContext, Desc);
        if (m_pGlobalMeshOffsetMatrixBuffer == nullptr)
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CGPUAnimationSystem::Ready_SkinningJobBuffer()
{
    CStructuredBuffer::SB_DESC Desc = {};
    Desc.iElementStride = sizeof(AnimationSkinningJob);
    Desc.iElementCount = GLOBAL_SKINNING_JOB_CAPACITY;
    Desc.pInitialData = nullptr;
    Desc.bAppend = false;

    m_pSkinningJobBuffer = CStructuredBuffer::Create(m_pDevice, m_pContext, Desc);
    if (m_pSkinningJobBuffer == nullptr)
        return E_FAIL;

    return S_OK;
}

void CGPUAnimationSystem::Update_BoneComputeConstantBuffer(const BoneComputeInfoData& Info)
{
    if (m_pBoneComputeCB == nullptr)
        return;

    D3D11_MAPPED_SUBRESOURCE Mapped = {};
    
    if (FAILED(m_pContext->Map(m_pBoneComputeCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
        return;

    memcpy(Mapped.pData, &Info, sizeof(BoneComputeInfoData));

    m_pContext->Unmap(m_pBoneComputeCB, 0);
}

void CGPUAnimationSystem::Merge_FreeFinalBoneMatrixRanges()
{
    if (m_FreeFinalBoneMatrixRanges.empty())
        return;

    sort(m_FreeFinalBoneMatrixRanges.begin(), m_FreeFinalBoneMatrixRanges.end(),
        [](const BufferRange& Left, const BufferRange& Right)
        {
            return Left.iOffset < Right.iOffset;
        });

    vector<BufferRange> MergedRanges;
    MergedRanges.reserve(m_FreeFinalBoneMatrixRanges.size());

    MergedRanges.push_back(m_FreeFinalBoneMatrixRanges.front());

    for (_uint i = 1; i < static_cast<_uint>(m_FreeFinalBoneMatrixRanges.size()); i++)
    {
        BufferRange& Back = MergedRanges.back();
        const BufferRange& Current = m_FreeFinalBoneMatrixRanges[i];

        const _uint iBackEnd = Back.iOffset + Back.iCount;

        if (iBackEnd >= Current.iOffset)
        {
            const _uint iCurrentEnd = Current.iOffset + Current.iCount;
            Back.iCount = max(iBackEnd, iCurrentEnd) - Back.iOffset;
        }
        else
        {
            MergedRanges.push_back(Current);
        }
    }

    m_FreeFinalBoneMatrixRanges = MergedRanges;
}

HRESULT CGPUAnimationSystem::Allocate_MeshBoneInputRange(_uint iCount, _uint* pOutOffset)
{
    if (pOutOffset == nullptr)
        return E_FAIL;

    *pOutOffset = 0;

    if (iCount == 0)
        return S_OK;

    for (auto iter = m_FreeMeshBoneInputRanges.begin(); iter != m_FreeMeshBoneInputRanges.end(); ++iter)
    {
        if (iter->iCount < iCount)
            continue;

        *pOutOffset = iter->iOffset;

        iter->iOffset += iCount;
        iter->iCount -= iCount;

        if (iter->iCount == 0)
            m_FreeMeshBoneInputRanges.erase(iter);

        return S_OK;
    }

    if (m_iNextMeshBoneInputOffset + iCount > GLOBAL_MESH_BONE_INPUT_CAPACITY)
    {
        MSG_BOX("Global Mesh Bone Input Buffer Capacity Exceeded");
        return E_FAIL;
    }

    *pOutOffset = m_iNextMeshBoneInputOffset;
    m_iNextMeshBoneInputOffset += iCount;

    return S_OK;
}

void CGPUAnimationSystem::Merge_FreeMeshBoneInputRanges()
{
    if (m_FreeMeshBoneInputRanges.empty())
        return;

    sort(m_FreeMeshBoneInputRanges.begin(), m_FreeMeshBoneInputRanges.end(),
        [](const BufferRange& Left, const BufferRange& Right)
        {
            return Left.iOffset < Right.iOffset;
        });

    vector<BufferRange> MergedRanges;
    MergedRanges.reserve(m_FreeMeshBoneInputRanges.size());

    MergedRanges.push_back(m_FreeMeshBoneInputRanges.front());

    for (_uint i = 1; i < static_cast<_uint>(m_FreeMeshBoneInputRanges.size()); i++)
    {
        BufferRange& Back = MergedRanges.back();
        const BufferRange& Current = m_FreeMeshBoneInputRanges[i];

        const _uint iBackEnd = Back.iOffset + Back.iCount;

        if (iBackEnd >= Current.iOffset)
        {
            const _uint iCurrentEnd = Current.iOffset + Current.iCount;
            Back.iCount = max(iBackEnd, iCurrentEnd) - Back.iOffset;
        }
        else
        {
            MergedRanges.push_back(Current);
        }
    }

    m_FreeMeshBoneInputRanges = MergedRanges;
}

HRESULT CGPUAnimationSystem::Ready_GlobalCombinedBoneMatrixBuffer()
{
    CStructuredBuffer::SB_DESC Desc = {};
    Desc.iElementStride = sizeof(_float4x4);
    Desc.iElementCount = GLOBAL_COMBINED_BONE_MATRIX_CAPACITY;
    Desc.pInitialData = nullptr;
    Desc.bAppend = false;

    m_pGlobalCombinedMatrixBuffer = CStructuredBuffer::Create(m_pDevice, m_pContext, Desc);

    if (m_pGlobalCombinedMatrixBuffer == nullptr)
        return E_FAIL;

    return S_OK;
}

void CGPUAnimationSystem::Merge_FreeCombinedBoneMatrixRanges()
{
    if (m_FreeCombinedBoneMatrixRanges.empty())
        return;

    sort(m_FreeCombinedBoneMatrixRanges.begin(), m_FreeCombinedBoneMatrixRanges.end(),
        [](const BufferRange& Left, const BufferRange& Right)
        {
            return Left.iOffset < Right.iOffset;
        });

    vector<BufferRange> MergedRanges;
    MergedRanges.reserve(m_FreeCombinedBoneMatrixRanges.size());

    MergedRanges.push_back(m_FreeCombinedBoneMatrixRanges.front());

    for (_uint i = 1; i < static_cast<_uint>(m_FreeCombinedBoneMatrixRanges.size()); i++)
    {
        BufferRange& Back = MergedRanges.back();
        const BufferRange& Current = m_FreeCombinedBoneMatrixRanges[i];

        const _uint iBackEnd = Back.iOffset + Back.iCount;

        if (iBackEnd >= Current.iOffset)
        {
            const _uint iCurrentEnd = Current.iOffset + Current.iCount;
            Back.iCount = max(iBackEnd, iCurrentEnd) - Back.iOffset;
        }
        else
        {
            MergedRanges.push_back(Current);
        }
    }

    m_FreeCombinedBoneMatrixRanges = MergedRanges;
}

HRESULT CGPUAnimationSystem::Upload_SkinningJobs()
{
    if (m_SkinningJobs.empty())
        return S_OK;

    if (m_pSkinningJobBuffer == nullptr)
        return E_FAIL;

    if (m_SkinningJobs.size() > GLOBAL_SKINNING_JOB_CAPACITY)
    {
        MSG_BOX("Skinning Job Buffer Capacity Exceeded");
        return E_FAIL;
    }

    m_pSkinningJobBuffer->Update_Buffer_Region(m_SkinningJobs.data(), 0, static_cast<_uint>(m_SkinningJobs.size()));

    return S_OK;
}

ID3D11UnorderedAccessView* CGPUAnimationSystem::Get_FinalBoneMatrixUAV() const
{
    if (m_pGlobalFinalBoneMatrixBuffer == nullptr)
        return nullptr;

    return m_pGlobalFinalBoneMatrixBuffer->Get_UAV();
}

ID3D11ShaderResourceView* CGPUAnimationSystem::Get_GlobalCombinedBoneMatrixSRV() const
{
    if (m_pGlobalCombinedMatrixBuffer == nullptr)
        return nullptr;

    return m_pGlobalCombinedMatrixBuffer->Get_SRV();
}

ID3D11ShaderResourceView* CGPUAnimationSystem::Get_GlobalMeshBoneIndexSRV() const
{
    if (m_pGlobalMeshBoneIndexBuffer == nullptr)
        return nullptr;

    return m_pGlobalMeshBoneIndexBuffer->Get_SRV();
}

ID3D11ShaderResourceView* CGPUAnimationSystem::Get_GlobalMeshOffsetMatrixSRV() const
{
    if (m_pGlobalMeshOffsetMatrixBuffer == nullptr)
        return nullptr;

    return m_pGlobalMeshOffsetMatrixBuffer->Get_SRV();
}

ID3D11ShaderResourceView* CGPUAnimationSystem::Get_SkinningJobSRV() const
{
    if (m_pSkinningJobBuffer == nullptr)
        return nullptr;

    return m_pSkinningJobBuffer->Get_SRV();
}

CGPUAnimationSystem* CGPUAnimationSystem::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CGPUAnimationSystem* pInstance = new CGPUAnimationSystem(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Faild to Created : CGPUAnimationSystem");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CGPUAnimationSystem::Free()
{
    __super::Free();

    m_SkinningJobs.clear();
    m_iMaxSkinningJobMeshBoneCount = 0;

    Safe_Release(m_pSkinningJobBuffer);

    Safe_Release(m_pGlobalCombinedMatrixBuffer);
    
    Safe_Release(m_pGlobalMeshOffsetMatrixBuffer);
    Safe_Release(m_pGlobalMeshBoneIndexBuffer);

    Safe_Release(m_pGlobalFinalBoneMatrixBuffer);
   
    
    Safe_Release(m_pBoneComputeCB);
    Safe_Release(m_pAnimationMeshBoneCS);

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
