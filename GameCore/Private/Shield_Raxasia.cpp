#include "Shield_Raxasia.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "Model.h"
#include "Mesh.h"
#include "AIObject.h"
#include "BT_BlackBoard.h"
#include "HitUtil.h"
#include "DynamicRigidBody.h"
#include "Monster_Include.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif

#ifdef _DEBUG
BEGIN_REFLECTION(CShield_Raxasia, CWeapon_Monster)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fSweepRadius, Float, Set_SweepRadius, _float)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fSweepHalfHeight, Float, Set_SweepHeight, _float)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fBlockedRatio, Float, Set_BlockedRatio, _float)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fColHalfX, Float, Set_ColliderHalfX, _float)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fColHalfY, Float, Set_ColliderHalfY, _float)
ADD_PROPERTY_SETTER(CShield_Raxasia, m_fColHalfZ, Float, Set_ColliderHalfZ, _float)
ADD_PROPERTY(CShield_Raxasia, m_fShieldHP, Float)
END_REFLECTION(CShield_Raxasia)
#endif


CShield_Raxasia::CShield_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CWeapon_Monster{ pDevice, pContext }
{
}

CShield_Raxasia::CShield_Raxasia(const CShield_Raxasia& Prototype)
	: CWeapon_Monster{ Prototype }
{
}

HRESULT CShield_Raxasia::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CShield_Raxasia::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	auto pDesc = static_cast<SHIELD_DESC*>(pArg);
	m_fSweepRadius = pDesc->fSweepRadius;
	m_fSweepHalfHeight = pDesc->fSweepHalfHeight;
	m_fShieldMaxHP = pDesc->fShieldHP;
	m_fShieldHP = pDesc->fShieldHP;
	m_fBlockedRatio = pDesc->fBlockedRatio;
	m_fColHalfX = pDesc->vColliderHalf.x;
	m_fColHalfY = pDesc->vColliderHalf.y;
	m_fColHalfZ = pDesc->vColliderHalf.z;

	if (false == pDesc->strBashTopBoneName.empty())
		m_iBashTopBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strBashTopBoneName.c_str());
	if (false == pDesc->strBashBottomBoneName.empty())
		m_iBashBottomBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strBashBottomBoneName.c_str());

	m_MeshVisibles.assign(m_pModelCom->Get_RenderUnits().size(), true);

	if (FAILED(Ready_RigidBody(*pDesc)))
		return E_FAIL;

	Set_Broke(false);

#ifdef _DEBUG
	Rebuild_DebugCapsule();
#endif

	return S_OK;
}

void CShield_Raxasia::Priority_Update(_float fTimeDelta)
{
}

void CShield_Raxasia::Update(_float fTimeDelta)
{
}

void CShield_Raxasia::Physical_Update(_float fFixedTime)
{
}

