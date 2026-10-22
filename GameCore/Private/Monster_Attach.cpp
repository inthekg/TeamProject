#include "Monster_Attach.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "Model.h"
#include "Mesh.h"
#include "AIObject.h"

CMonster_Attach::CMonster_Attach(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPartObject{ pDevice, pContext }
{
}

CMonster_Attach::CMonster_Attach(const CMonster_Attach& Prototype)
	: CPartObject{ Prototype }
{
}

HRESULT CMonster_Attach::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CMonster_Attach::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Attach");
		return E_FAIL;
	}

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	return S_OK;
}

void CMonster_Attach::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CMonster_Attach::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CMonster_Attach::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Attach::Late_Update(_float fTimeDelta)
{
	if (nullptr == m_pSocketBoneMatrix)
		return;

	_matrix SocketMatrix = XMLoadFloat4x4(m_pSocketBoneMatrix);

	_vector vScale, vRotation, vTranslation;
	if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, SocketMatrix))
	{
		SocketMatrix = XMMatrixAffineTransformation(
			XMVectorSet(1.f, 1.f, 1.f, 0.f), XMVectorZero(), vRotation, vTranslation);
	}

	_matrix WorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()) * SocketMatrix;
	__super::Compute_CombinedWorldMatrix(WorldMatrix);

	if (false == m_bRender)
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);

	if (true == m_bShadow)
		m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
}

HRESULT CMonster_Attach::Render()
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	_uint iCount = ETOUI(Units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterialCom = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		if (eCurPass != pMaterialCom->Get_RenderGroup())
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.iBoneCount = pMesh->Get_BoneCount();

		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = m_CombinedWorldMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.iAlphaClip = 0;
		command.fEmissiveAmount = m_fEmissiveAmount;
		command.iRimProfile = m_iRimProfile;
		command.fRimAmount = m_fRimAmount;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

HRESULT CMonster_Attach::Render_Shadow()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	_uint iCount = ETOUI(Units.size());

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterialCom = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.iBoneCount = pMesh->Get_BoneCount();

		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = m_CombinedWorldMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

HRESULT CMonster_Attach::Ready_Components(void* pArg)
{
	ATTACH_DESC Desc = *static_cast<ATTACH_DESC*>(pArg);

	m_pOwner =	Desc.pOwner;
	m_bRender = Desc.bRender;
	m_bShadow = Desc.bShadow;

	if (nullptr == m_pOwner || true == Desc.strSocketBoneName.empty() || true == Desc.strModelTag.empty())
	{
		MSG_BOX("CMonster_Attach : Invalid Desc");
		return E_FAIL;
	}

	if (FAILED(__super::Add_Component(Desc.iLevelID, Desc.strModelTag,
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	m_pModelCom->Update_Bones();

	m_pSocketBoneMatrix = m_pOwner->Get_Bone_CombinedMatPtr(Desc.strSocketBoneName.c_str());
	if (nullptr == m_pSocketBoneMatrix)
	{
		MSG_BOX("CMonster_Attach : Socket Bone Not Found");
		return E_FAIL;
	}

	return S_OK;
}

CMonster_Attach* CMonster_Attach::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Attach* pInstance = new CMonster_Attach(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Attach");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMonster_Attach::Clone(void* pArg)
{
	CMonster_Attach* pInstance = new CMonster_Attach(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Attach");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMonster_Attach::Free()
{
	__super::Free();

	Safe_Release(m_pModelCom);
}
