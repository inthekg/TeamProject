
#include "MapObject.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "Material.h"
#include "Shader.h"
#include "Shader_Resolver.h"
#include "Texture.h"


CMapObject::CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{

}

CMapObject::CMapObject(const CMapObject& Prototype)
    : CGameObject{ Prototype }
    , m_pMaterialcom{ Prototype.m_pMaterialcom }
    , m_pMeshCom{ Prototype.m_pMeshCom }
    , m_pShaderCom{ Prototype.m_pShaderCom }
    , m_LocalBounds{ Prototype.m_LocalBounds }
#ifdef _DEBUG
    , m_DebugMaterialComs{ Prototype.m_DebugMaterialComs }
#endif
{
    // 복제품은 원본과 렌더 리소스를 공유하고 Transform만 별도로 생성한다.
    Safe_AddRef(m_pMaterialcom);
    Safe_AddRef(m_pMeshCom);
    Safe_AddRef(m_pShaderCom);

#ifdef _DEBUG
    for (CMaterial* pMaterial : m_DebugMaterialComs)
        Safe_AddRef(pMaterial);
#endif
}


HRESULT CMapObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CMapObject::Priority_Update(_float fTimeDelta)
{
}

void CMapObject::Update(_float fTimeDelta)
{
    
}

void CMapObject::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
   // m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CMapObject::Render()
{
    if (!m_pMaterialcom || !m_pMeshCom)
    {
        MSG_BOX("MapObject : Render Failed ");
        return E_FAIL;
    }

    StaticCommand command{};
    command.header.etype = RENDER_TYPE::STATIC;
    command.header.iCommandSize = sizeof(StaticCommand);
    command.pVIBuffer = m_pMeshCom;
    command.pMaterial = m_pMaterialcom;
    command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();
    command.fEmissiveAmount = 1.f;
    command.iAlphaClip = m_pMaterialcom->Get_OpacityMaskSource();

    m_pGameInstance->Submit_Static_Resolved(command);
    return S_OK;
}


HRESULT CMapObject::Set_Mesh(wstring MeshName)
{
    CVIBuffer* pTemp = nullptr;
    if (FAILED(m_pGameInstance->Get_VIBuffer(MeshName, &pTemp))) return E_FAIL;
    m_pMeshCom = static_cast<CMesh*>(pTemp);
    return S_OK;
}


HRESULT CMapObject::Set_Material(wstring MaterialName)
{
    CMaterial* pMaterial = nullptr;

    if (FAILED(m_pGameInstance->Get_Material(MaterialName, &pMaterial)))
        return E_FAIL;

#ifdef _DEBUG
    for (CMaterial* pDebugMaterial : m_DebugMaterialComs)
        Safe_Release(pDebugMaterial);

    m_DebugMaterialComs.clear();
#endif

    Safe_Release(m_pMaterialcom);
    m_pMaterialcom = pMaterial;

#ifdef _DEBUG
    // 디버그 Inspector에서 기본 머테리얼도 0번 슬롯으로 보이도록 별도 참조를 보관한다.
    Safe_AddRef(m_pMaterialcom);
    m_DebugMaterialComs.push_back(m_pMaterialcom);
#endif

    return S_OK;
}

#ifdef _DEBUG
HRESULT CMapObject::Add_Material(wstring MaterialName)
{
    CMaterial* pMaterial = nullptr;

    if (FAILED(m_pGameInstance->Get_Material(MaterialName, &pMaterial)))
        return E_FAIL;

    for (CMaterial* pDebugMaterial : m_DebugMaterialComs)
    {
        if (pDebugMaterial == pMaterial)
        {
            // 같은 머테리얼 중복 등록을 막고 Get_Material에서 증가한 참조를 되돌린다.
            Safe_Release(pMaterial);
            return S_OK;
        }
    }

    // 디버그 전용 추가 머테리얼 슬롯으로 보관한다.
    m_DebugMaterialComs.push_back(pMaterial);

    return S_OK;
}
#endif

