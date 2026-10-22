#include "MasterModel.h"
#include "GameInstance.h"
#include "Bone.h"
#include "Animation.h"


CMasterModel::CMasterModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CModel{ pDevice, pContext }
{
}	

CMasterModel::CMasterModel(const CMasterModel& Prototype)
	: CModel{ Prototype }
{
}	


HRESULT	CMasterModel::Initialize_Prototype(_wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix)
{
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);

    _wstring strResourceDir = m_pGameInstance->Get_ResourcePath() + L"/Models/" + strFolderName + L"/";
    _wstring strMeshBonePath = strResourceDir + strModelName + GlobalFileRoute::g_strBoneMeshFileTag;
    //_wstring strAnimationPath = strResourceDir + strModelName + GlobalFileRoute::g_strAnimationFileTag;
    //_wstring strAnimationCSVPath = strResourceDir + strModelName + GlobalFileRoute::g_strAnimationCSVFileTag;

    if (FAILED(Ready_MeshBones(strMeshBonePath)))
    {
        MSG_BOX("Read Mesh , Bone Failed");
        return E_FAIL;
    }

    for (auto& pBones : m_Bones)
    {
        const _float4x4* transform = pBones->Get_TransformationMatrixPtr();

        _matrix mat = XMLoadFloat4x4(transform);

        _vector vScale, vRot, vTrans;
        XMMatrixDecompose(&vScale, &vRot, &vTrans, mat);

        _float4 vScaleF, vRotF, vTransF;

        XMStoreFloat4(&vScaleF, vScale);
        XMStoreFloat4(&vRotF, vRot);
        XMStoreFloat4(&vTransF, vTrans);

        TransformData transformData = { vScaleF, vRotF, vTransF };
        m_BasePose.push_back(transformData);
    }

    return S_OK;
}

HRESULT	CMasterModel::Initialize(void* pArg)
{
    return S_OK;
}

//MasterModel 은 본 + 애니메이션만 저장함 
HRESULT CMasterModel::Ready_MeshBones(_wstring& strMeshBoneFilePath)
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

    if (m_iNumBones <= 0)
    {
        MSG_BOX("CMasterModel : No Bone Data");
        return E_FAIL;
    }

    //Read Bones
    for (uint32_t i = 0; i < m_iNumBones; ++i)
    {
        BoneData boneData{};
        fin.read(reinterpret_cast<char*>(&boneData), sizeof(BoneData));

        CBone* pBone = CBone::Create(boneData.boneName, boneData.parentIndex, XMLoadFloat4x4(&boneData.transformMatrix));
        if (pBone == nullptr) return E_FAIL;

        m_Bones.push_back(pBone);
    }

    //Read Mesh 
    for (size_t i = 0; i < m_iNumMeshes; i++)
    {
        MeshData header{};
        fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));
        //값이 들어있을 경우를 대비하여 읽기만 함
    }

    m_iNumMeshes = 0;

    return S_OK;
}

CMasterModel* CMasterModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix)
{
    CMasterModel* pInstance = new CMasterModel(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(strFolderName, strModelName, PreTransformMatrix)))
    {
        MSG_BOX("Failed to Created : CMasterModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CMasterModel::Clone(void* pArg)
{
    CMasterModel* pInstance = new CMasterModel(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CMasterModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMasterModel::Free()
{
    __super::Free();
}