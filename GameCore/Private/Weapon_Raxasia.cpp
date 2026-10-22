#include "Weapon_Raxasia.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "Model.h"
#include "Mesh.h"
#include "AIObject.h"
#include "HitUtil.h"
#include "Effect.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif

#ifdef _DEBUG
BEGIN_REFLECTION(CWeapon_Raxasia, CWeapon_Monster)
ADD_PROPERTY_SETTER(CWeapon_Raxasia, m_fSweepRadius, Float, Set_SweepRadius, _float)
ADD_PROPERTY_SETTER(CWeapon_Raxasia, m_fSweepHalfHeight, Float, Set_SweepHeight, _float)
ADD_PROPERTY_SETTER(CWeapon_Raxasia, m_fGroundRayExtend, Float, Set_GroundRayExtend, _float)
ADD_PROPERTY_SETTER(CWeapon_Raxasia, m_fSparkGraceTime, Float, Set_SparkGraceTime, _float)
ADD_PROPERTY_SETTER(CWeapon_Raxasia, m_fSparkRayExtend, Float, Set_SparkRayExtend, _float)
END_REFLECTION(CWeapon_Raxasia)
#endif




CWeapon_Raxasia::CWeapon_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CWeapon_Monster{ pDevice, pContext }
{
}

CWeapon_Raxasia::CWeapon_Raxasia(const CWeapon_Raxasia& Prototype)
	: CWeapon_Monster{ Prototype }
{
}

HRESULT CWeapon_Raxasia::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CWeapon_Raxasia::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	auto pDesc = static_cast<RAXASIA_WEAPON_DESC*>(pArg);
	m_fSweepRadius = pDesc->fSweepRadius;
	m_fSweepHalfHeight = pDesc->fSweepHalfHeight;
	m_fGroundRayExtend = pDesc->fGroundRayExtend;
	m_strTrailTop = pDesc->strBladeTopBoneName;
	m_strTrailBot = pDesc->strBladeBottomBoneName;

	if (false == pDesc->strBladeTopBoneName.empty())
		m_iBladeTopBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strBladeTopBoneName.c_str());
	if (false == pDesc->strBladeBottomBoneName.empty())
		m_iBladeBottomBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strBladeBottomBoneName.c_str());

	m_MeshVisibles.assign(m_pModelCom->Get_RenderUnits().size(), true);

	Set_Phase(pDesc->bPhase2);

#ifdef _DEBUG
	Rebuild_DebugCapsule();
#endif

	return S_OK;
}

void CWeapon_Raxasia::Priority_Update(_float fTimeDelta)
{
}

void CWeapon_Raxasia::Update(_float fTimeDelta)
{
}

void CWeapon_Raxasia::Physical_Update(_float fFixedTime)
{
}

void CWeapon_Raxasia::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		if (m_fSweepRadius != m_fLastDbgRadius || m_fSweepHalfHeight != m_fLastDbgHalfHeight)
			Rebuild_DebugCapsule();

		_matrix BladeTop, BladeBot;
		if (nullptr != m_pDebugCapsule && SUCCEEDED(Get_BladeMatrix(BladeTop, BladeBot)))
		{
			const _vector vTop = BladeTop.r[3];
			const _vector vBot = BladeBot.r[3];
			const _vector vCenter = (vTop + vBot) * 0.5f;

			_vector vBladeDir = vTop - vBot;
			if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBladeDir)), 0.f))
				vBladeDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
			vBladeDir = XMVector3Normalize(vBladeDir);

			const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
			const _float  fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBladeDir));
			_vector vQuat;
			if (IsEqual(fDot, 1.f))
				vQuat = XMQuaternionIdentity();
			else if (IsEqual(fDot, -1.f))
				vQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
			else
			{
				const _vector vAxis = XMVector3Normalize(XMVector3Cross(vAxisX, vBladeDir));
				vQuat = XMQuaternionRotationAxis(vAxis, acosf(fDot));
			}

			m_pDebugCapsule->Update_Pose(vCenter, vQuat);
			m_pDebugCapsule->Late_Update(fTimeDelta);
		}
	}
#endif
}

