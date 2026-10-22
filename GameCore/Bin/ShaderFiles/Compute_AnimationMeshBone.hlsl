StructuredBuffer<float4x4> g_CombinedBoneMatrices   : register(t0); 
StructuredBuffer<uint>     g_AllMeshBoneIndices     : register(t1);
StructuredBuffer<float4x4> g_AllMeshOffsetMatrices  : register(t2);

struct SkinningJob
{
    uint iTotalMeshBoneCount;
    uint iOutputBoneOffset;
    uint iInputBoneOffset;
    uint iCombinedBoneOffset;
};

StructuredBuffer<SkinningJob> g_SkinningJobs        : register(t3);

RWStructuredBuffer<float4x4>  g_OutBoneMatrices     : register(u0);     // 최종 스키닝 matrix 출력 버퍼

cbuffer BoneComputeInfo : register(b0)
{
 
    uint g_JobCount;
    uint g_MaxMeshBoneCount;
    
    uint g_Padding0;
    uint g_Padding1;
    
    uint g_Padding2;
    uint g_Padding3;
    uint g_Padding4;
    uint g_Padding5;
};

// 스레드 그룹에 64개 스레드
[numthreads(64, 1, 1)]
void CS_Main(uint3 vDispatchThreadID : SV_DispatchThreadID)
{
    uint iLocalBoneIndex = vDispatchThreadID.x;
    uint iJobIndex = vDispatchThreadID.y;
    
    if (iJobIndex >= g_JobCount)
        return;
    
    SkinningJob Job = g_SkinningJobs[iJobIndex];
    
    if (iLocalBoneIndex >= Job.iTotalMeshBoneCount)
        return;
    
    uint iInputBoneIndex = Job.iInputBoneOffset + iLocalBoneIndex;
    
    uint iSkeletonBoneIndex = g_AllMeshBoneIndices[iInputBoneIndex];
    uint iCombinedBoneIndex = Job.iCombinedBoneOffset + iSkeletonBoneIndex;
    
    float4x4 OffsetMatrix = g_AllMeshOffsetMatrices[iInputBoneIndex];
    float4x4 CombinedMatrix = g_CombinedBoneMatrices[iCombinedBoneIndex];
    
    uint iOutputBoneIndex = Job.iOutputBoneOffset + iLocalBoneIndex;
    
    g_OutBoneMatrices[iOutputBoneIndex] = transpose(mul(CombinedMatrix, OffsetMatrix));
}