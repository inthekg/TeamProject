#include "Projectile.h"
#include "GameInstance.h"
#include "Transform.h"
#include "Model.h"
#include "Mesh.h"
#include "RenderStruct.h"
#include "Effect.h"
#include "HitUtil.h"
#include "Player.h"
#include "Character.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif

namespace
{
	static const _wstring	PROJECTILE_MODEL_TAG = TEXT("Prototype_Component_Model_Bullet");

	typedef struct tagModeConfig
	{
		_bool			bGround;
		_bool			bReflect;
		_bool			bHoming;
		_bool			bPierce;
	} MODE_CONFIG;

	static const MODE_CONFIG s_ModeConfig[] =
	{
		{ false, false, false, false },
		{ false, false, false, true  },
		{ false, true,  true,  false },
		{ true,  false, false, true  },
	};
}
CProjectile::CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject{ pDevice, pContext }
{
}

CProjectile::CProjectile(const CProjectile& Prototype)
	: CPoolableObject{ Prototype }
{
}

HRESULT CProjectile::Initialize_Prototype()
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	if (FAILED(Add_Component(ETOUI(LEVEL::GAMEPLAY), Get_ModelProtoTag(),
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	m_pModelCom->Update_Bones();

	Set_InActive();

	return S_OK;
}

HRESULT CProjectile::Initialize(void* pArg)
{
	return S_OK;
}

void CProjectile::Priority_Update(_float fTimeDelta)
{
}

void CProjectile::Update_Standard(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	if (m_fReflectStraight > 0.f)
		m_fReflectStraight -= fTimeDelta;

	if (true == m_bHoming && m_fReflectStraight <= 0.f)
	{
		CGameObject* pHome = (true == m_bReflected) ? m_pOwner : m_pTarget;
		if (nullptr != pHome)
		{
			_vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vTo = XMVectorSubtract(Get_HomePos(pHome), vMy);
			const _float fDistToHome = XMVectorGetX(XMVector3Length(vTo));

			if (fDistToHome > 1e-6f)
			{
				const _vector vDesired = XMVector3Normalize(vTo);

				if (fDistToHome <= m_fFaceDist)
				{
					XMStoreFloat3(&m_vDir, vDesired);
				}
				else
				{
					const _float fFacing = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_vDir), vDesired));
					const _bool bMovingAway = (fDistToHome > m_fPrevHomeDist + 1e-3f);
					if (true == m_bReflected && (fFacing < 0.f || true == bMovingAway))
						XMStoreFloat3(&m_vDir, vDesired);
					else
						Home_Toward(vDesired, (true == m_bReflected) ? m_fReflectHomeRatio : m_fHomeRatio);
				}

				m_fPrevHomeDist = fDistToHome;
			}
		}
	}

	Orient_ToDir();

	m_fSpeed += m_fAccel * fTimeDelta;

	const _vector vDir = XMLoadFloat3(&m_vDir);
	const _vector vPrev = m_pTransformCom->Get_State(STATE::POSITION);
	const _float fStep = m_fSpeed * fTimeDelta;
	_vector vCur = XMVectorAdd(vPrev, XMVectorScale(vDir, fStep));

	if (true == m_bGround)
		vCur = XMVectorSetY(vCur, m_fGroundY);

	HitInfo tHit{};
	tHit.fDamage = m_fDamage;
	tHit.pOtherActor = nullptr;
	tHit.pDamangeCauser = this;
	tHit.eHitType = m_eHitType;

	unordered_set<CGameObject*> Targets;

	if (false == m_bReflected && false == m_bHitConsumed)
	{
		CollisionResult tPlayer{};
		if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPrev, vDir, fStep, FILTER_GROUP::PLAYER, tPlayer))
		{
			_bool bParried = false;
			CGameObject* pHit = tPlayer.pOtherActor;

			if (nullptr != pHit)
			{
				tHit.vHitPosition = tPlayer.tSweepData.vHitPosition;

				const _bool bVolleyOpen = (nullptr == m_pVolleyHit) || (false == *m_pVolleyHit);
				if (true == bVolleyOpen)
				{
					m_bParriedPending = false;

					pHit->Take_Damage(tHit);

					bParried = m_bParriedPending;

					if (false == bParried && nullptr != m_pVolleyHit)
						*m_pVolleyHit = true;
				}

				m_bHitConsumed = true;
			}

			if (true == m_bReflectMode && true == bParried)
			{
				Reflect();
				m_pTransformCom->Set_State(STATE::POSITION, vPrev);
				return;
			}

			if (false == m_bPierce)
			{
				InActivate();
				return;
			}
		}
	}
	else if (true == m_bReflected)
	{
		if (nullptr != m_pOwner)
		{
			const _vector vToOwner = XMVectorSubtract(Get_HomePos(m_pOwner), vPrev);
			if (XMVectorGetX(XMVector3Length(vToOwner)) <= (m_fHitAssistDist + m_fRadius))
			{
				XMStoreFloat3(&tHit.vHitPosition, vPrev);
				m_pOwner->Take_Damage(tHit);
				InActivate();
				return;
			}
		}

		_bool bDamaged = false;
		if (true == CHitUtil::Sweep_Sphere(m_pGameInstance, m_fRadius, vPrev, vCur, tHit, Targets, FILTER_GROUP::MONSTER, &bDamaged))
		{
			InActivate();
			return;
		}
	}

	if (false == m_bGround && false == m_bReflectMode)
	{
		CollisionResult tWall{};
		if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPrev, vDir, fStep, static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT)), tWall, PxQueryFlag::eSTATIC))
		{
			InActivate();
			return;
		}
	}

	m_pTransformCom->Set_State(STATE::POSITION, vCur);

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		if (nullptr == m_pDbg)
		{
			CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
			DbgDesc.fRadius = m_fRadius;
			DbgDesc.fHalfHeight = 0.f;
			DbgDesc.iSegment = 8;
			DbgDesc.vColor = m_bReflected ? _float4{ 1.f, 0.f, 0.f, 1.f } : _float4{ 1.f, 1.f, 0.f, 1.f };
			m_pDbg = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);
		}
		if (nullptr != m_pDbg)
		{
			m_pDbg->Update_Pose(vCur, XMQuaternionIdentity());
			m_pDbg->Late_Update(fTimeDelta);
		}
	}
