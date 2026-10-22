#include "PartsModel.h"
#include "GameInstance.h"
#include "Engine_Utility.h"
#include "MasterModel.h"
#include "StructuredBuffer.h"

CPartsModel::CPartsModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CModel{pDevice, pContext}
{

}

CPartsModel::CPartsModel(const CPartsModel& Prototype)
	: CModel{ Prototype }
{
}


HRESULT	CPartsModel::Initialize_Prototype(_wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix)
{
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);

    _wstring strResourceDir = m_pGameInstance->Get_ResourcePath() + L"/Models/" + strFolderName + L"/";
    _wstring strMeshBonePath = strResourceDir + strModelName + GlobalFileRoute::g_strBoneMeshFileTag;
    m_strMeshBoneFilePath = strMeshBonePath;

    if (FAILED(Ready_MeshBones(strMeshBonePath)))
    {
        MSG_BOX("Read Mesh , Bone Failed");
        return E_FAIL;
    }

    m_iNumMaterials = m_iNumMeshes;
    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        _wstring strMaterialName = m_Meshes[i]->Get_MaterialName();
        _wstring strMaterialPath = strResourceDir + GlobalFileRoute::g_strMaterialFolder + strMaterialName + GlobalFileRoute::g_strMaterialFileTag;
        if (FAILED(Ready_Materials(strMaterialPath)))
        {
            MSG_BOX("Read Material Failed");
            return E_FAIL; 
        }
    }

    m_RenderUnits.reserve(m_iNumMeshes);
    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        m_RenderUnits.push_back({ m_Meshes[i], Get_Material(i) });
    }

    // 프로토타입에서는 Mastermodel이 없으므로 Combined Buffer를 만들지 않는다.
    // Initialize에서 호출

    return S_OK;
}

HRESULT	CPartsModel::Initialize(void* pArg)
{
    if (pArg == nullptr)
    {
        MSG_BOX("CPartsModel Clone : Arg is nullptr. Push MasterModel Address ");
        return E_FAIL;
    }
    PARTMODEL_DESC* pDesc = reinterpret_cast<PARTMODEL_DESC*>(pArg);

    m_pMasterModel = pDesc->m_pMasterModel;
    Safe_AddRef(m_pMasterModel);

    if (FAILED(Ready_BoneMatrixBuffer()))
        return E_FAIL;

    if (FAILED(Ready_PackedMeshBoneData()))
        return E_FAIL;

    // 혹시 복사 생성자 쪽에서 잘 못 만들어진 Combined Buffer 가 있으면 제거하고  MasterModel Bone 개수 기준으로 다시 만들기 위해서
    m_CombinedBoneMatrices.clear();

    if (FAILED(Ready_MasterCombinedBoneMatrixBuffer()))
        return E_FAIL;

    return S_OK;
}

void CPartsModel::Update_MeshBones()
{
    _uint iMasterVersion = m_pMasterModel->Get_BoneVersion();

    if (m_iLastBoneVersion == iMasterVersion) return;

    const auto& masterBones = m_pMasterModel->Get_Bones();

    Update_CombinedBoneMatrixBuffer(masterBones);
    
    if (m_bUseComputeMeshBone)
    {
        Dispatch_ComputeMeshBones();

        m_iLastBoneVersion = iMasterVersion;
        return;
    }

    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        if (m_Meshes[i] == nullptr)
            continue;

        _float4x4* pCPUOut = m_BoneMatrixSet[i].m_TransposedBoneMatrices.data();

        m_Meshes[i]->Update_BoneMatrices(masterBones, pCPUOut);

        if (m_pBoneMatrixBuffer != nullptr && i < m_MeshBoneOffsets.size())
        {
            const _uint iOffset = m_MeshBoneOffsets[i];
            const _uint iBoneCount = m_Meshes[i]->Get_BoneCount();

            if (iOffset + iBoneCount <= m_GPUUploadBoneMatrices.size())
            {
                memcpy(m_GPUUploadBoneMatrices.data() + iOffset, pCPUOut, sizeof(_float4x4) * iBoneCount);
            }
        }
    }
    if (m_pBoneMatrixBuffer != nullptr && !m_GPUUploadBoneMatrices.empty())
    {
        m_pBoneMatrixBuffer->Update_Buffer(m_GPUUploadBoneMatrices.data(), static_cast<_uint>(m_GPUUploadBoneMatrices.size()));
    }

    m_iLastBoneVersion = iMasterVersion;
}