HRESULT CWeapon_Raxasia::Render()
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = ETOUI(Units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterialCom = Units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

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

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

void CWeapon_Raxasia::Attack_Begin(_float fDamage, HIT_TYPE eType)
{
	m_eHitType = eType;
	m_bCollisionActive = true;
	m_bPrevCenterValid = false;
	m_bHitSuccess = false;
	m_CollisionTargets.clear();
	if (fDamage >= 0.f)
		m_fDamage = fDamage;
}


void CWeapon_Raxasia::Attack_End()
{
	m_bCollisionActive = false;
}

void CWeapon_Raxasia::Update_Attack(_float fTimeDelta)
{
	_matrix BladeTop, BladeBot;
	if (FAILED(Get_BladeMatrix(BladeTop, BladeBot)))
	{
		m_bPrevCenterValid = false;
		Stop_GroundSpark();
		return;
	}

	const _vector vTop = BladeTop.r[3];
	const _vector vBot = BladeBot.r[3];
	const _vector vCenter = (vTop + vBot) * 0.5f;

	const _vector vBladeVec = vTop - vBot;
	const _float  fBladeLength = XMVectorGetX(XMVector3Length(vBladeVec));

	_vector vBladeDir = vBladeVec;
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBladeDir)), 0.f))
		vBladeDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	vBladeDir = XMVector3Normalize(vBladeDir);

	const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	const _float  fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBladeDir));
	_vector vQuat;
	if (IsEqual(fDot, 1.f))
		vQuat = XMQuaternionIdentity();
	else if (IsEqual(fDot, -1.f))
		vQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
	else
	{
		const _vector vAxis = XMVector3Normalize(XMVector3Cross(vAxisX, vBladeDir));
		vQuat = XMQuaternionRotationAxis(vAxis, acosf(fDot));
	}

	CollisionResult tGroundSweep{};
	const _bool bSweepTouch = m_pGameInstance->Sweep_Capsule(m_fSweepRadius, m_fSweepHalfHeight, vCenter, vQuat,
		XMVectorSet(0.f, -1.f, 0.f, 0.f), 0.01f, FILTER_GROUP::GROUND, tGroundSweep, PxQueryFlag::eSTATIC);

	Update_GroundSpark(fTimeDelta, vBot, vBladeDir, fBladeLength, bSweepTouch);

	if (false == m_bPrevCenterValid)
	{
		XMStoreFloat3(&m_vPrevCenter, vCenter);
		m_bPrevCenterValid = true;
		return;
	}

	const _vector vPrev = XMLoadFloat3(&m_vPrevCenter);
	XMStoreFloat3(&m_vPrevCenter, vCenter);

	if (false == m_bCollisionActive)
		return;

	HitInfo tHit;
	tHit.fDamage = m_fDamage;
	tHit.pOtherActor = m_pOwner;
	tHit.pDamangeCauser = this;
	tHit.eHitType = m_eHitType;

	_bool bDamaged = false;

	const _float fMove = XMVectorGetX(XMVector3Length(vCenter - vPrev));
	if (fMove <= 1e-4f)
	{
		CHitUtil::Overlap_Sphere(m_pGameInstance, m_fSweepRadius + m_fSweepHalfHeight, vCenter,
			tHit, m_CollisionTargets, m_eTargetGroup, &bDamaged);


		if (true == bDamaged)
			m_bHitSuccess = true;

		return;
	}

	CHitUtil::Sweep_Capsule(m_pGameInstance, m_fSweepRadius, m_fSweepHalfHeight,
		vPrev, vCenter, vQuat, tHit, m_CollisionTargets, m_eTargetGroup, &bDamaged);

	if (true == bDamaged)
		m_bHitSuccess = true;
}



HRESULT CWeapon_Raxasia::Get_BladeMatrix(_matrix& OutTop, _matrix& OutBottom)
{
	if (m_iBladeTopBoneIndex == -1 || m_iBladeBottomBoneIndex == -1)
		return E_FAIL;

	if (FAILED(Get_BoneWorldMatrix(m_iBladeTopBoneIndex, OutTop)))
		return E_FAIL;
	if (FAILED(Get_BoneWorldMatrix(m_iBladeBottomBoneIndex, OutBottom)))
		return E_FAIL;

	return S_OK;
}

_bool CWeapon_Raxasia::Get_BladeTipWorld(_vector& OutPos)
{
	_matrix matTop, matBottom;
	if (FAILED(Get_BladeMatrix(matTop, matBottom)))
		return false;

	OutPos = matTop.r[3];
	return true;
}

