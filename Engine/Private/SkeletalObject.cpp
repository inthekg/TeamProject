#include "SkeletalObject.h"
#include "GameInstance.h"
#include "Model.h"
#include "RenderStruct.h"


CSkeletalObject::CSkeletalObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{

}

CSkeletalObject::CSkeletalObject(const CSkeletalObject& Prototype)
    : CGameObject{ Prototype }
{

}

HRESULT CSkeletalObject::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CSkeletalObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CSkeletalObject::Priority_Update(_float fTimeDelta)
{

}

void CSkeletalObject::Update(_float fTimeDelta)
{
}

void CSkeletalObject::Physical_Update(_float fFixedTime)
{
}

void CSkeletalObject::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
    m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

// 수정
HRESULT CSkeletalObject::Render()
{
    if (!m_pModelCom)
    {
        MSG_BOX("SkeletalObject : Render_Anim Failed. Model is nullptr");
        return E_FAIL;
    }

    const auto& units = m_pModelCom->Get_RenderUnits();
    _uint iCount = static_cast<_uint>(units.size());

    for (_uint i = 0; i < iCount; i++)
    {
        CMesh* pMesh = units[i].pMesh;
        CMaterial* pMaterialCom = units[i].pMaterial;

        if (pMesh == nullptr || pMaterialCom == nullptr)
        {
            MSG_BOX("SkeletalObject : Render_Anim Failed");
            return E_FAIL;
        }

        SkinnedCommand command{};
        command.header.etype = RENDER_TYPE::SKINNED;
        command.iBoneCount = pMesh->Get_BoneCount();
        command.header.iCommandSize = sizeof(SkinnedCommand);
        command.pVIBuffer = pMesh;
        command.pMaterial = pMaterialCom;
        command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

        if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
            continue;

        m_pGameInstance->Submit_Skinned_Resolved(command);
    }

    return S_OK;
}

HRESULT CSkeletalObject::Ready_Components()
{
    return S_OK;
}


void CSkeletalObject::Free()
{
    __super::Free();

    Safe_Release(m_pModelCom);

}
