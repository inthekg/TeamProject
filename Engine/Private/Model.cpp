#include "Model.h"
#include "GameInstance.h"
#include "Bone.h"
#include "Animation.h"
#include "ModelDataStruct.h"
#include "Parser.h"
#include "Engine_Utility.h"
#include "Shader.h"
#include "Material_Parser.h"
#include "StructuredBuffer.h"
#include "GPUAnimationSystem.h"

CModel::CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent{ pDevice, pContext }
    , m_pInstance{ CGameInstance::Get_Instance() }
{
}

CModel::CModel(const CModel& Prototype)
    : CComponent{ Prototype }
    , m_eType{ Prototype.m_eType }
    , m_iNumMeshes{ Prototype.m_iNumMeshes }
    , m_Meshes{ Prototype.m_Meshes }
    , m_iNumMaterials{ Prototype.m_iNumMaterials }
    , m_Materials{ Prototype.m_Materials }
    , m_PreTransformMatrix{ Prototype.m_PreTransformMatrix }
    , m_pInstance{ CGameInstance::Get_Instance() }
    , m_BasePose(Prototype.m_BasePose)
    , m_BoneMatrixSet(Prototype.m_BoneMatrixSet)
	, m_iNumBones{ Prototype.m_iNumBones }
    , m_RenderUnits {Prototype.m_RenderUnits}
    , m_strMeshBoneFilePath { Prototype.m_strMeshBoneFilePath }
{
    m_bClone = true;

    //본만 개별 생성 
    for (auto& pPrototypeBone : Prototype.m_Bones)
        m_Bones.push_back(pPrototypeBone->Clone());

    for (auto& pMaterial : m_Materials)
        Safe_AddRef(pMaterial);

    for (auto& pMesh : m_Meshes)
        Safe_AddRef(pMesh);
}

_int CModel::Get_BoneIndex(const _char* pBoneName)
{
    _int iIndex = { -1 };

    auto iter = find_if(m_Bones.begin(), m_Bones.end(), [&](CBone* pBone)->_bool
        {
            ++iIndex;

            if (true == pBone->Compare_Name(pBoneName))
                return true;
            return false;
        });

    if (iter == m_Bones.end())
        return -1;

    return iIndex;
}

const _float4x4* CModel::Get_Bone_CombinedMatPtr(_uint iBoneIndex)
{
    if (iBoneIndex < 0 || iBoneIndex >= m_Bones.size()) return nullptr;
    return m_Bones[iBoneIndex]->Get_CombinedTransformationMatrixPtr();
}

const _float4x4* CModel::Get_Bone_CombinedMatPtr(const _char* pBoneName)
{
    _int iBoneIndex = Get_BoneIndex(pBoneName);

    if (iBoneIndex < 0 || iBoneIndex >= m_Bones.size()) return nullptr;
    return m_Bones[iBoneIndex]->Get_CombinedTransformationMatrixPtr();
}

const _char* CModel::Get_BoneName(_uint iBoneIndex) const
{
    if (iBoneIndex >= m_Bones.size()) return nullptr;
    return m_Bones[iBoneIndex]->Get_Name();
}

const _float4x4* CModel::Get_TransposedBoneMatrices(_uint iMeshIndex) const
{
    if (iMeshIndex >= m_iNumMeshes) return nullptr;
    return m_BoneMatrixSet[iMeshIndex].m_TransposedBoneMatrices.data();
}

CMesh* CModel::Get_Mesh(_uint iMeshIndex)
{
    if (m_iNumMeshes <= iMeshIndex) return nullptr;
    return m_Meshes[iMeshIndex];
}

CMaterial* CModel::Get_Material(_uint iMeshIndex)
{
    //TODO : 메시와 메테리얼 순서가 일치함 . 추후에 중복은 제거하도록 변경 
    if (iMeshIndex >= m_iNumMaterials) return nullptr;
    return m_Materials[iMeshIndex];
}

ID3D11ShaderResourceView* CModel::Get_BoneMatrixSRV() const
{
    if (m_pBoneMatrixBuffer == nullptr)
        return nullptr;

    return m_pBoneMatrixBuffer->Get_SRV();
}