#endif

	m_fRemain -= fStep;
	if (m_fRemain <= 0.f)
		InActivate();
}

void CProjectile::Update_Missile(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	if (m_fReflectStraight > 0.f)
		m_fReflectStraight -= fTimeDelta;

	if (true == m_bReflected && m_fReflectStraight <= 0.f && nullptr != m_pOwner)
	{
		_vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
		_vector vTo = XMVectorSubtract(Get_HomePos(m_pOwner), vMy);
		const _float fDistToHome = XMVectorGetX(XMVector3Length(vTo));

		if (fDistToHome > 1e-6f)
		{
			const _vector vDesired = XMVector3Normalize(vTo);

			if (fDistToHome <= m_fFaceDist)
			{
				XMStoreFloat3(&m_vDir, vDesired);
			}
			else
			{
				const _float fFacing = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_vDir), vDesired));
				const _bool bMovingAway = (fDistToHome > m_fPrevHomeDist + 1e-3f);
				if (fFacing < 0.f || true == bMovingAway)
					XMStoreFloat3(&m_vDir, vDesired);
				else
					Home_Toward(vDesired, m_fReflectHomeRatio);
			}

			m_fPrevHomeDist = fDistToHome;
		}
	}

	Orient_ToDir();

	m_fSpeed += m_fAccel * fTimeDelta;

	const _vector vDir = XMLoadFloat3(&m_vDir);
	const _vector vPrev = m_pTransformCom->Get_State(STATE::POSITION);
	const _float fStep = m_fSpeed * fTimeDelta;
	const _vector vCur = XMVectorAdd(vPrev, XMVectorScale(vDir, fStep));

	HitInfo tHit{};
	tHit.fDamage = m_fDamage;
	tHit.pOtherActor = nullptr;
	tHit.pDamangeCauser = this;
	tHit.eHitType = m_eHitType;

	if (false == m_bReflected && false == m_bHitConsumed)
	{
		CollisionResult tPlayer{};
		if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPrev, vDir, fStep, FILTER_GROUP::PLAYER, tPlayer))
		{
			_bool bParried = false;
			CGameObject* pHit = tPlayer.pOtherActor;

			if (nullptr != pHit)
			{
				tHit.vHitPosition = tPlayer.tSweepData.vHitPosition;
				m_bParriedPending = false;
				pHit->Take_Damage(tHit);
				bParried = m_bParriedPending;
				m_bHitConsumed = true;
			}

			if (true == bParried)
			{
				Reflect();
				m_pTransformCom->Set_State(STATE::POSITION, vPrev);
				return;
			}

			On_ImpactFx(XMLoadFloat3(&tPlayer.tSweepData.vHitPosition));
			InActivate();
			return;
		}
	}
	else if (true == m_bReflected)
	{
		if (nullptr != m_pOwner)
		{
			const _vector vToOwner = XMVectorSubtract(Get_HomePos(m_pOwner), vPrev);
			if (XMVectorGetX(XMVector3Length(vToOwner)) <= (m_fHitAssistDist + m_fRadius))
			{
				XMStoreFloat3(&tHit.vHitPosition, vPrev);
				m_pOwner->Take_Damage(tHit);
				InActivate();
				return;
			}
		}

		unordered_set<CGameObject*> Targets;
		_bool bDamaged = false;
		if (true == CHitUtil::Sweep_Sphere(m_pGameInstance, m_fRadius, vPrev, vCur, tHit, Targets, FILTER_GROUP::MONSTER, &bDamaged))
		{
			InActivate();
			return;
		}
	}

	CollisionResult tWall{};
	if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPrev, vDir, fStep, static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT)), tWall, PxQueryFlag::eSTATIC))
	{
		On_Explode(XMLoadFloat3(&tWall.tSweepData.vHitPosition));
		InActivate();
		return;
	}

	m_pTransformCom->Set_State(STATE::POSITION, vCur);

	m_fRemain -= fStep;
	if (m_fRemain <= 0.f)
		InActivate();
}

