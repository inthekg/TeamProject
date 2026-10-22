
#include "StaticObject.h"
#include "GameInstance.h"
#include "RenderStruct.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CStaticObject, CGameObject)

END_REFLECTION(CStaticObject)
#endif

CStaticObject::CStaticObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{

}

CStaticObject::CStaticObject(const CStaticObject& Prototype)
    : CGameObject{ Prototype }
{

}

HRESULT CStaticObject::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;



    return S_OK;
}

HRESULT CStaticObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components())) return E_FAIL;

    return S_OK;
}

void CStaticObject::Priority_Update(_float fTimeDelta)
{
}

void CStaticObject::Update(_float fTimeDelta)
{
    
}

void CStaticObject::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
    m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CStaticObject::Render()
{
    if (!m_pModelCom) return E_FAIL;

    const auto& units = m_pModelCom->Get_RenderUnits();
    _uint iCount = static_cast<_uint>(units.size());


    //Static Mesh용 커맨드
    for (_uint i = 0; i < iCount; i++)
    {
        CMesh* pMesh = units[i].pMesh;
        CMaterial* pMaterialCom = units[i].pMaterial;

        if (pMesh == nullptr || pMaterialCom == nullptr)
        {
            MSG_BOX("StaticObject : Render Failed");
            return E_FAIL;
        }

    	StaticCommand command{};
    	command.header.etype = RENDER_TYPE::STATIC;
    	command.header.iCommandSize = sizeof(StaticCommand);
    	command.pVIBuffer = pMesh;
    	command.pMaterial = pMaterialCom;

        _matrix CombinedMatritx = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix()) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
        XMStoreFloat4x4(&command.worldMatrix, CombinedMatritx);

    	m_pGameInstance->Submit_Static_Resolved(command);
    }
    return S_OK;
}

HRESULT CStaticObject::Ready_Components()
{
    return S_OK;
}

CStaticObject* CStaticObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext )
{
    CStaticObject* pInstance = new CStaticObject(pDevice, pContext);

    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Created : CStaticObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CStaticObject::Clone(void* pArg)
{
    CStaticObject* pInstance = new CStaticObject(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CStaticObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CStaticObject::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
}
