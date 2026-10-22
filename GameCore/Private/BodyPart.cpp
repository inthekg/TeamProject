#include "BodyPart.h"
#include "GameInstance.h"
#include "PartsModel.h"
#include "RenderStruct.h"

CBodyPart::CBodyPart(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CSkeletalObject{ pDevice, pContext }
{
}

CBodyPart::CBodyPart(const CBodyPart& Prototype)
	: CSkeletalObject{ Prototype }
{
}

HRESULT CBodyPart::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBodyPart::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CBodyPart Clone : Arg is nullptr");
		return E_FAIL;
	}

	BODYPART_DESC* pDesc = reinterpret_cast<BODYPART_DESC*>(pArg);
	m_pMasterModel = pDesc->pMasterModel;
	m_pParentMatrix = pDesc->pParentMatrix;
	m_iLevelIndex = pDesc->iLevelIndex;
	m_strModelTag = pDesc->strModelTag;
	m_bAlphaClip = pDesc->bAlphaClip;

	if(m_pMasterModel == nullptr)
	{
		MSG_BOX("CBodyPart Clone : MasterModel is nullptr");
		return E_FAIL;
	}

	if (FAILED(Ready_Components()))
		return E_FAIL;

	return S_OK;
}

HRESULT CBodyPart::Render()
{
	if (!m_pModelCom)
	{
		MSG_BOX("CBodyPart : Render_Anim Failed. Model is nullptr");
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
		command.worldMatrix = *m_pParentMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.iAlphaClip = m_bAlphaClip ? 1 : 0;
		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;
		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

HRESULT CBodyPart::Render_Shadow()
{
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

		//if (pMat->Get_RenderGroup() != RENDERID::NONBLEND)
		//	continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.pVIBuffer = pMesh;
		command.pMaterial = pMat;
		command.worldMatrix = *m_pParentMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}
	return S_OK;
}

void CBodyPart::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	m_fDissolveAmount = fAmount; 
	m_vDissolveColor = vColor;
}

HRESULT CBodyPart::Ready_Components()
{
	CPartsModel::PARTMODEL_DESC Desc = {};
	Desc.m_pMasterModel = m_pMasterModel;

	if (FAILED(__super::Add_Component(m_iLevelIndex, m_strModelTag, TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom),&Desc)))
		return E_FAIL;
	return S_OK;
}

CBodyPart* CBodyPart::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBodyPart* pInstance = new CBodyPart(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBodyPart");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBodyPart::Clone(void* pArg)
{
	CBodyPart* pInstance = new CBodyPart(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBodyPart");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBodyPart::Free()
{
	__super::Free();
}