//Parts는 전체 본이 없고 mesh 데이터만 소유함. 전체 본은 Master의 Bone을 참조하여 Mesh별 본행렬을 계산함
HRESULT CPartsModel::Ready_MeshBones(_wstring& strMeshBoneFilePath)
{
    ifstream fin(strMeshBoneFilePath, ios::binary);
    if (!fin.is_open())
    {
        MSG_BOX("Read Mesh Bone Failed");
        return E_FAIL;
    }

    uint32_t iBoneCount = 0;
    uint32_t meshCount = 0;

    fin.read(reinterpret_cast<char*>(&iBoneCount), sizeof(uint32_t));
    fin.read(reinterpret_cast<char*>(&meshCount), sizeof(uint32_t));

    m_iNumMeshes = meshCount;
    m_iNumBones = iBoneCount;

    m_BoneMatrixSet.resize(m_iNumMeshes);

    //Read Bones
    for (uint32_t i = 0; i < m_iNumBones; ++i)
    {
        BoneData boneData{};
        fin.read(reinterpret_cast<char*>(&boneData), sizeof(BoneData));
        //만약 본 데이터가 들어있으면 읽어서 버림 
    }
    m_iNumBones = 0;

    //Read Mesh 
    for (size_t i = 0; i < m_iNumMeshes; i++)
    {
        MeshData header{};
        fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));

        MeshPassData passData;
        strcpy_s(passData.name, header.meshName);
        strcpy_s(passData.materialName, header.materialName);
        passData.hasAnimation = header.HasAnimation;
        passData.vertexCount = header.vertexCount;
        passData.indexCount = header.indexCount;
        passData.vertexStride = header.vertexStride;
        passData.boneCount = header.boneCount;

        //Read Vertices
        if (passData.hasAnimation)
        {
            if (m_eType == MODEL::END) m_eType = MODEL::ANIM;
            //Animation 
            passData.AnimVertices.resize(header.vertexCount);
            fin.read
            (
                reinterpret_cast<char*>(passData.AnimVertices.data()),
                sizeof(VTXANIMMESH) * header.vertexCount
            );
        }
        else
        {
            if (m_eType == MODEL::END) m_eType = MODEL::NONANIM;
            //No Animation
            passData.NoAnimVertices.resize(header.vertexCount);
            fin.read
            (
                reinterpret_cast<char*>(passData.NoAnimVertices.data()),
                sizeof(VTXMESH) * header.vertexCount
            );
        }

        //Read Indices
        passData.Indices.resize(header.indexCount);
        fin.read
        (
            reinterpret_cast<char*>(passData.Indices.data()),
            sizeof(uint32_t) * header.indexCount
        );

        //Read Bone Indices
        passData.BoneIndices.resize(header.boneCount);
        fin.read
        (
            reinterpret_cast<char*>(passData.BoneIndices.data()),
            sizeof(uint32_t) * header.boneCount
        );

        passData.OffsetMatrices.resize(header.boneCount);
        fin.read
        (
            reinterpret_cast<char*>(passData.OffsetMatrices.data()),
            sizeof(_float4x4) * header.boneCount
        );

        m_BoneMatrixSet[i].m_TransposedBoneMatrices.resize(passData.boneCount);

        //이미 존재해도 Bin파일은 다 읽고 넘어가야하니까 읽은 뒤 중복 체크 
        _wstring MeshName = CharArrayToWstring(header.meshName);
        CVIBuffer* pMesh = nullptr;

        m_pInstance->Get_VIBuffer(MeshName, &pMesh);

        //이미 ResourceMgr에 존재한다면 
        if (pMesh != nullptr)
        {
            m_Meshes.push_back(static_cast<CMesh*>(pMesh));
            continue;
        }

        pMesh = CMesh::Create(m_pDevice, m_pContext, passData);
        if (pMesh == nullptr)
        {
            MSG_BOX("Mesh Create Fail!");
            return E_FAIL;
        }
        pMesh->Set_Name(MeshName);

        if (FAILED(m_pInstance->Register_VIBuffer(pMesh)))
        {
			Safe_Release(pMesh);
			//다른 스레드가 이미 등록했을 수 있으므로 다시 가져오기
            m_pInstance->Get_VIBuffer(MeshName, &pMesh);
            if(pMesh == nullptr)
                return E_FAIL;  

            m_Meshes.push_back(static_cast<CMesh*>(pMesh));
            continue;
        }

        m_Meshes.push_back(static_cast<CMesh*>(pMesh));


        //ResourceMgr외로 Model 에 보관하므로 AddRef
        Safe_AddRef(pMesh);
    }
    return S_OK;
}

HRESULT CPartsModel::Ready_MasterCombinedBoneMatrixBuffer()
{
    if (m_pMasterModel == nullptr)
        return E_FAIL;

    const _uint iMasterBoneCount = m_pMasterModel->Get_NumBones();

    if (iMasterBoneCount == 0)
        return S_OK;

    m_CombinedBoneMatrices.resize(iMasterBoneCount);

    m_iGPUCombinedBoneOffset = 0;
    m_iGPUCombinedBoneCount = 0;
    m_bHasGPUCombinedBoneRange = false;

    return S_OK;
}

CPartsModel* CPartsModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix)
{
    CPartsModel* pInstance = new CPartsModel(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(strFolderName, strModelName,PreTransformMatrix)))
    {
        MSG_BOX("Failed to Created : CPartsModel");  
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CPartsModel::Clone(void* pArg)
{
    CPartsModel* pInstance = new CPartsModel(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CPartsModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPartsModel::Free()
{
	__super::Free();
    Safe_Release(m_pMasterModel);
}