_bool CWeapon_Raxasia::Get_BladeGrabSocket(_float4x4& OutMat)
{
	if (nullptr == m_pModelCom)
		return false;

	const _int iBoneIndex = m_pModelCom->Get_BoneIndex("Weapon_Repulse");
	if (-1 == iBoneIndex)
		return false;

	_matrix matBone;
	if (FAILED(Get_BoneWorldMatrix(iBoneIndex, matBone)))
		return false;

	XMStoreFloat4x4(&OutMat, matBone);
	return true;
}

void CWeapon_Raxasia::Start_Trail()
{
	if ( nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return;

	if (m_pTrail != nullptr)
	{
		m_pTrail->Stop();
		m_pTrail = nullptr;
	}

	if (m_strTrailTop.empty() || m_strTrailBot.empty())
		return;

	const _float4x4* pTop = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailTop.c_str());
	const _float4x4* pBot = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailBot.c_str());
	if (nullptr == pTop || nullptr == pBot)
		return;

	m_pTrail = m_pGameInstance->Play_Effect("Effect_Raxasia_Normal_Trail", pTop, pBot, Get_CombinedWorldMatrixPtr());
}

void CWeapon_Raxasia::Start_Elec_Trail()
{
	if (nullptr != m_pTrail || nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return;

	if (m_strTrailTop.empty() || m_strTrailBot.empty())
		return;

	const _float4x4* pTop = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailTop.c_str());
	const _float4x4* pBot = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailBot.c_str());
	if (nullptr == pTop || nullptr == pBot)
		return;

	m_pTrail = m_pGameInstance->Play_Effect("Effect_Raxasia_Thunder_Trail", pTop, pBot, Get_CombinedWorldMatrixPtr());
}

void CWeapon_Raxasia::Stop_Trail()
{
	if (nullptr != m_pTrail)
	{
		m_pTrail->Stop_Follow();
		m_pTrail->Stop();
		m_pTrail = nullptr;
	}
}

vector<_int> CWeapon_Raxasia::Find_MeshIndices(const _wstring& strMeshKey) const
{
	vector<_int> Indices;
	if (nullptr == m_pModelCom)
		return Indices;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	for (_uint i = 0; i < ETOUI(Units.size()); ++i)
	{
		if (nullptr != Units[i].pMesh && Units[i].pMesh->Get_Name().find(strMeshKey) != _wstring::npos)
			Indices.push_back(ETOI(i));
	}
	return Indices;
}

void CWeapon_Raxasia::Set_MeshVisible(const _wstring& strMeshKey, _bool bVisible)
{
	for (_int iIndex : Find_MeshIndices(strMeshKey))
	{
		if (iIndex >= 0 && iIndex < static_cast<_int>(m_MeshVisibles.size()))
			m_MeshVisibles[iIndex] = bVisible;
	}
}

void CWeapon_Raxasia::Set_Phase(_bool bPhase2)
{
	if (m_MeshVisibles.size() < 3)
		return;

	m_MeshVisibles[1] = (false == bPhase2);
}

#ifdef _DEBUG
void CWeapon_Raxasia::Rebuild_DebugCapsule()
{
	Safe_Release(m_pDebugCapsule);

	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DebugDesc{};
	DebugDesc.fRadius = m_fSweepRadius;
	DebugDesc.fHalfHeight = m_fSweepHalfHeight;
	DebugDesc.iSegment = 10;
	DebugDesc.vColor = _float4{ 1.f, 0.f, 0.f, 1.f };
	m_pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DebugDesc);

	m_fLastDbgRadius = m_fSweepRadius;
	m_fLastDbgHalfHeight = m_fSweepHalfHeight;
}
#endif

CWeapon_Raxasia* CWeapon_Raxasia::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CWeapon_Raxasia* pInstance = new CWeapon_Raxasia(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CWeapon_Raxasia");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CWeapon_Raxasia::Clone(void* pArg)
{
	CWeapon_Raxasia* pInstance = new CWeapon_Raxasia(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWeapon_Raxasia");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CWeapon_Raxasia::Free()
{
	__super::Free();

#ifdef _DEBUG
	Safe_Release(m_pDebugCapsule);
#endif
}