ID3D11UnorderedAccessView* CModel::Get_BoneMatrixUAV() const
{
    if (m_pBoneMatrixBuffer == nullptr)
        return nullptr;

    return m_pBoneMatrixBuffer->Get_UAV();
}

_uint CModel::Get_MeshBoneOffset(_uint iMeshIndex) const
{
    if (iMeshIndex >= m_MeshBoneOffsets.size())
        return 0;

    return m_MeshBoneOffsets[iMeshIndex];
}

_uint CModel::Get_TotalPackedMeshBoneCount() const
{
    return static_cast<_uint>(m_PackedMeshBoneIndices.size());
}

HRESULT CModel::Fill_SkinnedCommandBoneInfo(SkinnedCommand& command, _uint iMeshIndex) const
{
    if (iMeshIndex >= m_iNumMeshes)
        return E_FAIL;

    if (m_bUseComputeMeshBone)
    {
        if (!m_bHasGPUFinalBoneRange)
            return E_FAIL;

        ID3D11ShaderResourceView* pGlobalFinalSRV = m_pGameInstance->Get_GlobalFinalBoneMatrixSRV();
        if (pGlobalFinalSRV == nullptr)
            return E_FAIL;

        command.pBoneMatrixSRV = pGlobalFinalSRV;
        command.iBoneOffset = m_iGPUFinalBoneOffset + Get_MeshBoneOffset(iMeshIndex);

        return S_OK;
    }

    ID3D11ShaderResourceView* pLocalFinalSRV = Get_BoneMatrixSRV();
    if (pLocalFinalSRV == nullptr)
        return E_FAIL;

    command.pBoneMatrixSRV = pLocalFinalSRV;
    command.iBoneOffset = Get_MeshBoneOffset(iMeshIndex);

    return S_OK;
}

HRESULT CModel::Fill_MeshSkinPalette(_uint iMeshIndex, vector<_float4x4>& OutPalette) const
{
    if (iMeshIndex >= m_iNumMeshes || m_Meshes[iMeshIndex] == nullptr)
        return E_FAIL;

    CMesh* pMesh = m_Meshes[iMeshIndex];
    const vector<_uint>& BoneIndices = pMesh->Get_BoneIndices();
    const vector<_float4x4>& OffsetMats = pMesh->Get_OffsetMatrices();
    const _uint iBoneCount = pMesh->Get_BoneCount();

    if (iBoneCount == 0 || BoneIndices.size() < iBoneCount || OffsetMats.size() < iBoneCount)
        return E_FAIL;

    OutPalette.resize(iBoneCount);
    for (_uint i = 0; i < iBoneCount; ++i)
    {
        const _uint iSkelBone = BoneIndices[i];
        if (iSkelBone >= m_Bones.size() || m_Bones[iSkelBone] == nullptr)
            return E_FAIL;

        _matrix matCombined = XMLoadFloat4x4(m_Bones[iSkelBone]->Get_CombinedTransformationMatrixPtr());
        _matrix matOffset = XMLoadFloat4x4(&OffsetMats[i]);
        XMStoreFloat4x4(&OutPalette[i], XMMatrixTranspose(XMMatrixMultiply(matOffset, matCombined)));
    }

    return S_OK;
}

HRESULT CModel::Fill_ModelSkinPalette(vector<_float4x4>& OutPalette) const
{
    OutPalette.clear();
    for (_uint m = 0; m < m_iNumMeshes; ++m)
    {
        CMesh* pMesh = m_Meshes[m];
        if (pMesh == nullptr)
            continue;

        const vector<_uint>& BoneIndices = pMesh->Get_BoneIndices();
        const vector<_float4x4>& OffsetMats = pMesh->Get_OffsetMatrices();
        const _uint iBoneCount = pMesh->Get_BoneCount();

        for (_uint i = 0; i < iBoneCount; ++i)
        {
            _float4x4 entry;
            const _bool bValid = (i < BoneIndices.size() && i < OffsetMats.size()
                && BoneIndices[i] < m_Bones.size() && m_Bones[BoneIndices[i]] != nullptr);
            if (bValid)
            {
                _matrix matCombined = XMLoadFloat4x4(m_Bones[BoneIndices[i]]->Get_CombinedTransformationMatrixPtr());
                _matrix matOffset = XMLoadFloat4x4(&OffsetMats[i]);
                XMStoreFloat4x4(&entry, XMMatrixTranspose(XMMatrixMultiply(matOffset, matCombined)));
            }
            else
                XMStoreFloat4x4(&entry, XMMatrixIdentity());

            OutPalette.push_back(entry);
        }
    }
    return OutPalette.empty() ? E_FAIL : S_OK;
}

