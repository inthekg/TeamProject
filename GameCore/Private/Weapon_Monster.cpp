#include "Weapon_Monster.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "Model.h"
#include "Mesh.h"
#include "AIObject.h"
#include "Effect.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CWeapon_Monster, CPartObject)
END_REFLECTION(CWeapon_Monster)
#endif // _DEBUG

CWeapon_Monster::CWeapon_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPartObject{ pDevice, pContext }
{
}

CWeapon_Monster::CWeapon_Monster(const CWeapon_Monster& Prototype)
	: CPartObject{ Prototype }
{
}

HRESULT CWeapon_Monster::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CWeapon_Monster::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CWeapon_Monster");
		return E_FAIL;
	}

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	return S_OK;
}

void CWeapon_Monster::Priority_Update(_float fTimeDelta)
{

}

void CWeapon_Monster::Update(_float fTimeDelta)
{
}

void CWeapon_Monster::Physical_Update(_float fFixedTime)
{

}

void CWeapon_Monster::Late_Update(_float fTimeDelta)
{
	Update_ParryFlash(fTimeDelta);

	_matrix SocketMatrix = XMLoadFloat4x4(m_pSocketBoneMatrix);

	_vector vScale, vRotation, vTranslation;
	if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, SocketMatrix))
	{
		SocketMatrix = XMMatrixAffineTransformation(
			XMVectorSet(1.f, 1.f, 1.f, 0.f), XMVectorZero(), vRotation, vTranslation);
	}

	_matrix WorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()) * SocketMatrix;
	__super::Compute_CombinedWorldMatrix(WorldMatrix);

	Update_Attack(fTimeDelta);

	if (true == m_bRender)
	{
		m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
		m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
	}
}

HRESULT CWeapon_Monster::Render()
{
	if (nullptr == m_pModelCom)
	{
		MSG_BOX("CBodyPart : Render_Anim Failed. Model is nullptr");
		return E_FAIL;
	}

	const auto& Units = m_pModelCom->Get_RenderUnits();
	_uint iCount = ETOUI(Units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterialCom = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterialCom)
		{
			MSG_BOX("CWeapon_Monster Not Found Mesh , Materials");
			return E_FAIL;
		}

		if (eCurPass != pMaterialCom->Get_RenderGroup())
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
		command.fEmissiveAmount = m_fEmissiveAmount;
		command.iRimProfile = m_iRimProfile;
		command.fRimAmount = m_fRimAmount;
		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

HRESULT CWeapon_Monster::Render_Shadow()
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


void CWeapon_Monster::Begin_ParryFlash()
{
	if (m_fParryFlashTime <= 0.f)
		return;

	m_fParryFlashTimer = m_fParryFlashTime;

	Set_RimProfile(PARRY_RIM_PROFILE);
	Set_RimAmount(m_fParryFlashPeak);
}

_bool CWeapon_Monster::Get_BoneWorld(const string& strBoneName, _matrix& OutMatrix)
{
	if (nullptr == m_pModelCom || true == strBoneName.empty())
		return false;

	const _float4x4* pBoneCombined = m_pModelCom->Get_Bone_CombinedMatPtr(strBoneName.c_str());
	if (nullptr == pBoneCombined)
		return false;

	OutMatrix = XMLoadFloat4x4(pBoneCombined) * XMLoadFloat4x4(&m_CombinedWorldMatrix);

	return true;
}


HRESULT CWeapon_Monster::Ready_Components(void* pArg)
{
	auto pDesc = static_cast<WEAPON_DESC*>(pArg);
	m_pOwner = pDesc->pOwner;
	m_pSocketBoneMatrix = pDesc->pSocketBoneMatrix;
	m_bRender = pDesc->bRender;
	m_fDamage = pDesc->fDamage;

	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), pDesc->strModelTag,
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	m_pModelCom->Update_Bones();

	return S_OK;
}

HRESULT CWeapon_Monster::Get_BoneWorldMatrix(_int iBoneIndex, _matrix& OutMatrix)
{
	if (nullptr == m_pModelCom || iBoneIndex < 0)
		return E_FAIL;

	const _float4x4* pBoneCombined = m_pModelCom->Get_Bone_CombinedMatPtr(static_cast<_uint>(iBoneIndex));
	if (nullptr == pBoneCombined)
		return E_FAIL;

	OutMatrix = XMLoadFloat4x4(pBoneCombined) * XMLoadFloat4x4(&m_CombinedWorldMatrix);
	return S_OK;
}

