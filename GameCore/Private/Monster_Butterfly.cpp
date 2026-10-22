#include "Monster_Butterfly.h"
#include "GameInstance.h"
#include "StatComponent_AI.h"
#include "Navigation.h"
#include "Movement_AI.h"
#include "CCT.h"
#include "DropItem.h"

CMonster_Butterfly::CMonster_Butterfly(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
}

CMonster_Butterfly::CMonster_Butterfly(const CMonster_Butterfly& Prototype)
	: CAIObject{ Prototype }
{
}

HRESULT CMonster_Butterfly::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CMonster_Butterfly::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CHARACTER_SETUP_DESC CharDesc{};
	CharDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_ButterFly");
	CharDesc.strAnimFolderTag = TEXT("ButterFly");
	CharDesc.strAnimTag = TEXT("ButterFly");
	CharDesc.pRootBoneName = "Root";
	CharDesc.bUseCCT = true;
	CharDesc.fCCTRadius = 0.5f;
	CharDesc.fCCTHeight = 1.2f;
	CharDesc.fRunSpeed = 2.5f;
	CharDesc.fSprintSpeed = 4.f;

	if (nullptr != pArg)
		CharDesc.iModelLevel = static_cast<AIOBJECT_DESC*>(pArg)->iProtoLevel;
	else
		CharDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	// 이동 이펙트와 같은 복부 본을 라이트 추적 위치로 사용한다.
	m_pButterflyLightBoneMatrix = Get_Bone_CombinedMatPtr("Bn_Abdomen_004");

	if (FAILED(Ready_ButterflyLight()))
		return E_FAIL;

	m_EffectKeys[EFFECT_KEY::MOVE] = Hash_String(L"LOOP_BFLY_MOVE");

	Set_EmissiveAmount(3.f);

	Spawn();

	return S_OK;
}

HRESULT CMonster_Butterfly::Ready_NotifyKeys()
{
	m_NotifyKeys[Hash_String(L"HitOn_Weapon")] = NOTIFY_KEY::WEAPON_ON;
	m_NotifyKeys[Hash_String(L"HitOff_Weapon")] = NOTIFY_KEY::WEAPON_OFF;
	m_NotifyKeys[Hash_String(L"SET_DEAD")] = NOTIFY_KEY::SET_DEAD;

	return S_OK;
}

HRESULT CMonster_Butterfly::Ready_ButterflyLight()
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	// 풀 객체를 최초 생성할 때 포인트 라이트를 한 번만 등록한다.
	m_iButterflyLightIndex = m_pGameInstance->Get_LightCount(LIGHT::POINT);
	m_bButterflyLightEnable = false;

	m_ButterflyLightDesc = {};
	m_ButterflyLightDesc.eType = LIGHT::POINT;
	m_ButterflyLightDesc.vDiffuse = _float4(1.f * BUTTERFLY_LIGHT_INTENSITY, 0.08f * BUTTERFLY_LIGHT_INTENSITY, 0.04f * BUTTERFLY_LIGHT_INTENSITY, 1.f);
	m_ButterflyLightDesc.vAmbient = _float4(0.03f, 0.f, 0.f, 0.05f);
	m_ButterflyLightDesc.vSpecular = _float4(0.5f * BUTTERFLY_LIGHT_INTENSITY, 0.04f * BUTTERFLY_LIGHT_INTENSITY, 0.02f * BUTTERFLY_LIGHT_INTENSITY, 1.f);
	m_ButterflyLightDesc.vDirection = _float4(0.f, -1.f, 0.f, 0.f);
	m_ButterflyLightDesc.vPosition = _float4(0.f, 0.f, 0.f, 1.f);
	m_ButterflyLightDesc.fRange = BUTTERFLY_LIGHT_RANGE;
	m_ButterflyLightDesc.fInnerCos = 0.f;
	m_ButterflyLightDesc.fOuterCos = 0.f;
	m_ButterflyLightDesc.bShadow = false;
	m_ButterflyLightDesc.bGodRay = false;
	m_ButterflyLightDesc.fGodRayIntensity = 0.f;

	if (FAILED(m_pGameInstance->Add_Light(LIGHT::POINT, m_ButterflyLightDesc)))
	{
		m_iButterflyLightIndex = UINT_MAX;
		return E_FAIL;
	}

	// GHOST 상태로 시작하므로 등록 직후에는 라이트를 끈다.
	if (FAILED(m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iButterflyLightIndex, false)))
		return E_FAIL;

	return S_OK;
}