HRESULT CModel::Initialize_Prototype(_wstring strModelName, _fmatrix PreTransformMatrix)
{
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);

    _wstring strResourceDir = m_pGameInstance->Get_ResourcePath() + L"/Models/" + strModelName + L"/";

    //각 경로 지정 
    _wstring strMeshBonePath = strResourceDir + strModelName + GlobalFileRoute::g_strBoneMeshFileTag;
    m_strMeshBoneFilePath = strMeshBonePath;

    if (FAILED(Ready_MeshBones(strMeshBonePath)))
    {
        MSG_BOX("Read Mesh , Bone Failed");
        return E_FAIL;
    }

    m_iNumMaterials = m_iNumMeshes;
    for(_uint i = 0; i < m_iNumMeshes; i++)
    {
		_wstring strMaterialName = m_Meshes[i]->Get_MaterialName();
        _wstring strMaterialPath = strResourceDir + GlobalFileRoute::g_strMaterialFolder +  strMaterialName + GlobalFileRoute::g_strMaterialFileTag;
        if (FAILED(Ready_Materials(strMaterialPath)))
        {
            MSG_BOX("Read Material Failed");
            return E_FAIL;
        }
	}

    if (m_eType == MODEL::ANIM)
    {
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
    }

    m_RenderUnits.reserve(m_iNumMeshes);
    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        m_RenderUnits.push_back({ m_Meshes[i], Get_Material(i) });
    }

  

    return S_OK;
}

HRESULT CModel::Initialize(void* pArg)
{
    if (FAILED(Ready_BoneMatrixBuffer()))
        return E_FAIL;

    if (FAILED(Ready_PackedMeshBoneData()))
        return E_FAIL;

    if (FAILED(Ready_CombinedBoneMatrixBuffer()))
        return E_FAIL;

    m_bGPUAnimationBuffersReady = false;

    return S_OK;
}

void CModel::Update_Bones(const vector<TransformData>* pFinalPose)
{    
    // 본 행렬
    if (pFinalPose != nullptr)
    {
        for (_uint i = 0; i < (_uint)m_Bones.size(); i++)
        {
            _vector vScale = XMLoadFloat4(&(*pFinalPose)[i].vScale);
            _vector vRotation = XMLoadFloat4(&(*pFinalPose)[i].vRot);
            _vector vTranslation = XMLoadFloat4(&(*pFinalPose)[i].vTranslation);

            _matrix TransformationMatrix =
                XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);

            m_Bones[i]->Set_TransformationMatrix(TransformationMatrix);
        }
    }

    for (auto& pBone : m_Bones)
    {
        pBone->Update_CombinedTransformMatrices(m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
    }

    //본이 갱신됏다면 Bone 버전 업 
    m_iBoneVersion++;
}

const vector<RenderUnit>& CModel::Get_RenderUnits()
{
    Update_MeshBones();
    return m_RenderUnits;
}

_matrix CModel::Compute_CombinedFromPose(const vector<TransformData>& Pose, _int iBoneIndex) const
{
    _matrix Combined = XMMatrixIdentity();

    if (iBoneIndex < 0 || iBoneIndex >= static_cast<_int>(m_Bones.size()) || Pose.size() != m_Bones.size())
        return Combined;

    //자식에서 최상위로 올라가므로 기존 코드와 반대로 로컬(부모 행렬)을 오른쪽에 곱함. 
    _int iIdx = iBoneIndex;
    while (iIdx >= 0)
    {
        _matrix Local = XMMatrixAffineTransformation(
            XMLoadFloat4(&Pose[iIdx].vScale), XMVectorSet(0.f, 0.f, 0.f, 1.f),
            XMLoadFloat4(&Pose[iIdx].vRot), XMLoadFloat4(&Pose[iIdx].vTranslation));
        
        Combined = Combined * Local;
        iIdx = m_Bones[iIdx]->Get_ParentIndex();
    }

    //루트 도달 
    if (iIdx < 0)
        Combined = Combined * XMLoadFloat4x4(&m_PreTransformMatrix);

    return Combined;
}


