#include "Shooter.h"
#include "GameInstance.h"
#include "Model.h"
#include "Bone.h"
#include "Animator.h"
#include "Mesh.h"
#include "Material.h"
#include <cstdio>

#ifdef _DEBUG
BEGIN_REFLECTION(CShooter, CPartObject)
END_REFLECTION(CShooter)
#endif

CShooter::CShooter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPartObject{ pDevice, pContext }
{
}

CShooter::CShooter(const CShooter& Prototype)
	: CPartObject{ Prototype }
{
}

HRESULT CShooter::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CShooter::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	SHOOTER_DESC* pDesc = static_cast<SHOOTER_DESC*>(pArg);

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	m_vOffset = pDesc->vOffset;
	m_vScale = pDesc->vScale;

	if (FAILED(Ready_Model(*pDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CShooter::Ready_Model(const SHOOTER_DESC& Desc)
{
	if (FAILED(__super::Add_Component(Desc.iModelLevel, Desc.strModelProtoTag,
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	CAnimator::ANIMATOR_DESC AnimDesc{};
	AnimDesc.BasePose = *m_pModelCom->Get_BindPosePtr();
	AnimDesc.RawAnimationLoadDesc.strFolderName = Desc.strAnimFolderTag;
	AnimDesc.RawAnimationLoadDesc.strAnimationBaseName = Desc.strAnimFolderTag;
	AnimDesc.RawAnimationLoadDesc.eTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	AnimDesc.RawAnimationLoadDesc.bStaticAnimation = true;

	const vector<CBone*>& Bones = m_pModelCom->Get_Bones();
	AnimDesc.Bones.clear();
	AnimDesc.Bones.reserve(Bones.size());
	for (_uint i = 0; i < Bones.size(); ++i)
	{
		CAnimator::ANIMATOR_BONE_DESC BoneDesc{};
		if (nullptr != Bones[i])
		{
			BoneDesc.strName = Bones[i]->Get_Name();
			_wstring strBoneName;
			strBoneName.assign(BoneDesc.strName.begin(), BoneDesc.strName.end());
			BoneDesc.iNameHash = Hash_String(strBoneName.c_str());
			BoneDesc.iParentIndex = Bones[i]->Get_ParentIndex();
		}
		AnimDesc.Bones.push_back(BoneDesc);
	}
	AnimDesc.PreTransformMatrix = m_pModelCom->Get_PreTransformMatrix();

	m_pAnimator = CAnimator::Create(AnimDesc);
	if (nullptr == m_pAnimator)
		return E_FAIL;

	m_pAnimator->Play_Clip_ByIndex(0);
	m_pAnimator->Set_Loop(true);

	return S_OK;
}

void CShooter::Priority_Update(_float fTimeDelta)
{
}

void CShooter::Update(_float fTimeDelta)
{
	if (nullptr != m_pAnimator)
		m_pAnimator->Update(fTimeDelta);
}

void CShooter::Physical_Update(_float fFixedTime)
{
}

void CShooter::Late_Update(_float fTimeDelta)
{
	if (nullptr == m_pModelCom || nullptr == m_pAnimator || nullptr == m_pParentMatrix)
		return;

	m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	const _matrix matLocal = XMMatrixScaling(m_vScale.x, m_vScale.y, m_vScale.z)
		* XMMatrixTranslation(m_vOffset.x, m_vOffset.y, m_vOffset.z);
	Compute_CombinedWorldMatrix(matLocal);

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
	m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
}

HRESULT CShooter::Render()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = static_cast<_uint>(units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

	_int iSub = 0;
	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

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

		command.iAlphaClip = 0;
		command.fEmissiveAmount = 0.f;
		command.iRimProfile = 0;
		command.fRimAmount = 0.f;
		m_pGameInstance->Submit_Skinned_Resolved(command);
		++iSub;
	}

	return S_OK;
}

HRESULT CShooter::Render_Shadow()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = static_cast<_uint>(units.size());

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
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

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

CShooter* CShooter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CShooter* pInstance = new CShooter(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CShooter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CShooter::Clone(void* pArg)
{
	CShooter* pInstance = new CShooter(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CShooter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CShooter::Free()
{
	__super::Free();
	Safe_Release(m_pAnimator);
	Safe_Release(m_pModelCom);
}
