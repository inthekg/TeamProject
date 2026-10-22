#include "SkeletalWeapon.h"
#include "Shader.h"	
#include "Model.h"
#include "DynamicRigidBody.h"
#include "Transform.h"
#include "GameInstance.h"


CSkeletalWeapon::CSkeletalWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CWeapon(pDevice, pContext)
{

}

CSkeletalWeapon::CSkeletalWeapon(const CSkeletalWeapon& Prototype)
    :CWeapon(Prototype)
{
	for (_uint i = 0; i < ETOUI(AttackType::End); i++)
	{
		for (int j = 0; j < MAX_COMBO; j++)
		{
			m_FXNames[i][j] = Prototype.m_FXNames[i][j];
		}
	}
}

HRESULT CSkeletalWeapon::Initialize_Prototype()
{
    __super::Initialize_Prototype();

    return S_OK;
}

HRESULT CSkeletalWeapon::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg))) return E_FAIL;

    return S_OK;
}

void CSkeletalWeapon::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CSkeletalWeapon::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CSkeletalWeapon::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CSkeletalWeapon::Render()
{
	if (m_bSwapping) return S_OK;

	if (!m_pModelCom)
	{
		MSG_BOX("CSkeletalWeapon : Render_Anim Failed. Model is nullptr");
		return E_FAIL;
	}

	const auto& units = m_pModelCom->Get_RenderUnits();
	_uint iCount = static_cast<_uint>(units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

	for (_uint i = 0; i < iCount; i++)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;

		if (pMesh == nullptr || pMaterialCom == nullptr)
		{
			MSG_BOX("CBodyPart : Render_Anim Failed");
			return E_FAIL;
		}

		if (pMaterialCom->Get_RenderGroup() != eCurPass)
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);

		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = m_CombinedWorldMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

    return S_OK;
}

HRESULT CSkeletalWeapon::Render_Shadow()
{
	if (m_bSwapping) return S_OK;

	if (!m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	_uint iCount = static_cast<_uint>(units.size());
	for (_uint i = 0; i < iCount; i++)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMat = units[i].pMaterial;
		if (pMesh == nullptr || pMat == nullptr)
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);

		command.pVIBuffer = pMesh;
		command.pMaterial = pMat;
		command.worldMatrix = m_CombinedWorldMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;


		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;
		m_pGameInstance->Submit_Skinned_Resolved(command);
	}
	return S_OK;
}

void CSkeletalWeapon::Free()
{
    __super::Free();
}