void CModel::Update_MeshBones()
{
    //버전이 변경될때만 Mesh의 본 행렬도 갱신 
    if (m_iLastBoneVersion == m_iBoneVersion) return;

    Update_CombinedBoneMatrixBuffer(m_Bones);

    if (m_bUseComputeMeshBone)
    {
        Dispatch_ComputeMeshBones();

        m_iLastBoneVersion = m_iBoneVersion;
        return;
    }

    // GPU 업로드용 배열도 넣음
    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        if (m_Meshes[i] == nullptr)
            continue;

        _float4x4* pCPUOut = m_BoneMatrixSet[i].m_TransposedBoneMatrices.data();

        m_Meshes[i]->Update_BoneMatrices(m_Bones, pCPUOut);

        if (m_pBoneMatrixBuffer != nullptr && i < m_MeshBoneOffsets.size())
        {
            const _uint iOffset = m_MeshBoneOffsets[i];
            const _uint iBoneCount = m_Meshes[i]->Get_BoneCount();

            if (iOffset + iBoneCount <= m_GPUUploadBoneMatrices.size())
            {
                memcpy(m_GPUUploadBoneMatrices.data() + iOffset, pCPUOut, sizeof(_float4x4) * iBoneCount);
            }
        }

        //m_Meshes[i]->Update_BoneMatrices(m_Bones, m_BoneMatrixSet[i].m_TransposedBoneMatrices.data());
    }

    if (m_pBoneMatrixBuffer != nullptr && !m_GPUUploadBoneMatrices.empty())
    {
        m_pBoneMatrixBuffer->Update_Buffer(m_GPUUploadBoneMatrices.data(), static_cast<_uint>(m_GPUUploadBoneMatrices.size()));
    }

    m_iLastBoneVersion = m_iBoneVersion;
}

HRESULT CModel::Ready_Materials(_wstring& strMaterialFilePath)
{
    ifstream file(strMaterialFilePath);
    if (!file.is_open())
    {
        MSG_BOX("Material File Open Fail");
        return E_FAIL;
    }

    json matJson;
    file >> matJson;

    _wstring MaterialName = Get_FileNameFromPath(strMaterialFilePath);

    CMaterial* pMaterial = nullptr;

    m_pInstance->Get_Material(MaterialName, &pMaterial);
    //이미 있으면 Return
    if (pMaterial != nullptr)
    {
        //Get_Material에서 Add_Ref됨 
        m_Materials.push_back(pMaterial);
        return S_OK;
    }

    pMaterial = CMaterial::Create(m_pDevice, m_pContext);
    if (pMaterial == nullptr) return E_FAIL;

    pMaterial->Set_Name(MaterialName);

    if (matJson.contains("Textures"))
    {
        for (auto& item : matJson["Textures"].items())
        {
            //TEXTURE_TYPE type = MapKeyToTextureType(item.key());
            if (!item.value().is_string()) continue;
            TEXTURE_TYPE type = MapKeyToTextureType(item.key(), item.value().get<string>());

            wstring strTextureOriginPath = StringToWString(item.value().get<string>());
            wstring strTextureName = Get_FileNameFromPath(strTextureOriginPath);
            //TODO : 모든 텍스쳐는 DDS라고 가정함
            wstring strTexturePath = m_pGameInstance->Get_ResourcePath() + L"/Textures/" + strTextureName + L".dds";

            CTexture* pTexture = nullptr;

            m_pGameInstance->Get_Texture(strTextureName, &pTexture);

            if (pTexture == nullptr)
            {
                pTexture = CTexture::Create(m_pDevice, m_pContext, &strTextureName, &strTexturePath);

                if (pTexture == nullptr)
                {
                    _wstring msg = L"Texture not found : " + strTextureName;
                    MessageBox(nullptr, msg.c_str(), L"System Message", MB_OK);
                    continue;
                }

                if (FAILED(m_pGameInstance->Register_Texture(pTexture)))
                {
                    Safe_Release(pTexture);

					//다른 스레드에서 이미 등록했을수도 있음. Get으로 다시 가져오기
                    m_pGameInstance->Get_Texture(strTextureName, &pTexture);
                    if (pTexture == nullptr) continue;
                    
                    pMaterial->Input_Texture(type, pTexture);
                    //Get으로 증가된 Texture Ref 감소 
                    Safe_Release(pTexture);
                    continue;
                }
                pMaterial->Input_Texture(type, pTexture);
            }
            else
            {
                pMaterial->Input_Texture(type, pTexture);
                //Get으로 증가된 Texture Ref 감소 
                Safe_Release(pTexture);
            }
        }
    }
    
    if (matJson.contains("Parameters"))
    {
        auto& params = matJson["Parameters"];

        if (params.contains("Scalars"))
        {
            for (auto& item : params["Scalars"].items())
            {
                string key = item.key();
                _float value = item.value().get<float>();
                pMaterial->Input_Scalar(key, value);
            }
        }

        if (params.contains("Colors"))
        {
            for (auto& item : params["Colors"].items())
            {
                std::string key = item.key();
                auto& colorObj = item.value(); 

                _float4 colorValue;
                colorValue.x = colorObj.value("R", 0.0f);
                colorValue.y = colorObj.value("G", 0.0f);
                colorValue.z = colorObj.value("B", 0.0f);
                colorValue.w = colorObj.value("A", 1.0f); 

                pMaterial->Input_Color(key, colorValue);
            }
        }
    }
    pMaterial->Set_Meta_Desc(Build_MaterialMeta(matJson));
    if (FAILED(m_pInstance->Register_Material(pMaterial)))
    {
        Safe_Release(pMaterial);

		//다른 스레드가 같은 메테리얼을 등록했을수도 있음. Get으로 다시 가져오기
		m_pInstance->Get_Material(MaterialName, &pMaterial);
		if (pMaterial == nullptr)
            return E_FAIL;

        m_Materials.push_back(pMaterial);
        file.close();
        return S_OK;
    }
    m_Materials.push_back(pMaterial);
    Safe_AddRef(pMaterial);

    file.close();
    return S_OK;
}

