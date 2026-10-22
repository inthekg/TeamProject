#include "StaticWeapon.h"
#include "Shader.h"	
#include "Model.h"
#include "DynamicRigidBody.h"
#include "Transform.h"
#include "GameInstance.h"
#include "Player.h"


CStaticWeapon::CStaticWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CWeapon(pDevice, pContext)
{
}

CStaticWeapon::CStaticWeapon(const CStaticWeapon& Prototype)
	:CWeapon(Prototype)	
{
}

HRESULT CStaticWeapon::Initialize_Prototype()
{
	__super::Initialize_Prototype();
	return S_OK;
}

HRESULT CStaticWeapon::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg))) return E_FAIL;

	return S_OK;
}

void CStaticWeapon::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CStaticWeapon::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);
}

void CStaticWeapon::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CStaticWeapon::Render()
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

        _matrix preCombinedMatritx = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix()) * XMLoadFloat4x4(&m_CombinedWorldMatrix);
        XMStoreFloat4x4(&command.worldMatrix, preCombinedMatritx);

        m_pGameInstance->Submit_Static_Resolved(command);
    }
    return S_OK;
}

void CStaticWeapon::Free()
{
	__super::Free();
}
