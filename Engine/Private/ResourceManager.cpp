#include "ResourceManager.h"
#include "Engine_Utility.h"
#include "GameInstance.h"
#include "Animation.h"

CResource_Manager::CResource_Manager()
    : m_pGameInstance(CGameInstance::Get_Instance())
{
}

HRESULT CResource_Manager::Get_VIBuffer(_wstring strVIBufferName, CVIBuffer** pOutBuffer)
{
    if (pOutBuffer == nullptr) return E_FAIL;

	shared_lock<shared_mutex> lock(m_ResourceLock); //RAII로 자동 unlock + 멀티 읽기 가능 

    CVIBuffer* pBuffer = Find_VIBuffer(strVIBufferName);
    if (pBuffer == nullptr) return E_FAIL;

    //카운트 증가 후 반환 
    Safe_AddRef(pBuffer);
    *pOutBuffer = pBuffer;

    return S_OK;
}

HRESULT CResource_Manager::Get_Texture(_wstring strTextureName, CTexture** pOutTexture)
{
    if (pOutTexture == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CTexture* pTexture = Find_Texture(strTextureName);
    if (pTexture == nullptr) return E_FAIL;

    Safe_AddRef(pTexture);
    *pOutTexture = pTexture;

    return S_OK;
}

HRESULT CResource_Manager::Get_Material(_wstring strMaterialName, CMaterial** pOutMaterial)
{
    if (pOutMaterial == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CMaterial* pMaterial = Find_Material(strMaterialName);
    if (pMaterial == nullptr) return E_FAIL;

    Safe_AddRef(pMaterial);
    *pOutMaterial = pMaterial;

    return S_OK;
}

HRESULT CResource_Manager::Get_Shader(_wstring strShaderName, CShader** pOutShader)
{
    if (pOutShader == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CShader* pShader = Find_Shader(strShaderName);
    if (pShader == nullptr) return E_FAIL;

    Safe_AddRef(pShader);
    *pOutShader = pShader;

    return S_OK;
}

HRESULT CResource_Manager::Get_Animation(_wstring strAnimationName, CAnimation** pOutAnimation)
{
    if (pOutAnimation == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CAnimation* pAnimation = Find_Animation(strAnimationName);
    if (pAnimation == nullptr) return E_FAIL;

    Safe_AddRef(pAnimation);
    *pOutAnimation = pAnimation;

    return S_OK;
}

HRESULT CResource_Manager::Get_VIBuffer(_uint64 iVIBufferKey, CVIBuffer** pOutBuffer)
{
	if (pOutBuffer == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CVIBuffer* pBuffer = Find_VIBuffer(iVIBufferKey);
    if (pBuffer == nullptr) return E_FAIL;

    Safe_AddRef(pBuffer);
    *pOutBuffer = pBuffer;

    return S_OK;
}

HRESULT CResource_Manager::Get_Texture(_uint64 iTextureKey, CTexture** pOutTexture)
{
	if (pOutTexture == nullptr) return E_FAIL;
    
    shared_lock<shared_mutex> lock(m_ResourceLock);

    CTexture* pTexture = Find_Texture(iTextureKey);
    if (pTexture == nullptr) return E_FAIL;

    Safe_AddRef(pTexture);
    *pOutTexture = pTexture;

    return S_OK;
}

HRESULT CResource_Manager::Get_Material(_uint64 iMaterialKey, CMaterial** pOutMaterial)
{
	if (pOutMaterial == nullptr) return E_FAIL;
    
    shared_lock<shared_mutex> lock(m_ResourceLock);

    CMaterial* pMaterial = Find_Material(iMaterialKey);
    if (pMaterial == nullptr) return E_FAIL;

    Safe_AddRef(pMaterial);
    *pOutMaterial = pMaterial;

    return S_OK;
}

HRESULT CResource_Manager::Get_Shader(_uint64 iShaderKey, CShader** pOutShader)
{
	if (pOutShader == nullptr) return E_FAIL;
    
    shared_lock<shared_mutex> lock(m_ResourceLock);

    CShader* pShader = Find_Shader(iShaderKey);
    if (pShader == nullptr) return E_FAIL;

    Safe_AddRef(pShader);
    *pOutShader = pShader;

    return S_OK;
}

HRESULT CResource_Manager::Get_Animation(_uint64 iAnimationKey, CAnimation** pOutAnimation)
{
    if (pOutAnimation == nullptr) return E_FAIL;

    shared_lock<shared_mutex> lock(m_ResourceLock);

    CAnimation* pAnimation = Find_Animation(iAnimationKey);
    if (pAnimation == nullptr) return E_FAIL;

    Safe_AddRef(pAnimation);
    *pOutAnimation = pAnimation;

    return S_OK;
}


void CResource_Manager::Get_MaterialSnapShot(vector<CMaterial*>& OutMaterials)
{
    shared_lock<shared_mutex> lock(m_ResourceLock);
    OutMaterials = m_MaterialVector;
}

HRESULT CResource_Manager::Input_VIBuffer(CVIBuffer* pVIBuffer)
{
    if (pVIBuffer == nullptr) return E_FAIL;

    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    if (Find_VIBuffer(pVIBuffer->Get_Name()) != nullptr) return E_FAIL;

    _uint64 iKey = Hash_String(pVIBuffer->Get_Name().c_str());
    m_VIBufferMap.emplace(iKey, pVIBuffer);

    pVIBuffer->Set_ID(m_iVIBufferID);
    ++m_iVIBufferID;

    return S_OK;
}

HRESULT CResource_Manager::Input_Shader(CShader* pShader)
{
    if (pShader == nullptr) return E_FAIL;

    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    if (Find_Shader(pShader->Get_Name()) != nullptr) return E_FAIL;

    _uint64 iKey = Hash_String(pShader->Get_Name().c_str());
    m_ShaderMap.emplace(iKey, pShader);

    pShader->Set_ID(m_iShaderID);
    ++m_iShaderID;

    return S_OK;
}

HRESULT CResource_Manager::Input_Texture(CTexture* pTexture)
{
    if (pTexture == nullptr) return E_FAIL;

    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    if (Find_Texture(pTexture->Get_Name()) != nullptr) return E_FAIL;

    _uint64 iKey = Hash_String(pTexture->Get_Name().c_str());
    m_TextureMap.emplace(iKey, pTexture);

    pTexture->Set_ID(m_iTextureID);
    ++m_iTextureID;

    return S_OK;
}

HRESULT CResource_Manager::Input_Material(CMaterial* pMaterial, _wstring& KeyName)
{
    if (pMaterial == nullptr) return E_FAIL;
    
    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    const wstring& name = (KeyName.empty() ? pMaterial->Get_Name() : KeyName);
    if (Find_Material(name) != nullptr) return E_FAIL;

    if (m_iMaterialID >= g_iNumMaxMaterals)
    {
        MSG_BOX("Material ID Overflow");
        return E_FAIL;
    }

    _uint64 iKey = Hash_String(name.c_str());

    m_MaterialMap.emplace(iKey, pMaterial);
	m_MaterialVector.push_back(pMaterial);

    pMaterial->Set_ID(m_iMaterialID);
    ++m_iMaterialID;

    m_bMaterialDirtyFlag = true;

    //wchar_t buf[256];
    //swprintf_s(buf, L"[MAT ADD] %s  total=%llu\n", name.c_str(), (unsigned long
    //    long)m_MaterialVector.size());
    //OutputDebugStringW(buf);

    return S_OK;
}

#pragma region 카피
HRESULT CResource_Manager::Input_StaticAnimation(CAnimation* pAnimation, _uint64 iKey)
{
    if (pAnimation == nullptr) return E_FAIL;

    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    if (0 == iKey) iKey = pAnimation->Get_AnimationFileHash();
    if (Find_Animation(iKey) != nullptr) return E_FAIL;

    m_StaticAnimationMap.emplace(iKey, pAnimation);
    pAnimation->Set_ID(m_iStaticAnimationID);
    ++m_iStaticAnimationID;

    return S_OK;
}


HRESULT CResource_Manager::Input_DynamicAnimation(CAnimation* pAnimation, _uint64 iKey)
{
    if (pAnimation == nullptr) return E_FAIL;

    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    if (0 == iKey) iKey = pAnimation->Get_AnimationFileHash();
    if (Find_Animation(iKey) != nullptr) return E_FAIL;

    m_DynamicAnimationMap.emplace(iKey, pAnimation);

    pAnimation->Set_ID(m_iDynamicAnimationID);
    ++m_iDynamicAnimationID;

    return S_OK;
}
#pragma endregion




HRESULT CResource_Manager::Input_MaterialFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strMaterialFilePath)
{
    if (pContext == nullptr || pDevice == nullptr || strMaterialFilePath.empty()) return E_FAIL;

    ifstream file(strMaterialFilePath);
    if (!file.is_open())
    {
        MSG_BOX("Resource Manager : Material File Open Fail");
        return E_FAIL;
    }

    json matJson;
    file >> matJson;

    _wstring MaterialName = Get_FileNameFromPath(strMaterialFilePath);

    CMaterial* pMaterial = nullptr;

    //여기서 락 잡음 
    Get_Material(MaterialName, &pMaterial);

    //이미 있으면 OK
    if (pMaterial != nullptr)
    {
        Safe_Release(pMaterial);
        return S_OK;
    }

    pMaterial = CMaterial::Create(pDevice, pContext);
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

            Get_Texture(strTextureName, &pTexture);

            if (pTexture == nullptr)
            {
                pTexture = CTexture::Create(pDevice, pContext, &strTextureName, &strTexturePath);
                if (pTexture == nullptr)
                {
                    _wstring msg = L"Texture not found : " + strTextureName;
                    MessageBox(nullptr, msg.c_str(), L"System Message", MB_OK);
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

    if (FAILED(Input_Material(pMaterial, MaterialName)))
    {
        Safe_Release(pMaterial);
        return E_FAIL;
    }
    file.close();
    return S_OK;
}

void CResource_Manager::Release_DynamicAnimations()
{
    unique_lock<std::shared_mutex> Lock(m_ResourceLock);

    for (auto& pair : m_DynamicAnimationMap)
    {
        Safe_Release(pair.second);
    }
    m_DynamicAnimationMap.clear();
}

HRESULT CResource_Manager::Input_TextureFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strTextureFilePath)
{
    if (pContext == nullptr || pDevice == nullptr || strTextureFilePath.empty()) return E_FAIL;

    wstring strTextureName = Get_FileNameFromPath(strTextureFilePath);

    CTexture* pTexture = CTexture::Create(pDevice, pContext, &strTextureName, &strTextureFilePath);
    if (pTexture == nullptr)
    {
        MSG_BOX("Resource Manager : Texture File Create Fail");
        return E_FAIL;
    }
    //여기서 락 잡음 
    if (FAILED(Input_Texture(pTexture)))
    {
        Safe_Release(pTexture);
        return E_FAIL;
    }
    return S_OK;
}

// find~ 함수들은 private으로 , public함수에서 lock을 걸고 들어올거라 별도의 lock은 걸지 않음
CVIBuffer* CResource_Manager::Find_VIBuffer(const _wstring& strVIBufferName)
{
    _uint64 iKey = Hash_String(strVIBufferName.c_str());
    auto iter = m_VIBufferMap.find(iKey);

    if (iter == m_VIBufferMap.end()) return nullptr;

    return iter->second;
}

CShader* CResource_Manager::Find_Shader(const _wstring& strShaderName)
{
    _uint64 iKey = Hash_String(strShaderName.c_str());
    auto iter = m_ShaderMap.find(iKey);

    if (iter == m_ShaderMap.end()) return nullptr;

    return iter->second;
}

CTexture* CResource_Manager::Find_Texture(const _wstring& strTextureName)
{
    _uint64 iKey = Hash_String(strTextureName.c_str());
    auto iter = m_TextureMap.find(iKey);

    if (iter == m_TextureMap.end()) return nullptr;

    return iter->second;
}

CMaterial* CResource_Manager::Find_Material(const _wstring& strMaterialName)
{
    _uint64 iKey = Hash_String(strMaterialName.c_str());
    auto iter = m_MaterialMap.find(iKey);

    if (iter == m_MaterialMap.end()) return nullptr;

    return iter->second;
}

CAnimation* CResource_Manager::Find_Animation(const _wstring& strAnimationName)
{
    _uint64 iKey = Hash_String(strAnimationName.c_str());

    auto StaticIter = m_StaticAnimationMap.find(iKey);
    if (StaticIter != m_StaticAnimationMap.end()) return StaticIter->second;

    auto DynamicIter = m_DynamicAnimationMap.find(iKey);
    if (DynamicIter != m_DynamicAnimationMap.end()) return DynamicIter->second;

    return nullptr;
}

CVIBuffer* CResource_Manager::Find_VIBuffer(_uint64 iVIBufferKey)
{
    auto iter = m_VIBufferMap.find(iVIBufferKey);

    if (iter == m_VIBufferMap.end()) return nullptr;

    return iter->second;
}

CShader* CResource_Manager::Find_Shader(_uint64 iShaderKey)
{
    auto iter = m_ShaderMap.find(iShaderKey);

    if (iter == m_ShaderMap.end()) return nullptr;

    return iter->second;
}

CTexture* CResource_Manager::Find_Texture(_uint64 iTextureKey)
{
    auto iter = m_TextureMap.find(iTextureKey);

    if (iter == m_TextureMap.end()) return nullptr;

    return iter->second;
}

CMaterial* CResource_Manager::Find_Material(_uint64 iMaterialKey)
{
    auto iter = m_MaterialMap.find(iMaterialKey);

    if (iter == m_MaterialMap.end()) return nullptr;
    return iter->second;
}

CAnimation* CResource_Manager::Find_Animation(_uint64 iAnimationNameHashKey)
{
    auto StaticIter = m_StaticAnimationMap.find(iAnimationNameHashKey);
    if (StaticIter != m_StaticAnimationMap.end()) return StaticIter->second;

    auto DynamicIter = m_DynamicAnimationMap.find(iAnimationNameHashKey);
    if (DynamicIter != m_DynamicAnimationMap.end()) return DynamicIter->second;

    return nullptr;
}

void CResource_Manager::Release_VIBuffer(_wstring& strVIBufferName)
{
    //여기선 Hash_String가 컴파일타임에 변환X. 매개변수가 constexpr 이 아님
    _uint64 iKey = Hash_String(strVIBufferName.c_str());
    auto iter = m_VIBufferMap.find(iKey);
    if (iter == m_VIBufferMap.end()) return;

    Safe_Release(iter->second);
    m_VIBufferMap.erase(iter);
}

void CResource_Manager::Release_Shader(_wstring& strShaderName)
{
    _uint64 iKey = Hash_String(strShaderName.c_str());
    auto iter = m_ShaderMap.find(iKey);

    if (iter == m_ShaderMap.end()) return;

    Safe_Release(iter->second);
    m_ShaderMap.erase(iter);
}

void CResource_Manager::Release_Texture(_wstring& strTextureName)
{
    _uint64 iKey = Hash_String(strTextureName.c_str());
    auto iter = m_TextureMap.find(iKey);
    if (iter == m_TextureMap.end()) return;

    Safe_Release(iter->second);
    m_TextureMap.erase(iter);
}

//void CResource_Manager::Release_Material(_wstring& strMaterialName)
//{
//    _uint64 iKey = Hash_String(strMaterialName.c_str());
//    auto iter = m_MaterialMap.find(iKey);
//    if (iter == m_MaterialMap.end()) return;
//
//    Safe_Release(iter->second);
//    m_MaterialMap.erase(iter);
//}

void CResource_Manager::Release_Animation(_wstring& strAnimationName)
{
    _uint64 iKey = Hash_String(strAnimationName.c_str());

    auto StaticIter = m_StaticAnimationMap.find(iKey);
    if (StaticIter != m_StaticAnimationMap.end())
    {
        Safe_Release(StaticIter->second);
        m_StaticAnimationMap.erase(StaticIter);
        return;
    }

    auto DynamicIter = m_DynamicAnimationMap.find(iKey);
    if (DynamicIter != m_DynamicAnimationMap.end())
    {
        Safe_Release(DynamicIter->second);
        m_DynamicAnimationMap.erase(DynamicIter);
        return;
    }
}

CResource_Manager* CResource_Manager::Create()
{
    return new CResource_Manager();
}

void CResource_Manager::Free()
{
    __super::Free();

    //순서 지키기 
    for (auto& Pair : m_VIBufferMap)
        Safe_Release(Pair.second);
    m_VIBufferMap.clear();

    for (auto& Pair : m_MaterialMap)
        Safe_Release(Pair.second);
    m_MaterialMap.clear();
    m_MaterialVector.clear();

    for (auto& Pair : m_TextureMap)
        Safe_Release(Pair.second);
    m_TextureMap.clear();

    for (auto& Pair : m_ShaderMap)
        Safe_Release(Pair.second);
    m_ShaderMap.clear();

    for (auto& Pair : m_StaticAnimationMap)
        Safe_Release(Pair.second);
    m_StaticAnimationMap.clear();

    for (auto& Pair : m_DynamicAnimationMap)
        Safe_Release(Pair.second);
    m_DynamicAnimationMap.clear();
}