HRESULT CMapObject::Set_Transform(_fmatrix fModelMatrix)
{
    m_pTransformCom->Set_State(STATE::RIGHT, fModelMatrix.r[0]);
    m_pTransformCom->Set_State(STATE::UP, fModelMatrix.r[1]);
    m_pTransformCom->Set_State(STATE::LOOK, fModelMatrix.r[2]);
    m_pTransformCom->Set_State(STATE::POSITION, fModelMatrix.r[3]);


    return S_OK;
}

BoundingBox CMapObject::Get_WorldBounds() const
{
    BoundingBox worldBounds{};

    if (nullptr == m_pTransformCom)
        return worldBounds;

    m_LocalBounds.Transform(worldBounds, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()));
    return worldBounds;
}


CMapObject::MapPickedRenderInfo CMapObject::Get_PickedRenderInfo() const
{
    MapPickedRenderInfo Info{};

    if (m_pMaterialcom)
    {
        Info.strMaterialName = m_pMaterialcom->Get_Name();

        CShader_Resolver* pResolver = m_pGameInstance->Get_ShaderResolver();
        CShader* pShader = nullptr;

        // 일반 MapObject는 렌더러에서 Material_Meta 기준으로 static shader를 resolve한다.
        if (nullptr != pResolver)
            pShader = pResolver->Resolve(m_pMaterialcom->Get_Meta_Desc(), false);

        if (nullptr != pShader)
            Info.strShaderName = pShader->Get_Name();

        for (_uint i = 0; i < TEXTURE_TYPE_MAX; ++i)
        {
            TEXTURE_TYPE eType = static_cast<TEXTURE_TYPE>(i);
            vector<CTexture*>* pTextures = m_pMaterialcom->Get_Textures(eType);

            if (nullptr == pTextures)
                continue;

            for (CTexture* pTexture : *pTextures)
            {
                if (nullptr == pTexture)
                    continue;

                Info.Textures.emplace_back(eType, pTexture->Get_Name());
            }
        }

        Info.iBlendMode = -1;
    }

    return Info;
}


#ifdef _DEBUG
CMapObject::MapPickedRenderInfo CMapObject::Get_PickedRenderInfo(_uint iMaterialIndex) const
{
    MapPickedRenderInfo Info{};
    CMaterial* pMaterial = Get_DebugMaterial(iMaterialIndex);

    if (nullptr == pMaterial)
        return Info;

    Info.strMaterialName = pMaterial->Get_Name();

    CShader_Resolver* pResolver = m_pGameInstance->Get_ShaderResolver();
    CShader* pShader = nullptr;

    // 선택한 디버그 머테리얼 슬롯 기준으로 Shader/Texture 정보를 표시한다.
    if (nullptr != pResolver)
        pShader = pResolver->Resolve(pMaterial->Get_Meta_Desc(), false);

    if (nullptr != pShader)
        Info.strShaderName = pShader->Get_Name();

    for (_uint i = 0; i < TEXTURE_TYPE_MAX; ++i)
    {
        TEXTURE_TYPE eType = static_cast<TEXTURE_TYPE>(i);
        vector<CTexture*>* pTextures = pMaterial->Get_Textures(eType);

        if (nullptr == pTextures)
            continue;

        for (CTexture* pTexture : *pTextures)
        {
            if (nullptr == pTexture)
                continue;

            Info.Textures.emplace_back(eType, pTexture->Get_Name());
        }
    }

    Info.iBlendMode = -1;

    return Info;
}
#endif

HRESULT CMapObject::Ready_Components()
{
    return S_OK;
}

CMapObject* CMapObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapObject* pInstance = new CMapObject(pDevice, pContext);

    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Created : CMapObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CMapObject* CMapObject::Create_Copy(const CMapObject& Source)
{
    CMapObject* pInstance = new CMapObject(Source);

    // 가상 Clone을 사용하지 않아 CMapInstanceObject 오너가 일반 CMapObject로 잘리는 경로를 만들지 않는다.
    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Copied : CMapObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CMapObject::Free()
{
    __super::Free();

#ifdef _DEBUG
    for (CMaterial* pDebugMaterial : m_DebugMaterialComs)
        Safe_Release(pDebugMaterial);

    m_DebugMaterialComs.clear();
#endif

    Safe_Release(m_pShaderCom);
    Safe_Release(m_pMeshCom);
    Safe_Release(m_pMaterialcom);
}