void CMonster_Butterfly::Change_State(BFLY_STATE eNextState)
{
	m_eState = eNextState;

	// 등장 이후의 생존 상태에서만 켜고 GHOST, DEAD, DESPAWN에서는 즉시 끈다.
	const _bool bLightEnable = BFLY_STATE::GHOST != eNextState && BFLY_STATE::DEAD != eNextState && BFLY_STATE::DESPAWN != eNextState;
	Set_ButterflyLightEnable(bLightEnable);
}

void CMonster_Butterfly::Set_ButterflyLightEnable(_bool bEnable)
{
	if (nullptr == m_pGameInstance || UINT_MAX == m_iButterflyLightIndex)
		return;

	if (m_bButterflyLightEnable == bEnable)
		return;

	// 켜기 전에 현재 위치를 먼저 반영해 월드 원점에서 번쩍이지 않게 한다.
	if (true == bEnable)
		Update_ButterflyLight();

	if (SUCCEEDED(m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iButterflyLightIndex, bEnable)))
		m_bButterflyLightEnable = bEnable;
}

void CMonster_Butterfly::Update_ButterflyLight()
{
	if (nullptr == m_pGameInstance || nullptr == m_pTransformCom || UINT_MAX == m_iButterflyLightIndex)
		return;

	_vector vLightPosition = m_pTransformCom->Get_State(STATE::POSITION);

	if (nullptr != m_pButterflyLightBoneMatrix)
	{
		// 복부 본의 로컬 행렬과 나비 월드 행렬을 결합해 실제 발광 위치를 얻는다.
		const _matrix LightWorldMatrix = XMLoadFloat4x4(m_pButterflyLightBoneMatrix) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
		vLightPosition = LightWorldMatrix.r[3];
	}
	else
	{
		// 복부 본을 찾지 못하면 Transform보다 약간 위쪽을 사용한다.
		vLightPosition += XMVectorSet(0.f, BUTTERFLY_LIGHT_FALLBACK_Y, 0.f, 0.f);
	}

	XMStoreFloat4(&m_ButterflyLightDesc.vPosition, XMVectorSetW(vLightPosition, 1.f));
	m_pGameInstance->Set_LightDesc(LIGHT::POINT, m_iButterflyLightIndex, m_ButterflyLightDesc);
}

void CMonster_Butterfly::Update(_float fTimeDelta)
{
	if (m_fHudHitTimer > 0.f)
		m_fHudHitTimer -= fTimeDelta;

	if (true == Is_AISleep())
		return;

	if (true == m_bFrozen)
	{
		Drive_Animator(fTimeDelta);
		return;
	}

	if (false == m_bPatrolCollected)
	{
		Collect_PatrolCells();
		if (false == m_PatrolCells.empty())
			m_bPatrolCollected = true;
	}

	m_pStatCom->Update_StatComponent(fTimeDelta);

	Tick_Butterfly(fTimeDelta);

	Drive_MoveEffect();

	Apply_Movment(fTimeDelta);

	Drive_Animator(fTimeDelta);
}