// 이 객체의 최종 본 행렬 결과가 들어갈 자리를 만든다.
HRESULT CModel::Ready_BoneMatrixBuffer()
{
    if (m_eType != MODEL::ANIM)
        return S_OK;

    m_MeshBoneOffsets.assign(m_iNumMeshes, 0);

    _uint iTotalBoneMatrixCount = 0;

    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        m_MeshBoneOffsets[i] = iTotalBoneMatrixCount;

        CMesh* pMesh = m_Meshes[i];
        if (pMesh != nullptr)
            iTotalBoneMatrixCount += pMesh->Get_BoneCount();
    }

    if (iTotalBoneMatrixCount == 0)
        return S_OK;

    m_iGPUFinalBoneCount = iTotalBoneMatrixCount;
    m_iGPUFinalBoneOffset = 0;
    m_bHasGPUFinalBoneRange = false;

    m_GPUUploadBoneMatrices.resize(iTotalBoneMatrixCount);
    
    return S_OK;
}

// 이 메쉬의 몇 번 본을 어떤 오프셋으로 계산해야 하는지 GPU에게 알려준다.
HRESULT CModel::Ready_PackedMeshBoneData()
{
    if (m_eType != MODEL::ANIM)
        return S_OK;

    if (m_MeshBoneOffsets.empty())
        return S_OK;

    _uint iTotalBoneCount = 0;

    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        CMesh* pMesh = m_Meshes[i];
        if (pMesh == nullptr)
            continue;

        iTotalBoneCount += pMesh->Get_BoneCount();
    }

    if (iTotalBoneCount == 0)
        return S_OK;

    m_PackedMeshBoneIndices.resize(iTotalBoneCount);
    m_PackedMeshOffsetMatrices.resize(iTotalBoneCount);

    for (_uint i = 0; i < m_iNumMeshes; i++)
    {
        CMesh* pMesh = m_Meshes[i];
        if (pMesh == nullptr)
            continue;

        if (i >= m_MeshBoneOffsets.size())
            continue;

        const _uint iBoneCount = pMesh->Get_BoneCount();
        if (iBoneCount == 0)
            continue;

        const _uint iDstOffset = m_MeshBoneOffsets[i];

        const vector<_uint>& BoneIndices = pMesh->Get_BoneIndices();
        const vector<_float4x4>& OffsetMatrices = pMesh->Get_OffsetMatrices();

        if (BoneIndices.size() < iBoneCount)
            return E_FAIL;

        if (OffsetMatrices.size() < iBoneCount)
            return E_FAIL;

        if (iDstOffset + iBoneCount > iTotalBoneCount)
            return E_FAIL;

        memcpy(m_PackedMeshBoneIndices.data() + iDstOffset, BoneIndices.data(), sizeof(_uint) * iBoneCount);
        memcpy(m_PackedMeshOffsetMatrices.data() + iDstOffset, OffsetMatrices.data(), sizeof(_float4x4) * iBoneCount);
    }

    m_iGPUMeshBoneInputOffset = 0;
    m_iGPUMeshBoneInputCount = 0;
    m_bHasGPUMeshBoneInputRange = false;

    return S_OK;
}