void CProjectile::Physical_Update(_float fFixedTime)
{
}

void CProjectile::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CProjectile::Render()
{
	if (nullptr == m_pModelCom)
		return S_OK;

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
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);

		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.iAlphaClip = 0;
		command.fEmissiveAmount = 0.f;
		command.iRimProfile = 0;
		command.fRimAmount = 0.f;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

void CProjectile::Activate(void* pArg)
{
	__super::Activate(pArg);

	auto pDesc = static_cast<FIRE_DESC*>(pArg);
	if (nullptr == pDesc)
		return;

	m_bParriedPending = false;
	m_bReflected = false;
	m_bHitConsumed = false;
	m_fPrevHomeDist = FLT_MAX;

	Apply_Mode(Get_Mode());

	m_pOwner = pDesc->pOwner;
	m_pTarget = pDesc->pTarget;
	m_vScale = pDesc->vScale;
	XMStoreFloat3(&m_vDir, XMVector3Normalize(XMLoadFloat3(&pDesc->vDir)));
	m_fSpeed = pDesc->fSpeed;
	m_fAccel = pDesc->fAccel;
	m_fRemain = pDesc->fRange;
	m_fRadius = pDesc->fRadius;
	m_fFaceDist = pDesc->fFaceDist;
	m_fReflectStraightTime = pDesc->fReflectStraightTime;
	m_fHitAssistDist = pDesc->fHitAssistDist;
	m_fReflectStraight = 0.f;
	m_fDamage = pDesc->fDamage;
	m_eHitType = pDesc->eHitType;
	m_fGroundY = pDesc->vOrigin.y;
	m_pVolleyHit = pDesc->pVolleyHit;

	m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat3(&pDesc->vOrigin));

	Orient_ToDir();

	Spawn_Trails();
	Start_LoopSound();
}

void CProjectile::InActivate()
{
	Stop_LoopSound();
	Stop_Trails();
	m_pVolleyHit.reset();
	__super::InActivate();
}

void CProjectile::On_Parry()
{
	m_bParriedPending = true;
}

const _wstring& CProjectile::Get_ModelProtoTag() const
{
	return PROJECTILE_MODEL_TAG;
}

void CProjectile::On_ImpactFx(_fvector vPos)
{
}

void CProjectile::On_Explode(_fvector vPos)
{
}

void CProjectile::On_Reflect()
{
}

void CProjectile::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
}

void CProjectile::Apply_Mode(MODE eMode)
{
	m_eMode = eMode;

	const MODE_CONFIG& Config = s_ModeConfig[ETOUI(eMode)];
	m_bGround = Config.bGround;
	m_bReflectMode = Config.bReflect;
	m_bHoming = Config.bHoming;
	m_bPierce = Config.bPierce;

	const char* szLoopCue = Get_LoopCue();
	const char* szHitCue = Get_HitCue();

	m_strLoopCue = (nullptr != szLoopCue) ? szLoopCue : string();
	m_strHitCue = (nullptr != szHitCue) ? szHitCue : string();
}


void CProjectile::Orient_ToDir()
{
	_vector vLook = XMLoadFloat3(&m_vDir);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f)
		return;
	vLook = XMVector3Normalize(vLook);

	_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
	if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	vRight = XMVector3Normalize(vRight);
	_vector vUp = XMVector3Cross(vLook, vRight);

	m_pTransformCom->Set_State(STATE::RIGHT, XMVectorScale(vRight, m_vScale.x));
	m_pTransformCom->Set_State(STATE::UP, XMVectorScale(vUp, m_vScale.y));
	m_pTransformCom->Set_State(STATE::LOOK, XMVectorScale(vLook, m_vScale.z));
}