void CMonster_Butterfly::Tick_Butterfly(_float fTimeDelta)
{
	if (m_fAttackCool > 0.f)
		m_fAttackCool -= fTimeDelta;

	if (true == Is_Dead() && BFLY_STATE::DEAD != m_eState && BFLY_STATE::DESPAWN != m_eState)
	{
		// 사망을 감지한 프레임에 라이트부터 즉시 끈다.
		Change_State(BFLY_STATE::DEAD);

		Set_AnimFloat(L"Speed", 0.f);
		Set_AnimBool(L"Dead", true);
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
		Request_AnimAction(L"DEAD");
		return;
	}

	switch (m_eState)
	{
	case BFLY_STATE::GHOST:
	{
		_float fDist = 0.f;
		if (true == Get_DistToTarget(fDist))
		{
			if (false == m_bAlerted && fDist <= m_fAlertRange)
			{
				if (false == m_strAlertSound.empty() && nullptr != m_pTransformCom)
				{
					_float3 vPos;
					XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
					m_pGameInstance->Play_SoundAt(m_strAlertSound, CHANNEL_ID::SOUND_MONSTER, vPos);
				}
				m_bAlerted = true;
			}

			if (fDist <= m_fDetectRange)
			{
				if (nullptr != m_pCCT)
					m_pCCT->Set_Active(true);

				Capture_SpawnPos();
				Set_AnimInt(L"SpawnNumber", 1);
				Request_AnimAction(L"SPAWN");
				Change_State(BFLY_STATE::SPAWN);
			}
		}
		break;
	}
	case BFLY_STATE::SPAWN:
	{
		if (true == Is_AnimationFinished())
		{
			m_fLifeTimer = 0.f;
			m_bHasMoveTarget = false;
			Change_State(BFLY_STATE::PATROL);
		}
		break;
	}
	case BFLY_STATE::PATROL:
	{
		m_fLifeTimer += fTimeDelta;
		if (m_fLifeTimer >= m_fLifeMax)
		{
			Change_State(BFLY_STATE::DESPAWN);
			break;
		}

		_float fDist = 0.f;
		if (true == Get_DistToTarget(fDist) && fDist <= m_fAttackRange && m_fAttackCool <= 0.f)
		{
			Set_AnimFloat(L"Speed", 0.f);
			Set_AnimInt(L"PatternID", 1);
			Set_AnimTrigger(L"PatternStart");
			Request_AnimAction(L"ATTACK");
			m_bHasMoveTarget = false;
			Change_State(BFLY_STATE::ATTACK);
			break;
		}

		Drive_Move(fTimeDelta, false);
		break;
	}
	case BFLY_STATE::ATTACK:
	{
		if (true == Is_AnimationFinished())
		{
			m_fAttackCool = m_fAttackCooldown;
			m_HitTargets.clear();
			m_bHasMoveTarget = false;
			Change_State(BFLY_STATE::PATROL);
		}
		break;
	}
	case BFLY_STATE::HIT:
	{
		if (false == m_bHitEntered)
		{
			Set_AnimFloat(L"Speed", 0.f);
			Set_AnimInt(L"Direction", m_iHitDir);
			Set_AnimInt(L"HitInfo", 0);
			Set_AnimTrigger(L"DamageTrigger");
			Request_AnimAction(L"DAMAGE");

			if (nullptr != m_pMovementCom && nullptr != m_pTransformCom)
			{
				_vector vAway = XMVectorSubtract(m_pTransformCom->Get_State(STATE::POSITION), XMLoadFloat3(&m_vFleeFrom));
				m_pMovementCom->Begin_KnockBack(vAway, m_fHitKnockDist, m_fHitKnockTime, EASE_TYPE::LINEAR);
			}

			m_bHitEntered = true;
			break;
		}

		if (true == Is_AnimationFinished())
			Change_State(BFLY_STATE::FLEE);
		break;
	}
	case BFLY_STATE::FLEE:
	{
		m_fLifeTimer += fTimeDelta;
		if (m_fLifeTimer >= m_fLifeMax)
		{
			Change_State(BFLY_STATE::DESPAWN);
			break;
		}

		Drive_Move(fTimeDelta, true);

		if (false == m_bHasMoveTarget)
			Change_State(BFLY_STATE::PATROL);
		break;
	}
	case BFLY_STATE::DESPAWN:
	{
		Request_Despawn();
		break;
	}
	case BFLY_STATE::DEAD:
	{
		if (true == m_bDeadPending)
		{
			m_fDeadTimer -= fTimeDelta;
			if (m_fDeadTimer <= 0.f)
			{
				m_bDeadPending = false;
				Spawn_DropItem();
				Request_Despawn();
			}
		}
		break;
	}
	default:
		break;
	}
}

void CMonster_Butterfly::Do_SphereAttack()
{
	if (nullptr == m_pTransformCom)
		return;

	HitInfo tHit{};
	tHit.fDamage = m_fAttackDamage;
	tHit.eHitType = HIT_TYPE::KnockBack;
	tHit.fGroggy = 0.f;

	_vector vCenter = m_pTransformCom->Get_State(STATE::POSITION);
	Hit_OverlapSphere(m_fAttackRadius, vCenter, tHit, m_HitTargets, FILTER_GROUP::PLAYER);
}