// 이 객체의 현재 포즈 / 계층 본 행렬이 들어갈 자리를 만든다
HRESULT CModel::Ready_CombinedBoneMatrixBuffer()
{
    if (m_eType != MODEL::ANIM)
        return S_OK;

    if (m_iNumBones == 0)
        return S_OK;

    m_CombinedBoneMatrices.resize(m_iNumBones);

    m_iGPUMeshBoneInputOffset = 0;
    m_iGPUMeshBoneInputCount = 0;
    m_bHasGPUMeshBoneInputRange = false;    
    
    return S_OK;
}

void CModel::Update_CombinedBoneMatrixBuffer(const vector<class CBone*>& Bones)
{
    if (Bones.empty())
        return;

    if (m_CombinedBoneMatrices.size() < Bones.size())
        m_CombinedBoneMatrices.resize(Bones.size());

    for (_uint i = 0; i < static_cast<_uint>(Bones.size()); i++)
    {
        if (Bones[i] == nullptr)
            continue;

        const _float4x4* pCombined = Bones[i]->Get_CombinedTransformationMatrixPtr();

        if (pCombined == nullptr)
            continue;

        m_CombinedBoneMatrices[i] = *pCombined;
    }

    const _uint iBoneCount = static_cast<_uint>(Bones.size());

    if (m_bHasGPUCombinedBoneRange)
    {
        m_pGameInstance->Update_GlobalCombinedBoneMatrixRange(m_iGPUCombinedBoneOffset, m_CombinedBoneMatrices.data(), iBoneCount);
    }
}

void CModel::Dispatch_ComputeMeshBones()
{
    if (m_pGameInstance == nullptr)
        return;

    if (FAILED(Ensure_GPUAnimationBuffers()))
        return;

    if (m_bHasGPUCombinedBoneRange && !m_CombinedBoneMatrices.empty())
    {
        m_pGameInstance->Update_GlobalCombinedBoneMatrixRange(m_iGPUCombinedBoneOffset, m_CombinedBoneMatrices.data(), static_cast<_uint>(m_CombinedBoneMatrices.size()));
    }

    m_pGameInstance->Register_ModelSkinningJob(this);
}

HRESULT CModel::Ready_GPUCombinedBoneRange(_uint iBoneCount)
{
    m_iGPUCombinedBoneOffset = 0;
    m_iGPUCombinedBoneCount = 0;
    m_bHasGPUCombinedBoneRange = false;

    if (!m_bClone || iBoneCount == 0)
        return S_OK;

    if (FAILED(m_pGameInstance->Allocate_CombinedBoneMatrixRange(iBoneCount, &m_iGPUCombinedBoneOffset)))
        return E_FAIL;

    m_iGPUCombinedBoneCount = iBoneCount;
    m_bHasGPUCombinedBoneRange = true;

    return S_OK;
}

void CModel::Free_GPUCombinedBoneRange()
{
    if (!m_bClone)
        return;

    if (!m_bHasGPUCombinedBoneRange)
        return;

    m_pGameInstance->Free_CombinedBoneMatrixRange(m_iGPUCombinedBoneOffset, m_iGPUCombinedBoneCount);

    m_iGPUCombinedBoneOffset = 0;
    m_iGPUCombinedBoneCount = 0;
    m_bHasGPUCombinedBoneRange = false;
}