void CWeapon_Monster::Update_GroundSpark(_float fTimeDelta, _fvector vRayOrigin, _fvector vRayDir, _float fRayLength, _bool bSweepTouch)
{
	if (nullptr == m_pOwner)
	{
		Stop_GroundSpark();
		return;
	}

	const string* pFxName = m_pOwner->Get_GroundSparkFx();
	if (nullptr == pFxName)
	{
		Stop_GroundSpark();
		return;
	}

	if (nullptr != m_pSparkFx && m_strSparkFx != *pFxName)
		Stop_GroundSpark();

	if (fRayLength > 1e-4f && XMVectorGetX(XMVector3LengthSq(vRayDir)) > 1e-6f)
	{
		_float3 vOrigin, vDir;
		XMStoreFloat3(&vOrigin, vRayOrigin);
		XMStoreFloat3(&vDir, XMVector3Normalize(vRayDir));

		CollisionResult tBlade{};
		if (true == m_pGameInstance->Raycast(vOrigin, vDir, fRayLength + m_fSparkRayExtend,
			FILTER_GROUP::GROUND, tBlade, PxQueryFlag::eSTATIC))
		{
			const _bool bOnBlade = (tBlade.tRaycastData.fDistance <= fRayLength);

			if (true == bOnBlade || true == bSweepTouch)
			{
				XMStoreFloat4x4(&m_matSparkRoot, XMMatrixTranslationFromVector(XMLoadFloat3(&tBlade.tRaycastData.vHitPosition)));

				m_fSparkGrace = m_fSparkGraceTime;

				if (nullptr == m_pSparkFx)
				{
					m_pSparkFx = m_pGameInstance->Play_Effect(*pFxName, &m_matSparkRoot, nullptr);
					m_strSparkFx = *pFxName;
				}

				return;
			}
		}
	}

	if (nullptr == m_pSparkFx)
		return;

	if (true == bSweepTouch)
	{
		m_fSparkGrace = m_fSparkGraceTime;
		return;
	}

	m_fSparkGrace -= fTimeDelta;

	if (m_fSparkGrace <= 0.f)
		Stop_GroundSpark();
}

void CWeapon_Monster::Stop_GroundSpark()
{
	m_fSparkGrace = 0.f;

	if (nullptr == m_pSparkFx)
		return;

	m_pSparkFx->Stop_Follow();
	m_pSparkFx->Stop();

	m_pSparkFx = nullptr;
	m_strSparkFx.clear();
}

void CWeapon_Monster::Update_ParryFlash(_float fTimeDelta)
{
	if (m_fParryFlashTimer <= 0.f)
		return;

	m_fParryFlashTimer -= fTimeDelta;

	if (m_fParryFlashTimer <= 0.f)
	{
		m_fParryFlashTimer = 0.f;

		Set_RimProfile(0);
		Set_RimAmount(0.f);

		if (nullptr != m_pOwner && true == m_pOwner->Is_SuperArmor())
			m_pOwner->On_SuperArmor(true);

		return;
	}

	const _float fRatio = m_fParryFlashTimer / m_fParryFlashTime;

	Set_RimAmount(m_fParryFlashPeak * Ease(EASE_TYPE::EASE_IN, fRatio));
}


void CWeapon_Monster::Free()
{
	Stop_GroundSpark();

	__super::Free();

	Safe_Release(m_pModelCom);
}

#ifdef _DEBUG
void CWeapon_Monster::Debug_GroundSparkLog(_bool bRayHit, _bool bSweepTouch, _fvector vRayOrigin, _fvector vRayDir, _float fRayLength)
{
	const _vector vTip = vRayOrigin + XMVector3Normalize(vRayDir) * fRayLength;

	_float3 vProbeOrigin, vProbeDir;
	XMStoreFloat3(&vProbeOrigin, vRayOrigin + XMVectorSet(0.f, 5.f, 0.f, 0.f));
	vProbeDir = _float3(0.f, -1.f, 0.f);

	CollisionResult tProbe{};
	const _bool bProbe = m_pGameInstance->Raycast(vProbeOrigin, vProbeDir, 50.f, FILTER_GROUP::GROUND, tProbe, PxQueryFlag::eSTATIC);
	const _float fGroundY = bProbe ? tProbe.tRaycastData.vHitPosition.y : -9999.f;

	const _char* pState = "OFF";
	if (true == bRayHit)
		pState = (nullptr == m_pSparkFx) ? "RAY_START" : "RAY_KEEP";
	else if (true == bSweepTouch)
		pState = (nullptr == m_pSparkFx) ? "SWEEP_START" : "SWEEP_KEEP";
	else if (nullptr != m_pSparkFx)
		pState = "GRACE";

	_char szLog[200] = {};
	sprintf_s(szLog, "[GSpark] %-11s ray=%d sweep=%d tipGap=%7.4f grace=%5.3f fx=%d",
		pState, bRayHit ? 1 : 0, bSweepTouch ? 1 : 0,
		XMVectorGetY(vTip) - fGroundY, m_fSparkGrace, (nullptr != m_pSparkFx) ? 1 : 0);

	OutputDebugStringA(szLog);
}
#endif