void CMonster_Butterfly::Collect_PatrolCells()
{
	m_PatrolCells.clear();

	if (nullptr == m_pNavigation || nullptr == m_pTransformCom)
		return;

	_float3 vPos;
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

	_int iStartCell = -1;
	if (true == m_pNavigation->Set_CurrentCell(vPos, &iStartCell) && iStartCell >= 0)
		m_pNavigation->Collect_CellsByNeighbor(iStartCell, m_iPatrolDepth, &m_PatrolCells);
}

void CMonster_Butterfly::Drive_Move(_float fTimeDelta, _bool bFlee)
{
	if (true == m_PatrolCells.empty() || nullptr == m_pMovementCom || nullptr == m_pNavigation)
		return;

	_bool bJustPicked = false;

	if (false == m_bHasMoveTarget)
	{
		const _int iCell = bFlee ? Pick_FleeCell(m_vFleeFrom) : Pick_PatrolCell();
		if (iCell >= 0 && true == m_pNavigation->Get_CellCenter(static_cast<_uint>(iCell), &m_vMoveTarget))
		{
			m_bHasMoveTarget = true;
			bJustPicked = true;
		}
	}

	if (true == m_bHasMoveTarget)
	{
		m_pMovementCom->Set_SpeedType(bFlee ? SPEED_TYPE::SPRINT : SPEED_TYPE::RUN);
		m_pMovementCom->MoveTo_Navi(XMLoadFloat3(&m_vMoveTarget), fTimeDelta);

		Set_AnimFloat(L"Speed", bFlee ? 0.8f : 0.5f);
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));

		if (false == bJustPicked
			&& true == m_pMovementCom->Is_Reached(XMLoadFloat3(&m_vMoveTarget), m_pMovementCom->Get_ReachRadius()))
			m_bHasMoveTarget = false;
	}
}

void CMonster_Butterfly::Drive_MoveEffect()
{
	const _bool bWantOn = (BFLY_STATE::PATROL == m_eState || BFLY_STATE::FLEE == m_eState);

	if (bWantOn == m_bMoveFxOn)
		return;

	if (true == bWantOn)
		Start_LoopEffect(m_EffectKeys[EFFECT_KEY::MOVE], "Effect_ButterFly_Move", "Bn_Abdomen_004");
	else
		Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::MOVE]);

	m_bMoveFxOn = bWantOn;
}

_int CMonster_Butterfly::Pick_PatrolCell() const
{
	if (true == m_PatrolCells.empty())
		return -1;

	return static_cast<_int>(m_PatrolCells[rand() % m_PatrolCells.size()]);
}

_int CMonster_Butterfly::Pick_FleeCell(const _float3& vFrom) const
{
	_int iBest = -1;
	_float fBest = -1.f;

	_vector vF = XMLoadFloat3(&vFrom);

	for (_uint iCell : m_PatrolCells)
	{
		_float3 vCenter;
		if (false == m_pNavigation->Get_CellCenter(iCell, &vCenter))
			continue;

		_float fDistSq = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(XMLoadFloat3(&vCenter), vF)));
		if (fDistSq > fBest)
		{
			fBest = fDistSq;
			iBest = static_cast<_int>(iCell);
		}
	}

	return iBest;
}

void CMonster_Butterfly::Action_Notify(_uint64 iHashKey)
{
	auto it = m_NotifyKeys.find(iHashKey);
	if (it == m_NotifyKeys.end())
		return;

	switch (it->second)
	{
	case NOTIFY_KEY::WEAPON_ON:
		Do_SphereAttack();
		break;
	case NOTIFY_KEY::WEAPON_OFF:
		m_HitTargets.clear();
		break;
	case NOTIFY_KEY::SET_DEAD:
		m_bDeadPending = true;
		m_fDeadTimer = m_fDeadDelay;
		break;
	default:
		break;
	}
}

_bool CMonster_Butterfly::Take_Damage(const HitInfo& hitInfo)
{
	if (BFLY_STATE::GHOST == m_eState || BFLY_STATE::DEAD == m_eState || BFLY_STATE::DESPAWN == m_eState)
		return false;

	_bool bRet = __super::Take_Damage(hitInfo);

	if (true == Is_Dead())
		return bRet;

	m_fAttackCool = m_fAttackCooldown;

	_vector vTargetPos;
	if (true == Get_TargetPos(vTargetPos))
		XMStoreFloat3(&m_vFleeFrom, vTargetPos);
	else
		m_vFleeFrom = hitInfo.vHitPosition;

	Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));

	m_bHasMoveTarget = false;
	m_bHitEntered = false;
	Change_State(BFLY_STATE::HIT);

	return bRet;
}