HRESULT CModel::Ready_GPUAnimationBuffers()
{
    if (m_eType != MODEL::ANIM)
        return S_OK;

    if (!m_bClone || m_bGPUAnimationBuffersReady)
        return S_OK;

    if (m_iGPUFinalBoneCount > 0)
    {
        if (FAILED(m_pGameInstance->Allocate_FinalBoneMatrixRange(m_iGPUFinalBoneCount, &m_iGPUFinalBoneOffset)))
            return E_FAIL;

        m_bHasGPUFinalBoneRange = true;
    }

    if (!m_PackedMeshBoneIndices.empty())
    {
        if (FAILED(m_pGameInstance->Register_MeshBoneInputRange(m_PackedMeshBoneIndices, m_PackedMeshOffsetMatrices, &m_iGPUMeshBoneInputOffset, &m_iGPUMeshBoneInputCount)))
            return E_FAIL;

        m_bHasGPUMeshBoneInputRange = true;
    }

    if (!m_CombinedBoneMatrices.empty())
    {
        const _uint iBoneCount = static_cast<_uint>(m_CombinedBoneMatrices.size());

        if (FAILED(m_pGameInstance->Allocate_CombinedBoneMatrixRange(iBoneCount, &m_iGPUCombinedBoneOffset)))
            return E_FAIL;

        m_iGPUCombinedBoneCount = iBoneCount;
        m_bHasGPUCombinedBoneRange = true;
    }

    m_bGPUAnimationBuffersReady = true;

    return S_OK;
}

HRESULT CModel::Ensure_GPUAnimationBuffers()
{
    if (m_eType != MODEL::ANIM)
        return S_OK;

    if (m_bGPUAnimationBuffersReady)
        return S_OK;

    return Ready_GPUAnimationBuffers();
}

HRESULT CModel::Ready_MeshBones(_wstring& strMeshBoneFilePath)
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

        CBone* pBone = CBone::Create(boneData.boneName, boneData.parentIndex, XMLoadFloat4x4(&boneData.transformMatrix));
        if (pBone == nullptr) return E_FAIL;

        m_Bones.push_back(pBone);
    }

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
			//다른 스레드가 같은 메시를 등록했을수도 있음. Get으로 다시 가져오기
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

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strModelName, _fmatrix PreTransformMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(strModelName, PreTransformMatrix)))
    {
        _wstring strMessage = L"Failed to Created : CModel\nModel Name : " + strModelName;
        MessageBoxW(nullptr, strMessage.c_str(), L"System Message", MB_OK);
        Safe_Release(pInstance);
    }

    return pInstance;   
}

CComponent* CModel::Clone(void* pArg)
{
    CModel* pInstance = new CModel(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CModel::Free()
{
    __super::Free();

    if (m_bClone && m_bHasGPUFinalBoneRange && m_pGameInstance != nullptr)
    {
        m_pGameInstance->Free_FinalBoneMatrixRange(m_iGPUFinalBoneOffset, m_iGPUFinalBoneCount);

        m_iGPUFinalBoneOffset = 0;
        m_iGPUFinalBoneCount = 0;
        m_bHasGPUFinalBoneRange = false;
    }

    if (m_bClone && m_bHasGPUMeshBoneInputRange && m_pGameInstance != nullptr)
    {
        m_pGameInstance->Free_MeshBoneInputRange(m_iGPUMeshBoneInputOffset, m_iGPUMeshBoneInputCount);

        m_iGPUMeshBoneInputOffset = 0;
        m_iGPUMeshBoneInputCount = 0;
        m_bHasGPUMeshBoneInputRange = false;
    }

    Free_GPUCombinedBoneRange();

    for (auto& pBone : m_Bones)
        Safe_Release(pBone);
    m_Bones.clear();

    for (auto& pMaterial : m_Materials)
        Safe_Release(pMaterial);
    m_Materials.clear();

    for (auto& pMesh : m_Meshes)
        Safe_Release(pMesh);

    Safe_Release(m_pBoneMatrixBuffer);

    m_Meshes.clear();

}