void CShield_Raxasia::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

	if (nullptr != m_pRigidBody)
	{
		_vector vScale, vQuat, vPos;
		if (XMMatrixDecompose(&vScale, &vQuat, &vPos, XMLoadFloat4x4(&m_CombinedWorldMatrix)))
		{
			if (false == XMVector3IsNaN(vPos) && false == XMVector3IsInfinite(vPos) &&
				false == XMVector4IsNaN(vQuat) && false == XMVector4IsInfinite(vQuat))
			{
				m_pRigidBody->Set_GlobalTransform(vPos, XMQuaternionNormalize(vQuat));
			}
		}
	}

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		if (m_fSweepRadius != m_fLastDbgRadius || m_fSweepHalfHeight != m_fLastDbgHalfHeight)
			Rebuild_DebugCapsule();

		_matrix BashTop, BashBot;
		if (nullptr != m_pDebugCapsule && SUCCEEDED(Get_BashMatrix(BashTop, BashBot)))
		{
			const _vector vTop = BashTop.r[3];
			const _vector vBot = BashBot.r[3];
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


HRESULT CShield_Raxasia::Render()
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

void CShield_Raxasia::Attack_Begin(_float fDamage, HIT_TYPE eType)
{
	m_eHitType = eType;
	m_bCollisionActive = true;
	m_bPrevCenterValid = false;
	m_bHitSuccess = false;
	m_CollisionTargets.clear();
	if (fDamage >= 0.f)
		m_fDamage = fDamage;
}

void CShield_Raxasia::Attack_End()
{
	m_bCollisionActive = false;
}

void CShield_Raxasia::Update_Attack(_float fTimeDelta)
{
	if (false == m_bCollisionActive)
	{
		m_bPrevCenterValid = false;
		return;
	}

	_matrix BashTop, BashBot;
	if (FAILED(Get_BashMatrix(BashTop, BashBot)))
		return;

	const _vector vTop = BashTop.r[3];
	const _vector vBot = BashBot.r[3];
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

	HitInfo tHit;
	tHit.fDamage = m_fDamage;
	tHit.pOtherActor = m_pOwner;
	tHit.pDamangeCauser = this;
	tHit.eHitType = m_eHitType;

	_bool bDamaged = false;

	if (false == m_bPrevCenterValid)
	{
		XMStoreFloat3(&m_vPrevCenter, vCenter);
		m_bPrevCenterValid = true;
		CHitUtil::Overlap_Sphere(m_pGameInstance, m_fSweepRadius + m_fSweepHalfHeight, vCenter,
			tHit, m_CollisionTargets, m_eTargetGroup, &bDamaged);
		if (true == bDamaged)
			m_bHitSuccess = true;
		return;
	}

	const _vector vPrev = XMLoadFloat3(&m_vPrevCenter);
	XMStoreFloat3(&m_vPrevCenter, vCenter);

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

_bool CShield_Raxasia::Take_Damage(const HitInfo& hitInfo)
{
	if (true == m_bBroken)
		return true;

	m_fShieldHP -= hitInfo.fDamage;

	if (nullptr != m_pOwner)
		m_pOwner->Apply_ShieldBlock(hitInfo.fDamage * m_fBlockedRatio);

	if (m_fShieldHP > 0.f)
		return true;

	m_fShieldHP = 0.f;
	m_bBroken = true;

	Set_Physics_InScene(false);

	if (nullptr != m_pOwner && nullptr != m_pOwner->Get_BlackBoard())
		m_pOwner->Get_BlackBoard()->Set<_bool>(L"bShieldBroken", true);

	if (nullptr != m_pOwner)
		m_pOwner->On_ShieldBroken();

	return true;
}

HRESULT CShield_Raxasia::Get_BashMatrix(_matrix& OutTop, _matrix& OutBottom)
{
	if (m_iBashTopBoneIndex == -1 || m_iBashBottomBoneIndex == -1)
		return E_FAIL;

	if (FAILED(Get_BoneWorldMatrix(m_iBashTopBoneIndex, OutTop)))
		return E_FAIL;
	if (FAILED(Get_BoneWorldMatrix(m_iBashBottomBoneIndex, OutBottom)))
		return E_FAIL;

	return S_OK;
}

void CShield_Raxasia::Set_Physics_InScene(_bool bIn)
{
	if (nullptr == m_pRigidBody) return;
	PxRigidActor* pActor = m_pRigidBody->Get_RigidActor();
	if (nullptr == pActor) return;
	if (true == bIn) { if (nullptr == pActor->getScene()) m_pGameInstance->Add_ActorToScene(pActor); }
	else { if (nullptr != pActor->getScene()) m_pGameInstance->Remove_ActorFromScene(pActor); }
}

void CShield_Raxasia::Set_Broke(_bool bBroke)
{
	m_bBrokeMesh = bBroke;

	if (nullptr == m_pModelCom)
		return;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	for (_uint i = 0; i < ETOUI(Units.size()); ++i)
	{
		if (nullptr == Units[i].pMesh || i >= m_MeshVisibles.size())
			continue;

		const _bool bBrokenMesh = (Units[i].pMesh->Get_Name().find(L"Broken") != _wstring::npos);
		m_MeshVisibles[i] = (bBroke == bBrokenMesh);
	}
}

void CShield_Raxasia::Reset_Shield()
{
	m_fShieldHP = m_fShieldMaxHP;
	m_bBroken = false;

	m_CollisionTargets.clear();
	m_bCollisionActive = false;
	m_bPrevCenterValid = false;

	Set_Broke(false);
	Set_Physics_InScene(true);

	if (nullptr != m_pOwner && nullptr != m_pOwner->Get_BlackBoard())
		m_pOwner->Get_BlackBoard()->Set<_bool>(L"bShieldBroken", false);
}


HRESULT CShield_Raxasia::Ready_RigidBody(const SHIELD_DESC& Desc)
{
	m_pRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);
	if (nullptr == m_pRigidBody)
		return S_OK;

	PHYSIC_DESC pd{};
	pd.pOwner = this;
	pd.vInitPos = _float3{ 0.f, 0.f, 0.f };
	pd.vInitQuat = _float4{ 0.f, 0.f, 0.f, 1.f };
	pd.bIsKinematic = true;
	pd.fMass = 1.f;

	SHAPE_DESC shape{};
	shape.eType = GEOMETRY_TYPE::BOX;
	shape.vHalfExtents = _float3{ m_fColHalfX, m_fColHalfY, m_fColHalfZ };
	shape.bTrigger = false;
	pd.Shapes.push_back(shape);

	pd.iFilterWord0 = ETOUI(FILTER_GROUP::MONSTEROBJECT);
	pd.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::WEAPON);

	if (FAILED(m_pRigidBody->Initialize(&pd)))
	{
		Safe_Release(m_pRigidBody);
		return S_OK;
	}

	return S_OK;
}