void CMonster_Butterfly::Spawn()
{
	__super::Spawn();

	// 최초 생성과 풀 재사용 모두 GHOST 상태로 시작하며 라이트를 끈다.
	Change_State(BFLY_STATE::GHOST);
	m_fLifeTimer = 0.f;
	m_fAttackCool = 0.f;
	m_bAlerted = false;
	m_bHasMoveTarget = false;
	m_bPatrolCollected = false;
	m_HitTargets.clear();
	m_PatrolCells.clear();

	Stop_AllLoopEffects();
	m_bMoveFxOn = false;

	m_bDeadPending = false;
	m_fDeadTimer = 0.f;

	// 라이트 인덱스는 생성 때 확보한 리소스 핸들이라 건드리지 않는다. 켜짐 여부는 Despawn이 이미 끈다.
	m_bHitEntered = false;
	m_vFleeFrom = {};

	if (nullptr != m_pCCT)
		m_pCCT->Set_Active(false);
}

void CMonster_Butterfly::Despawn()
{
	// 풀 반납과 비활성화 경로에서도 라이트가 남지 않게 보장한다.
	Set_ButterflyLightEnable(false);
	__super::Despawn();
}

void CMonster_Butterfly::On_SetDead()
{
	Request_Despawn();
}

void CMonster_Butterfly::Spawn_DropItem()
{
	if (nullptr == m_pGameInstance || nullptr == m_pTransformCom)
		return;

	_float3 vPosition;
	XMStoreFloat3(&vPosition, m_pTransformCom->Get_State(STATE::POSITION));

	CDropItem::DROP_ITEM_DESC Desc{};
	Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	Desc.vTranslate = _float4(vPosition.x, vPosition.y, vPosition.z, 1.f);
	Desc.eItemType = ITEM_TYPE::ERGO;
	Desc.strItemName = TEXT("Ergo");
	Desc.iItemCount = 1;
	Desc.iErgoMinAmount = 100;
	Desc.iErgoMaxAmount = 200;
	Desc.vTriggerHalfExtents = _float3(2.f, 2.f, 2.f);

	CDropItem* pItem = m_pGameInstance->Clone_Prototype<CDropItem>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_DropItem"), &Desc);
	if (nullptr == pItem)
		return;

	if (FAILED(m_pGameInstance->Add_GameObject(static_cast<_uint>(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_MapObject"), pItem)))
		Safe_Release(pItem);
}

_bool CMonster_Butterfly::Can_FatalFront() const
{
	return false;
}

_bool CMonster_Butterfly::Can_FatalBack() const
{
	return false;
}

_bool CMonster_Butterfly::Is_Lockable() const
{
	return BFLY_STATE::GHOST != m_eState
		&& BFLY_STATE::DESPAWN != m_eState
		&& BFLY_STATE::DEAD != m_eState;
}

_bool CMonster_Butterfly::Get_DistToTarget(_float& fOut) const
{
	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return false;

	if (nullptr == m_pTransformCom)
		return false;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	fOut = XMVectorGetX(XMVector3Length(XMVectorSubtract(vTargetPos, vMyPos)));
	return true;
}

void CMonster_Butterfly::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Butterfly::Late_Update(_float fTimeDelta)
{
	if (BFLY_STATE::GHOST == m_eState)
		return;

	// 이동과 애니메이션이 반영된 복부 본 위치를 최종 라이트 위치로 사용한다.
	if (true == m_bButterflyLightEnable)
		Update_ButterflyLight();

	__super::Late_Update(fTimeDelta);
}

HRESULT CMonster_Butterfly::Render()
{
	return __super::Render();
}

CMonster_Butterfly* CMonster_Butterfly::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Butterfly* pInstance = new CMonster_Butterfly(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Butterfly");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Butterfly::Clone(void* pArg)
{
	CMonster_Butterfly* pInstance = new CMonster_Butterfly(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Butterfly");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Butterfly::Free()
{
	// Light 배열의 다른 인덱스가 밀리지 않도록 제거하지 않고 비활성화한다.
	Set_ButterflyLightEnable(false);
	m_iButterflyLightIndex = UINT_MAX;
	m_pButterflyLightBoneMatrix = nullptr;

	__super::Free();
}