_vector CProjectile::Get_HomePos(CGameObject* pHome)
{
	_vector vPos = XMVectorZero();
	if (nullptr == pHome)
		return vPos;

	CCharacter* pChar = static_cast<CCharacter*>(pHome);
	if (true == pChar->Get_LockOnWorldPosition(vPos))
		return vPos;

	return pHome->Get_Transform()->Get_State(STATE::POSITION);
}

void CProjectile::Home_Toward(_fvector vDesired, _float fRatio)
{
	_vector vCur = XMLoadFloat3(&m_vDir);

	_float fDot = XMVectorGetX(XMVector3Dot(vCur, vDesired));
	if (fDot > 1.f) fDot = 1.f;
	if (fDot < -1.f) fDot = -1.f;

	const _float fAngle = acosf(fDot);
	if (fAngle < 1e-4f)
	{
		XMStoreFloat3(&m_vDir, vDesired);
		return;
	}

	const _float fSin = sinf(fAngle);
	const _float fA = sinf((1.f - fRatio) * fAngle) / fSin;
	const _float fB = sinf(fRatio * fAngle) / fSin;

	_vector vResult = XMVectorAdd(XMVectorScale(vCur, fA), XMVectorScale(vDesired, fB));
	XMStoreFloat3(&m_vDir, XMVector3Normalize(vResult));
}

void CProjectile::Reflect()
{
	m_bReflected = true;
	m_fRemain = m_fReflectRange;
	m_fReflectStraight = m_fReflectStraightTime;
	m_fPrevHomeDist = FLT_MAX;

	XMStoreFloat3(&m_vDir, XMVectorNegate(XMLoadFloat3(&m_vDir)));

	On_Reflect();
}

void CProjectile::Spawn_Trails()
{
	Stop_Trails();

	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return;

	vector<TRAIL_SPEC> Specs;
	Collect_Trails(Specs);

	for (const TRAIL_SPEC& Spec : Specs)
	{
		if (nullptr == Spec.pKey || nullptr == Spec.pBoneA || nullptr == Spec.pBoneB)
			continue;

		const _float4x4* pBoneA = m_pModelCom->Get_Bone_CombinedMatPtr(Spec.pBoneA);
		const _float4x4* pBoneB = m_pModelCom->Get_Bone_CombinedMatPtr(Spec.pBoneB);
		if (nullptr == pBoneA || nullptr == pBoneB)
			continue;

		CEffect* pTrail = m_pGameInstance->Play_Effect(Spec.pKey, pBoneA, pBoneB, m_pTransformCom->Get_WorldMatrixPtr());
		if (nullptr != pTrail)
			m_Trails.push_back(pTrail);
	}
}

void CProjectile::Stop_Trails()
{
	for (CEffect* pTrail : m_Trails)
	{
		if (nullptr != pTrail)
		{
			pTrail->Stop_Follow();
			pTrail->Stop();
		}
	}
	m_Trails.clear();
}

void CProjectile::Start_LoopSound()
{
	Stop_LoopSound();

	if (true == m_bNoLoopSound || true == m_strLoopCue.empty() || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_hLoopSound = m_pGameInstance->Play_Cue(m_strLoopCue, SOUND_MONSTER, vWorldPos, 1.f, true);
}


void CProjectile::Stop_LoopSound()
{
	if (0 == m_hLoopSound)
		return;

	m_pGameInstance->Stop_SoundSource(m_hLoopSound);
	m_hLoopSound = 0;
}

void CProjectile::Update_LoopSound()
{
	if (0 == m_hLoopSound || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Update_SoundSourcePos(m_hLoopSound, vWorldPos);
}

void CProjectile::Play_HitSound(const _float3& vWorldPos)
{
	if (true == m_bHitSoundPlayed || true == m_strHitCue.empty())
		return;

	m_bHitSoundPlayed = true;
	m_pGameInstance->Play_Cue(m_strHitCue, SOUND_MONSTER, vWorldPos);
}

CGameObject* CProjectile::Clone(void* pArg)
{
	return nullptr;
}

void CProjectile::Free()
{
	Stop_Trails();
#ifdef _DEBUG
	Safe_Release(m_pDbg);
#endif
	Safe_Release(m_pModelCom);

	__super::Free();
}