vector<_int> CShield_Raxasia::Find_MeshIndices(const _wstring& strMeshKey) const
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

#ifdef _DEBUG
HRESULT CShield_Raxasia::Set_ColliderHalfX(_float fValue)
{
	m_fColHalfX = fValue;
	Resize_Collider();
	return S_OK;
}

HRESULT CShield_Raxasia::Set_ColliderHalfY(_float fValue)
{
	m_fColHalfY = fValue;
	Resize_Collider();
	return S_OK;
}

HRESULT CShield_Raxasia::Set_ColliderHalfZ(_float fValue)
{
	m_fColHalfZ = fValue;
	Resize_Collider();
	return S_OK;
}

void CShield_Raxasia::Resize_Collider()
{
	if (nullptr == m_pRigidBody)
		return;

	PxRigidActor* pActor = m_pRigidBody->Get_RigidActor();
	if (nullptr == pActor)
		return;

	PxShape* pShape = nullptr;
	if (pActor->getShapes(&pShape, 1) > 0 && nullptr != pShape)
		pShape->setGeometry(PxBoxGeometry(m_fColHalfX, m_fColHalfY, m_fColHalfZ));
}

void CShield_Raxasia::Rebuild_DebugCapsule()
{
	Safe_Release(m_pDebugCapsule);

	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DebugDesc{};
	DebugDesc.fRadius = m_fSweepRadius;
	DebugDesc.fHalfHeight = m_fSweepHalfHeight;
	DebugDesc.iSegment = 10;
	DebugDesc.vColor = _float4{ 0.f, 0.4f, 1.f, 1.f };
	m_pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DebugDesc);

	m_fLastDbgRadius = m_fSweepRadius;
	m_fLastDbgHalfHeight = m_fSweepHalfHeight;
}
#endif

CShield_Raxasia* CShield_Raxasia::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CShield_Raxasia* pInstance = new CShield_Raxasia(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CShield_Raxasia");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CShield_Raxasia::Clone(void* pArg)
{
	CShield_Raxasia* pInstance = new CShield_Raxasia(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CShield_Raxasia");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CShield_Raxasia::Free()
{
	__super::Free();

	Safe_Release(m_pRigidBody);

#ifdef _DEBUG
	Safe_Release(m_pDebugCapsule);
#endif
}
