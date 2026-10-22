#include "Boss_Festival.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"

#include "Model.h"
#include "Mesh.h"
#include "Weapon_Melee.h"
#include "Animator.h"
#include "Monster_Defines.h"
#include "Monster_Include.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CBoss_Festival, CAIObject)
ADD_PROPERTY_SETTER(CBoss_Festival, m_iDbgHitIdx, Int, Set_DbgHitIdx, _int)
ADD_PROPERTY_SETTER(CBoss_Festival, m_fDbgHitRadius, Float, Set_DbgHitRadius, _float)
ADD_PROPERTY_SETTER(CBoss_Festival, m_fDbgHitHalf, Float, Set_DbgHitHalf, _float)
END_REFLECTION(CBoss_Festival)
#endif

#ifdef _DEBUG
namespace
{
	const _int		DBG_HITBOX_COUNT = 5;
	const _int		DBG_PATTERN_COUNT = 7;

	const _wstring	g_DbgHitIds[DBG_HITBOX_COUNT] = { L"HandL", L"HandR", L"Body", L"Weapon", L"Basket" };
	const _tchar* g_DbgHitTags[DBG_HITBOX_COUNT] = { L"HandL", L"HandR", L"Body", L"Weapon", L"Basket" };
}
#endif


namespace
{
	const PATTERN_DEF s_P1[] = {
		{ 1, ATK_GROUP::SHORT, 1.f,  5.0f }, { 2, ATK_GROUP::SHORT, 1.f,  4.0f },
		{ 3, ATK_GROUP::LONG,  1.f,  6.2f }, { 4, ATK_GROUP::LONG,  0.5f, 7.0f },
		{ 5, ATK_GROUP::SHORT, 1.f,  6.2f }, { 6, ATK_GROUP::LONG,  1.f,  3.5f },
		{ 7, ATK_GROUP::SHORT, 1.f,  4.3f },
	};
	const PATTERN_DEF s_P2[] = {
		{ 1, ATK_GROUP::LONG, 1.f,  4.8f }, { 2, ATK_GROUP::LONG, 1.f,  8.0f },
		{ 3, ATK_GROUP::LONG, 1.f,  6.4f }, { 4, ATK_GROUP::LONG, 0.5f, 4.0f },
		{ 5, ATK_GROUP::LONG, 1.f,  6.4f }, { 6, ATK_GROUP::LONG, 1.f,  4.5f },
	};

	const PATTERN_DEF* Pattern_Table(_int iPhase)
	{
		return (2 == iPhase) ? s_P2 : s_P1;
	}

	_int Pattern_Count(_int iPhase)
	{
		return (2 == iPhase)
			? static_cast<_int>(sizeof(s_P2) / sizeof(PATTERN_DEF))
			: static_cast<_int>(sizeof(s_P1) / sizeof(PATTERN_DEF));
	}

	_float Pattern_Length(_int iPhase, _int iID)
	{
		const PATTERN_DEF* t = Pattern_Table(iPhase);
		const _int n = Pattern_Count(iPhase);
		for (_int i = 0; i < n; ++i)
			if (t[i].iID == iID) return t[i].fAttackLength;
		return 0.f;
	}

	_float Pattern_MaxLength(_int iPhase)
	{
		const PATTERN_DEF* t = Pattern_Table(iPhase);
		const _int n = Pattern_Count(iPhase);
		_float fMax = 0.f;
		for (_int i = 0; i < n; ++i)
			if (t[i].fAttackLength > fMax) fMax = t[i].fAttackLength;
		return fMax;
	}
}

CBoss_Festival::CBoss_Festival(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
	//추가 
	m_eActorType = ACTOR_TYPE::BOSS;
	m_strMonsterName = L"축제 인도자";
}

CBoss_Festival::CBoss_Festival(const CBoss_Festival& Prototype)
	: CAIObject{ Prototype }
	, m_EffectKeys{ Prototype.m_EffectKeys }
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBoss_Festival::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_FestivalLeader.json";

	m_NotifyKeys[Hash_String(TEXT("HeadIn"))] = NOTIFY_KEY::HEAD_IN;
	m_NotifyKeys[Hash_String(TEXT("HeadOut"))] = NOTIFY_KEY::HEAD_OUT;
	m_NotifyKeys[Hash_String(TEXT("SET_DEAD"))] = NOTIFY_KEY::SET_DEAD;
	m_NotifyKeys[Hash_String(TEXT("FX_SLIDEON"))] = NOTIFY_KEY::TRAIL_ON;
	m_NotifyKeys[Hash_String(TEXT("FX_SLIDEOFF"))] = NOTIFY_KEY::TRAIL_OFF;
	m_NotifyKeys[Hash_String(TEXT("Symbol_Start"))] = NOTIFY_KEY::FATAL_READY;
	m_NotifyKeys[Hash_String(TEXT("PHASE_CHANGE"))] = NOTIFY_KEY::PHASE_CHANGE;


	m_EffectKeys[EFFECT_KEY::FURY] = Hash_String(L"LOOP_FURY");
	m_EffectKeys[EFFECT_KEY::FIRE] = Hash_String(L"LOOP_FIRE");
	m_EffectKeys[EFFECT_KEY::SMOKE1] = Hash_String(L"LOOP_SMOKE1");
	m_EffectKeys[EFFECT_KEY::SMOKE2] = Hash_String(L"LOOP_SMOKE2");
	m_EffectKeys[EFFECT_KEY::SYMBOL] = Hash_String(L"LOOP_SYMBOL");
	m_EffectKeys[EFFECT_KEY::SLIDE] = Hash_String(L"LOOP_SLIDE");

	return S_OK;
}

HRESULT CBoss_Festival::Initialize(void* pArg)
{
	if (nullptr != pArg)
	{
		auto pDesc = static_cast<MONSTER_DESC*>(pArg);
		m_fShortAttackRange = pDesc->fShortAttackRange;
		m_fFarAttackRange = pDesc->fFarAttackRange;
		m_fPhase2Ratio = pDesc->fPhase2Ratio;
		m_fAttackGap = pDesc->fAttackGap;
		m_bWeaponTrailUse = pDesc->bWeaponTrailUse;
		if (false == pDesc->strWeaponTrailFx.empty())
			m_strWeaponTrailFx = pDesc->strWeaponTrailFx;
		Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });
	}

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CAIObject::CHARACTER_SETUP_DESC SetupDesc{};
	SetupDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Festival");
	SetupDesc.strAnimFolderTag = TEXT("FestivalLeader");
	SetupDesc.strAnimTag = TEXT("FestivalLeader");
	SetupDesc.pRootBoneName = "Root";
	SetupDesc.bUseCCT = true;
	SetupDesc.fCCTRadius = 1.8f;
	SetupDesc.fCCTHeight = 2.0f;
	if (pArg != nullptr)
	{
		auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);
		SetupDesc.iModelLevel = pDesc->iProtoLevel;
	}
	else SetupDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(SetupDesc)))
		return E_FAIL;

	if (FAILED(Ready_BodyMeshIndex()))
		return E_FAIL;

	if (FAILED(Ready_Weapon()))
		return E_FAIL;

	if (FAILED(Ready_HitBoxes()))
		return E_FAIL;

	Capture_SpawnPos();

	m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
	return S_OK;
}

void CBoss_Festival::Update(_float fTimeDelta)
{
	// Fguide 컷신이 끝나기 전에는 실제 보스의 애니메이션과 AI를 갱신하지 않는다.
	if (false == m_bBattleStarted || true == Is_AISleep())
		return;

	_vector vPrevPos = m_pTransformCom->Get_State(STATE::POSITION);

	__super::Update(fTimeDelta);

	if (nullptr != m_pCCT)
	{
		_vector vNewPos = m_pTransformCom->Get_State(STATE::POSITION);
		_vector vMove = XMVectorSubtract(vNewPos, vPrevPos);

		if (true == m_bGrounded)
		{
			m_fVerticalVel = -2.f;
		}
		else
		{
			m_fVerticalVel += PhysX::g_fGravity * PhysX::g_fGravityScale * fTimeDelta;
			if (m_fVerticalVel < -30.f)
				m_fVerticalVel = -30.f;
		}
		vMove = XMVectorSetY(vMove, m_fVerticalVel * fTimeDelta);

		CCTResult tResult = m_pCCT->Move(vMove, fTimeDelta);
		m_bGrounded = tResult.m_bGrounded;

		m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(&tResult.vSimulatedPosition));
	}
}

void CBoss_Festival::Physical_Update(_float fFixedTime)
{
	// 컷신용 FestivalLeader와 겹쳐 있는 동안 실제 보스의 물리 처리를 막는다.
	if (false == m_bBattleStarted)
		return;

	__super::Physical_Update(fFixedTime);
}

void CBoss_Festival::Late_Update(_float fTimeDelta)
{
	// RenderGroup 등록을 막아 Fguide 컷신이 끝날 때까지 실제 보스를 렌더링하지 않는다.
	if (false == m_bBattleStarted || true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
	{
		m_HitBoxes.On_StateChanged();
		m_HitBoxes.Set_HitType(HIT_TYPE::Normal);
		Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::SLIDE]);
	}
	m_HitBoxes.Update(m_pGameInstance, this, fTimeDelta);

	if (true == m_bWeaponLive && nullptr != m_pWeapon)
		m_pWeapon->Late_Update(fTimeDelta);

#ifdef _DEBUG
	Debug_MeasureAttack(fTimeDelta);
#endif
}

HRESULT CBoss_Festival::Render()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = static_cast<_uint>(units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();
	const _float4x4* pWorld = m_pTransformCom->Get_WorldMatrixPtr();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

		if (pMaterialCom->Get_RenderGroup() != eCurPass)
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = *pWorld;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.iAlphaClip = 0;
		command.fEmissiveAmount = (i < m_MeshEmissive.size() && m_MeshEmissive[i] > m_fEmissiveAmount)
			? m_MeshEmissive[i] : m_fEmissiveAmount;
		command.iRimProfile = m_iRimProfile;
		command.fRimAmount = m_fRimAmount;
		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

void CBoss_Festival::On_Freeze(_bool bFreeze)
{
	if (true == bFreeze)
	{
		m_HitBoxes.On_StateChanged();
		Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::SLIDE]);
	}
}

_bool CBoss_Festival::Can_FatalBack() const
{
	return false;
}

void CBoss_Festival::Spawn()
{
	__super::Spawn();

	// Chapter01에 미리 배치하되 BOSS_START 전까지 컷신 모델만 보이도록 대기한다.
	m_bBattleStarted = false;
	m_bDead = false;
	m_bPhase2 = false;
	m_bWasIdle = true;
	m_bChasing = false;
	m_bTurning = false;
	m_bPerceived = false;
	m_bGrounded = true;
	m_bAttackCommit = false;

	m_iCurPatternID = 1;
	m_iLastPatternID = 0;

	m_iShowcaseIdx = 0;
	m_bShowcaseWasP2 = false;

	m_fIdleTimer = 0.f;
	m_fDistToTarget = 9999.f;
	m_fVerticalVel = 0.f;
	m_fCommitTimer = 0.f;

	m_bWeaponLive = false;
	if (nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
		m_pWeapon->Set_Render(false);
	}

	Set_BodyMeshVisible(L"Head", true);
	Set_BodyMeshVisible(L"Bag", true);

	m_HitBoxes.On_StateChanged();
	Stop_AllLoopEffects();

	m_bMoving = false;
	m_bMustAttack = false;
	m_iMoveDir = 0;
	m_iTurnDir = static_cast<_int>(MON_DIRECTION::FRONT);
	m_fMoveTimer = 0.f;
	m_fApproachTimer = 0.f;
	m_fChaseSwitchTimer = 10.f;

	if (CBT_BlackBoard* pBB = Get_BlackBoard())
	{
		pBB->Set<_bool>(L"bPhase2", false);
		pBB->Set<_bool>(L"bPhaseChanging", false);
		Set_Groggy(false);
	}
}

void CBoss_Festival::Despawn()
{
	__super::Despawn();

	Stop_AllLoopEffects();

	if (nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
		m_pWeapon->Set_Render(false);
	}

	m_HitBoxes.On_StateChanged();
}

void CBoss_Festival::Action_Notify(_uint64 iHashKey)
{
	auto itType = m_HitTypeKeys.find(iHashKey);
	if (itType != m_HitTypeKeys.end())
	{
		m_HitBoxes.Set_HitType(itType->second);
		return;
	}

	auto it = m_NotifyKeys.find(iHashKey);
	if (it != m_NotifyKeys.end())
	{
		NOTIFY_KEY eKey = static_cast<NOTIFY_KEY>(it->second);
		switch (eKey)
		{
		case NOTIFY_KEY::HEAD_OUT:
			Set_BodyMeshVisible(L"Head", false);
			Set_BodyMeshVisible(L"Bag", false);
			m_bWeaponLive = true;
			m_pWeapon->Set_Render(true);
			Set_EmissiveAmount(1.f);
			m_pWeapon->Set_EmissiveAmount(1.f);
			m_pGameInstance->Play_Effect("Effect_Boss_Fire", m_pModelCom, "Bone068", m_pTransformCom->Get_WorldMatrixPtr(), _float3(0.f, 0.f, 0.f));
			Start_LoopEffect(m_EffectKeys[EFFECT_KEY::SMOKE1], "Effect_Boss_Smoke", "Bn_Head_Front");
			Start_LoopEffect(m_EffectKeys[EFFECT_KEY::SMOKE2], "Effect_Boss_Smoke", "Bone069");
			break;

		case NOTIFY_KEY::HEAD_IN:
		{
			m_pWeapon->Set_Render(false);
			Set_BodyMeshVisible(L"Head", true);
			m_pBlackBoard->Set<_bool>(L"bPhase2", true);
		}
		break;

		case NOTIFY_KEY::SET_DEAD:
		{
			m_bDead = true;
			m_HitBoxes.On_StateChanged();
			Set_EmissiveAmount(0.f);
			Set_RimProfile(0);
			Set_RimAmount(0.f);
			m_pWeapon->Set_EmissiveAmount(0.f);
			Stop_AllLoopEffects();
			__super::On_SetDead();

			BossClear_EventDesc Desc{};
			Desc.eMonsterType = SPAWN_MONSTER_TYPE::BOSS_FESTIVAL;
			m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, &Desc);
		}
		break;
		case NOTIFY_KEY::TRAIL_ON:
			Start_LoopEffect(m_EffectKeys[EFFECT_KEY::SLIDE], "Effect_Boss_Slide", "Bn_FR_Hair_03_End");
			break;

		case NOTIFY_KEY::TRAIL_OFF:
			Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::SLIDE]);
			break;

		case NOTIFY_KEY::FATAL_READY:
		{
			_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vOff = XMVector3Rotate(XMLoadFloat3(&m_vFatalOffsetFront), m_pTransformCom->Get_Quat());
			_float3 vWorldPos;
			XMStoreFloat3(&vWorldPos, XMVectorAdd(vPos, vOff));
			vWorldPos.y += 0.1f;
			Start_LoopEffect_World(m_EffectKeys[EFFECT_KEY::SYMBOL], "Effect_Fatal_Symbol", vWorldPos);
			m_bFatalReady = true;
			Begin_GroggyWindow(5.f);
		}
		break;
		case NOTIFY_KEY::PHASE_CHANGE:
			Set_Groggy(false);
			m_pBlackBoard->Set<BOOL>(L"bFatal", false);
			break;
		default:
			break;
		}
		return;
	}

	m_HitBoxes.Handle_Notify(iHashKey);
}

void CBoss_Festival::Begin_Battle()
{
	// 정상 컷신 종료 이벤트를 받은 프레임부터 실제 보스의 갱신과 렌더링을 허용한다.
	m_bBattleStarted = true;
	m_bPerceived = true;
	m_bDetected = true;

	m_pGameInstance->Play_BGM("MU_MS_Boss_FestivalLeader_PH01_New_Battle", 0.3f, 1.5f);
}

void CBoss_Festival::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
	if (EVENT_TYPE::BOSS_START == etype)
	{
		if (true == Is_MyBossStart(pData))
			Begin_Battle();
	}
}

#ifdef _DEBUG
HRESULT CBoss_Festival::Set_DbgHitIdx(_int iIndex)
{
	const _int iCount = m_HitBoxes.Get_Count();
	if (iCount <= 0)
		return S_OK;

	if (iIndex < 0)			iIndex = 0;
	if (iIndex >= iCount)	iIndex = iCount - 1;
	m_iDbgHitIdx = iIndex;

	m_fDbgHitRadius = m_HitBoxes.Get_Radius(iIndex);
	m_fDbgHitHalf = m_HitBoxes.Get_HalfHeight(iIndex);
	return S_OK;
}

HRESULT CBoss_Festival::Set_DbgHitRadius(_float fRadius)
{
	m_fDbgHitRadius = fRadius;
	m_HitBoxes.Set_Radius(m_iDbgHitIdx, fRadius);
	return S_OK;
}

HRESULT CBoss_Festival::Set_DbgHitHalf(_float fHalf)
{
	m_fDbgHitHalf = fHalf;
	m_HitBoxes.Set_HalfHeight(m_iDbgHitIdx, fHalf);
	return S_OK;
}

void CBoss_Festival::Debug_MeasureAttack(_float fTimeDelta)
{
	if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
		return;

	if (true == m_pGameInstance->Key_Down(DIK_F1))
	{
		Debug_ResetToOrigin();
		return;
	}

	if (true == m_pGameInstance->Key_Down(DIK_NUMPAD9))
	{
		m_iDbgPhase = (1 == m_iDbgPhase) ? 2 : 1;
		m_pBlackBoard->Set<_bool>(L"bPhase2", 2 == m_iDbgPhase);
		m_pBlackBoard->Set<_bool>(L"bPhaseChanging", false);

		_tchar szPhase[64];
		swprintf_s(szPhase, L"[FestivalRuler] phase = %d\n", m_iDbgPhase);
		OutputDebugStringW(szPhase);
		return;
	}

	if (m_iDbgFireDelay > 0)
	{
		--m_iDbgFireDelay;

		if (m_iDbgFireDelay > 0)
			return;

		const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
		XMStoreFloat3(&m_vDbgStartPos, vMyPos);

		_vector vStartLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vStartLook)) > 1e-6f)
			vStartLook = XMVector3Normalize(vStartLook);
		XMStoreFloat3(&m_vDbgStartLook, vStartLook);

		m_fDbgStartDist = -1.f;
		_vector vTargetPos;
		if (true == Get_TargetPos(vTargetPos))
			m_fDbgStartDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f)));

		Set_AnimInt(L"PatternID", m_iDbgAttackID);
		Set_AnimTrigger(L"PatternStart");
		Request_AnimAction(L"ATTACK");

		_tchar szFire[160];
		swprintf_s(szFire, L"[FestivalRuler] fire P%d (phase%d)  startDist=%.2f\n",
			m_iDbgAttackID, m_iDbgPhase, m_fDbgStartDist);
		OutputDebugStringW(szFire);
		return;
	}

	if (false == m_bDbgMeasuring)
	{
		static const _ubyte byFireKeys[DBG_PATTERN_COUNT] =
		{
			DIK_NUMPAD1, DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPAD4,
			DIK_NUMPAD5, DIK_NUMPAD6, DIK_NUMPAD7
		};

		for (_int i = 0; i < DBG_PATTERN_COUNT; ++i)
		{
			if (false == m_pGameInstance->Key_Down(byFireKeys[i]))
				continue;

			Cancel_All_Attacks();
			Begin_Battle();

			m_iDbgAttackID = i + 1;
			m_fDbgTimer = 0.f;
			m_fDbgScaleMax = 1.f;
			m_fDbgLunge = -9999.f;
			m_fDbgLungeAtHit = -9999.f;
			m_fDbgReach = -9999.f;

			for (_int k = 0; k < DBG_HITBOX_COUNT; ++k)
			{
				m_fDbgNear[k] = 9999.f;
				m_fDbgFar[k] = -9999.f;
				m_bDbgSeen[k] = false;
			}

			m_bDbgStarted = false;
			m_bDbgFirstHitDone = false;
			m_bDbgMeasuring = true;
			m_iDbgFireDelay = 2;

			m_bMoving = false;
			m_bApproaching = false;
			m_bAttackCommit = true;
			m_iCurPatternID = m_iDbgAttackID;
			break;
		}
		return;
	}

	m_fDbgTimer += fTimeDelta;
	m_bAttackCommit = true;
	m_bMoving = false;

	const _float fCurScale = Calc_RootMotionScale();
	if (fCurScale > m_fDbgScaleMax)
		m_fDbgScaleMax = fCurScale;

	const _vector vStart = XMLoadFloat3(&m_vDbgStartPos);
	const _vector vStartLook = XMLoadFloat3(&m_vDbgStartLook);

	const _vector vBody = m_pTransformCom->Get_State(STATE::POSITION);
	const _float  fLunge = XMVectorGetX(XMVector3Dot(XMVectorSetY(XMVectorSubtract(vBody, vStart), 0.f), vStartLook));
	if (fLunge > m_fDbgLunge)
		m_fDbgLunge = fLunge;

	if (false == Is_AnimIdle())
		m_bDbgStarted = true;

	if (false == Is_HitActive())
	{
		if (true == m_bDbgStarted && m_fDbgReach > -9998.f)
			m_bDbgFirstHitDone = true;
	}
	else if (false == m_bDbgFirstHitDone)
	{
		if (m_fDbgLungeAtHit < -9998.f)
			m_fDbgLungeAtHit = fLunge;

		for (_int k = 0; k < DBG_HITBOX_COUNT; ++k)
		{
			_float3 vCenter = {};
			_float  fRadius = 0.f;
			if (false == m_HitBoxes.Get_ArmedCenter(g_DbgHitIds[k], this, vCenter, fRadius))
				continue;

			const _vector vTo = XMVectorSetY(XMVectorSubtract(XMLoadFloat3(&vCenter), vStart), 0.f);
			const _float  fFwd = XMVectorGetX(XMVector3Dot(vTo, vStartLook));

			m_bDbgSeen[k] = true;
			if (fFwd < m_fDbgNear[k]) m_fDbgNear[k] = fFwd;
			if (fFwd > m_fDbgFar[k])  m_fDbgFar[k] = fFwd;

			if (fFwd + fRadius > m_fDbgReach)
				m_fDbgReach = fFwd + fRadius;
		}
	}

	const _bool bDone = (true == m_bDbgStarted && true == m_bDbgFirstHitDone);
	const _bool bTimeout = (m_fDbgTimer > 15.f);

	if (false == bDone && false == bTimeout)
		return;

	_tchar szLog[512];
	swprintf_s(szLog, L"[FestivalRuler] P%d(phase%d)  scale=%.2f  startDist=%.2f  reach=%+.2f  lungeAtHit=%+.2f  lungeMax=%+.2f  t=%.2fs%s\n",
		m_iDbgAttackID, m_iDbgPhase, m_fDbgScaleMax, m_fDbgStartDist,
		m_fDbgReach, m_fDbgLungeAtHit, m_fDbgLunge, m_fDbgTimer,
		bTimeout ? L"  (TIMEOUT)" : L"");
	OutputDebugStringW(szLog);

	for (_int k = 0; k < DBG_HITBOX_COUNT; ++k)
	{
		if (false == m_bDbgSeen[k])
			continue;

		swprintf_s(szLog, L"                 %-8s fwd=[%+.2f ~ %+.2f]\n",
			g_DbgHitTags[k], m_fDbgNear[k], m_fDbgFar[k]);
		OutputDebugStringW(szLog);
	}

	if (m_fDbgReach < -9998.f)
	{
		if (false == m_bDbgStarted)
			OutputDebugStringW(L"                 ATTACK_STATE_NOT_ENTERED (PatternID 조건 / ActionStateMaps 확인)\n");
		else
			OutputDebugStringW(L"                 NO_HITBOX_ARMED (HitOn_ 노티 / 히트박스 Id 확인)\n");
	}

	m_bDbgMeasuring = false;
}


void CBoss_Festival::Debug_ResetToOrigin()
{
	const _vector vOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	if (nullptr != m_pCCT)
		m_pCCT->Set_FootPosition(vOrigin);

	if (nullptr != m_pTransformCom)
		m_pTransformCom->Set_State(STATE::POSITION, vOrigin);

	Cancel_All_Attacks();

	m_bDbgMeasuring = false;
	m_bDbgStarted = false;
	m_bDbgFirstHitDone = false;
	m_iDbgFireDelay = 0;
	m_bAttackCommit = false;
	m_bMoving = false;
	m_bTurning = false;
	m_iPendingPatternID = -1;
	m_bApproaching = false;

	OutputDebugStringW(L"[FestivalRuler] reset to origin\n");
}

#endif

void CBoss_Festival::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	Update_PhaseTransition();
	Update_CombatParams();
	Mirror_ControllerParams();

	if (true == m_bDead || Is_Dead())
	{
		Set_AnimFloat(L"Speed", 0.f);
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::NONE));
		Set_AnimBool(L"Turn", false);
		m_bTurning = false;
		m_bMoving = false;
		return;
	}

	Update_PhaseRequest();
	Drive_Steering(fTimeDelta);
	Update_PatternSelect(fTimeDelta);
}

void CBoss_Festival::Record_HitType(const HitInfo& hitInfo)
{
}

_bool CBoss_Festival::Handles_FatalEnd() const
{
	return false;
}

void CBoss_Festival::On_Fatal_Begin()
{
	Set_Groggy(false);
}

_bool CBoss_Festival::Should_Ignore_Damage(const HitInfo& hitInfo)
{
	return m_pBlackBoard->Get<_bool>(L"bPhaseChanging");
}

void CBoss_Festival::On_Perceive_Attacker(_bool bPreFatal)
{
	if (false == bPreFatal)
		m_bPerceived = true;
}

_bool CBoss_Festival::Uses_Full_Hit_Reaction() const
{
	return false;
}

void CBoss_Festival::Apply_ChargeHit(_float fDamage, _float fGroggy)
{
	m_pStatCom->Apply_Damage(fDamage);
	if (Is_Dead())
		return;

	m_pStatCom->Set_SinceHit(0.f);

	if (true == m_pStatCom->Is_Staggerable())
	{
		m_pStatCom->Set_Staggerable(false);
		m_pStatCom->Reset_Groggy();
		Set_Groggy(true);
	}
	else
	{
		m_pStatCom->Add_Groggy(fGroggy);
		if (m_pStatCom->Is_GroggyFull())
		{
			m_pStatCom->Reset_Groggy();
			m_pStatCom->Set_Staggerable(true);
		}
	}
}

void CBoss_Festival::Set_BodyMeshEmissive(const _wstring& strKey, _float fAmount)
{
	auto it = m_MeshIndexMap.find(strKey);
	if (it == m_MeshIndexMap.end())
		return;

	for (_uint iSlot : it->second)
	{
		if (iSlot < m_MeshEmissive.size())
			m_MeshEmissive[iSlot] = fAmount;
	}
}

void CBoss_Festival::Update_PhaseTransition()
{
	const _bool bPhase2 = m_pBlackBoard->Get<_bool>(L"bPhase2");
	const _bool bPhaseChanging = m_pBlackBoard->Get<_bool>(L"bPhaseChanging");

	if (false == bPhase2 && false == bPhaseChanging &&
		m_pStatCom->Get_CurrentHP() > 0.f &&
		m_pStatCom->Get_CurrentHP() <= m_pStatCom->Get_MaxHP() * m_fPhase2Ratio)
	{
		m_pBlackBoard->Set<_bool>(L"bPhaseChanging", true);
	}
}

void CBoss_Festival::Update_PhaseRequest()
{
	if (false == m_pBlackBoard->Get<_bool>(L"bPhaseChanging"))
		return;

	if (true == Is_AnimInState(L"PHASE_CHANGE"))
		return;

	if (false == Is_AnimIdle())
		return;

	Set_AnimTrigger(L"PhaseChange");
	Request_AnimAction(L"PHASE_CHANGE");
}

void CBoss_Festival::Update_CombatParams()
{
	const _bool bDead = Is_Dead();
	if (true == bDead)
	{
		m_bPerceived = false;
		m_bDetected = false;
	}
	m_fDistToTarget = 9999.f;
	if (nullptr != m_pTransformCom && nullptr != m_pTarget)
	{
		if (CTransform* pTT = m_pTarget->Get_Transform())
		{
			_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vTo = XMVectorSubtract(
				XMVectorSetY(pTT->Get_State(STATE::POSITION), XMVectorGetY(vMyPos)), vMyPos);
			m_fDistToTarget = XMVectorGetX(XMVector3Length(vTo));
		}
	}

	const _int iPhase = m_bPhase2 ? 2 : 1;

	_float fAttackReach = Pattern_MaxLength(iPhase);
	const _float fCurLen = Pattern_Length(iPhase, m_iCurPatternID);
	if (fCurLen > 0.f)
		fAttackReach = fCurLen;

	const _float fMargin = 0.7f;
	const _float fDwell = 0.35f;

	m_fChaseSwitchTimer += m_pBlackBoard->Get<_float>(L"fTimeDelta");

	if (m_fChaseSwitchTimer >= fDwell)
	{
		if (false == m_bChasing && m_fDistToTarget > fAttackReach + fMargin)
		{
			m_bChasing = true;
			m_fChaseSwitchTimer = 0.f;
		}
		else if (true == m_bChasing && m_fDistToTarget <= fAttackReach)
		{
			m_bChasing = false;
			m_fChaseSwitchTimer = 0.f;
		}
	}

	const _float fCommitGiveUp = fAttackReach + 3.0f;
	if (false == m_bChasing)
	{
		m_bAttackCommit = true;
		m_fCommitTimer = 0.f;
	}
	else if (true == m_bAttackCommit)
	{
		m_fCommitTimer += m_pBlackBoard->Get<_float>(L"fTimeDelta");
		if (m_fDistToTarget > fCommitGiveUp || m_fCommitTimer > 1.2f)
			m_bAttackCommit = false;
	}

	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", m_bPerceived);
	m_pBlackBoard->Set<_bool>(L"bInAttackRange", m_bPerceived && (false == m_bChasing || m_bAttackCommit));
	m_pBlackBoard->Set<_bool>(L"bNeedChase", m_bPerceived && m_bChasing && false == m_bAttackCommit);
}

void CBoss_Festival::Mirror_ControllerParams()
{
	const _bool bPhase2Prev = m_bPhase2;
	m_bPhase2 = m_pBlackBoard->Get<_bool>(L"bPhase2");

	Set_AnimInt(L"Phase", static_cast<_int>(m_bPhase2 ? PHASE::PHASE2 : PHASE::PHASE1));

	Set_AnimBool(L"Dead", m_pBlackBoard->Get<_bool>(L"bDead") && false == m_bDead);
	Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
	Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
}

void CBoss_Festival::Drive_Facing(_fvector vTargetPos, _float fTimeDelta)
{
	if (nullptr == m_pTransformCom || nullptr == m_pMovementCom)
		return;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
	{
		m_bTurning = false;
		return;
	}

	vTo = XMVector3Normalize(vTo);
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	_float fDot = XMVectorGetX(XMVector3Dot(vLook, vTo));
	if (fDot > 1.f) fDot = 1.f;
	else if (fDot < -1.f) fDot = -1.f;
	const _float fAngleDeg = XMConvertToDegrees(acosf(fDot));

	if (false == m_bTurning)
	{
		if (fAngleDeg > m_fTurnStartDeg)
		{
			m_bTurning = true;
			const _float fCrossY = XMVectorGetY(XMVector3Cross(vLook, vTo));
			m_iTurnDir = static_cast<_int>(fCrossY >= 0.f ? MON_DIRECTION::RIGHT : MON_DIRECTION::LEFT);
		}
	}
	else
	{
		if (fAngleDeg <= m_fTurnStopDeg)
			m_bTurning = false;
	}

	if (true == m_bTurning)
	{
		Set_AnimInt(L"Direction", m_iTurnDir);
		Set_AnimBool(L"Turn", true);
	}

	if (true == Can_AnimRotate())
		m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
}

void CBoss_Festival::Drive_Steering(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);
	Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::NONE));
	Set_AnimBool(L"Turn", false);

	if (m_pStatCom->Is_Dead()) { m_bTurning = false; m_bMoving = false; return; }

	if (true == m_pBlackBoard->Get<_bool>(L"bGroggy") || true == m_pBlackBoard->Get<_bool>(L"bFatal") ||
		true == m_pBlackBoard->Get<_bool>(L"bPhaseChanging"))
	{
		m_bTurning = false; m_bMoving = false; return;
	}

	if (false == m_pBlackBoard->Get<_bool>(L"bPlayerInDetect")) { m_bTurning = false; m_bMoving = false; return; }

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos)) { m_bTurning = false; m_bMoving = false; return; }

	Drive_Facing(vTargetPos, fTimeDelta);

	if (true == m_bTurning)
		return;

	if (true == m_bMoving && true == Can_AnimMove())
	{
		const _vector vRight = m_pTransformCom->Get_State(STATE::RIGHT);

		Set_AnimInt(L"Direction", m_iMoveDir);
		Set_AnimFloat(L"Speed", 1.f);

		switch (static_cast<MON_DIRECTION>(m_iMoveDir))
		{
		case MON_DIRECTION::LEFT:  Move_Flat(XMVectorNegate(vRight), m_fStrafeSpeed, fTimeDelta);	break;
		case MON_DIRECTION::RIGHT: Move_Flat(vRight, m_fStrafeSpeed, fTimeDelta);					break;
		default: break;
		}
		return;
	}

	if (true == m_pBlackBoard->Get<_bool>(L"bNeedChase") && true == Can_AnimMove())
	{
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
		Set_AnimFloat(L"Speed", 3.f);

		Move_Flat(m_pTransformCom->Get_State(STATE::LOOK), m_fChaseSpeed, fTimeDelta);
	}
}

void CBoss_Festival::Update_PatternSelect(_float fTimeDelta)
{
	const _bool bIdleNow = Is_AnimIdle();

#ifdef _DEBUG
	if (true == m_bDbgMeasuring)
	{
		m_pBlackBoard->Set<_bool>(L"bAttackReady", false);
		Set_AnimInt(L"PatternID", m_iDbgAttackID);
		m_fIdleTimer = 0.f;
		m_bMoving = false;
		m_bWasIdle = bIdleNow;
		return;
	}
#endif

	if (true == bIdleNow)	m_fIdleTimer += fTimeDelta;
	else					m_fIdleTimer = 0.f;

	if (false == bIdleNow)
		m_fApproachTimer = 0.f;

	if (true == m_bMoving)
	{
		m_fMoveTimer -= fTimeDelta;
		if (m_fMoveTimer <= 0.f)
		{
			m_bMoving = false;
			m_bMustAttack = true;
		}
	}

	const _int   iPhase = m_bPhase2 ? 2 : 1;
	const _float fCurLen = Pattern_Length(iPhase, m_iCurPatternID);

	_bool bInRange = (fCurLen <= 0.f) || (m_fDistToTarget <= fCurLen + m_fAttackRangeTol);

	if (false == bInRange)
	{
		m_fApproachTimer += fTimeDelta;
		if (m_fApproachTimer >= m_fApproachTimeout)
			bInRange = true;
	}
	else
		m_fApproachTimer = 0.f;

	const _bool bPhaseChanging = m_pBlackBoard->Get<_bool>(L"bPhaseChanging");

	m_pBlackBoard->Set<_bool>(L"bAttackReady",
		bIdleNow && m_fIdleTimer >= m_fAttackGap && false == m_bMoving
		&& false == bPhaseChanging && true == bInRange);

	if (true == bPhaseChanging)
	{
		m_bMoving = false;
		m_bWasIdle = bIdleNow;
		return;
	}

	if (true == m_bPhase2 && false == m_bShowcaseWasP2)
		m_iShowcaseIdx = 0;
	m_bShowcaseWasP2 = m_bPhase2;

	const PATTERN_DEF* pTable = Pattern_Table(iPhase);
	const _int  iTableCount = Pattern_Count(iPhase);
	const _bool bShowcasing = (m_iShowcaseIdx < iTableCount);

	if (false == m_bWasIdle && true == bIdleNow && false == m_bMoving)
	{
		const _bool bCanMove = (false == m_bMustAttack)
			&& (false == bShowcasing || m_iShowcaseIdx > 0)
			&& (m_fDistToTarget <= Pattern_MaxLength(iPhase))
			&& ((static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) < m_fMoveChance);

		if (true == bCanMove)
		{
			m_iMoveDir = (0 == (rand() % 2)) ? static_cast<_int>(MON_DIRECTION::LEFT)
				: static_cast<_int>(MON_DIRECTION::RIGHT);

			m_bMoving = true;
			m_fMoveTimer = m_fMoveDuration;
			m_fIdleTimer = 0.f;
		}
		else if (true == bShowcasing)
		{
			m_iCurPatternID = pTable[m_iShowcaseIdx].iID;
			m_iLastPatternID = m_iCurPatternID;
			m_bMustAttack = false;
			m_fApproachTimer = 0.f;
		}
		else
		{
			m_iCurPatternID = Select_WeightedPattern(iPhase, m_fDistToTarget);
			m_bMustAttack = false;
			m_fApproachTimer = 0.f;
		}
	}

	if (true == m_bWasIdle && false == bIdleNow && true == bShowcasing && false == m_bMoving)
		++m_iShowcaseIdx;

	m_bWasIdle = bIdleNow;

	if (true == bIdleNow && false == m_bMoving)
		Set_AnimInt(L"PatternID", m_iCurPatternID);
}

_bool CBoss_Festival::Is_AnimIdle() const
{
	return Is_AnimInState(L"P1_IDLE") || Is_AnimInState(L"P2_IDLE");
}

_int CBoss_Festival::Select_WeightedPattern(_int iPhase, _float fDist)
{
	const PATTERN_DEF* pTable = Pattern_Table(iPhase);
	const _int iCount = Pattern_Count(iPhase);

	const _float W_RECENT = 0.15f;
	const _float W_CHAIN = 0.35f;

	const _float fLastLen = Pattern_Length(iPhase, m_iLastPatternID);
	const _bool  bLastFar = (fLastLen > m_fShortAttackRange);

	_float fW[8] = {};
	_float fTotal = 0.f;

	for (_int i = 0; i < iCount; ++i)
	{
		const _float fLen = pTable[i].fAttackLength;

		_float w = pTable[i].fWeight / (1.f + fabsf(fDist - fLen));

		if (pTable[i].iID == m_iLastPatternID)
			w *= W_RECENT;

		if (true == bLastFar && fLen > m_fShortAttackRange)
			w *= W_CHAIN;

		fW[i] = w;
		fTotal += w;
	}

	if (fTotal <= 0.f)
	{
		m_iLastPatternID = pTable[0].iID;
		return m_iLastPatternID;
	}

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	for (_int i = 0; i < iCount; ++i)
	{
		r -= fW[i];
		if (r <= 0.f)
		{
			m_iLastPatternID = pTable[i].iID;
			return m_iLastPatternID;
		}
	}

	m_iLastPatternID = pTable[iCount - 1].iID;
	return m_iLastPatternID;
}

HRESULT CBoss_Festival::Ready_BodyMeshIndex()
{
	const _wstring CtrlMeshKeys[] = { TEXT("Head"), TEXT("Bag"), TEXT("Upper") };
	for (const auto& key : CtrlMeshKeys)
	{
		vector<_int> Indices = Find_MeshIndices(key);
		if (true == Indices.empty())
			continue;

		vector<_uint>& Slots = m_MeshIndexMap[key];
		for (_int iIndex : Indices)
			Slots.push_back(static_cast<_uint>(iIndex));
	}

	m_MeshEmissive.assign(m_pModelCom->Get_RenderUnits().size(), 0.f);
	Set_BodyMeshEmissive(TEXT("Head"), 1.f);

	return S_OK;
}

void CBoss_Festival::Set_BodyMeshVisible(const _wstring& strKey, _bool bVisible)
{
	auto it = m_MeshIndexMap.find(strKey);
	if (it == m_MeshIndexMap.end())
		return;

	for (_uint iSlot : it->second)
		m_MeshVisibles[ETOI(iSlot)] = bVisible;
}

vector<_int> CBoss_Festival::Find_MeshIndices(const _wstring& strMaterialKey) const
{
	vector<_int> Indices;

	if (nullptr == m_pModelCom)
		return Indices;
	const auto& Units = m_pModelCom->Get_RenderUnits();
	for (_uint i = 0; i < ETOUI(Units.size()); i++)
	{
		if (Units[i].pMesh != nullptr && Units[i].pMesh->Get_MaterialName().find(strMaterialKey) != _wstring::npos)
			Indices.push_back(ETOI(i));
	}

	return Indices;
}

HRESULT CBoss_Festival::Ready_Weapon()
{
	const _float4x4* pSocket = Get_Bone_CombinedMatPtr("BN_Weapon_R");
	if (nullptr == pSocket)
		return E_FAIL;

	CWeapon_Melee::MELEE_DESC wd{};
	wd.pOwner = this;
	wd.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	wd.pSocketBoneMatrix = pSocket;
	wd.strModelTag = TEXT("Prototype_Component_Model_FestivalHead");
	wd.iLevelID = ETOUI(LEVEL::GAMEPLAY);
	wd.strTrailTopBoneName = "FX_WP_Trail_R_1_1";
	wd.strTrailBottomBoneName = "FX_WP_Trail_R_1_5";
	wd.strTrailFxTag = m_strWeaponTrailFx;
	wd.fSweepRadius = 0.7f;
	wd.fSweepHalfHeight = 1.2f;
	wd.fDamage = 1000.f;
	wd.bRender = false;
	wd.bTrailUse = m_bWeaponTrailUse;

	if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon_Melee"), TEXT("Obj_Weapon"), &wd)))
		return E_FAIL;

	m_pWeapon = static_cast<CWeapon_Monster*>(Get_PartObject(TEXT("Obj_Weapon")));

	return S_OK;
}


HRESULT CBoss_Festival::Ready_HitBoxes()
{
	m_HitBoxes.Add_BoneSphere(L"HandR", "SD_SK_Hand_R", 1.2f, 600.f);
	m_HitBoxes.Add_BoneSphere(L"HandL", "SD_SK_Hand_L", 1.2f, 600.f);
	m_HitBoxes.Add_BoneSphere(L"Body", "Bip001-Spine1", 2.5f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Basket", "Bn_Basket_Body", 2.5f, 800.f);
	m_HitBoxes.Add_Weapon(L"Weapon", m_pWeapon, 1000.f, "BN_Weapon_R1");

	if (FAILED(m_HitBoxes.Build(m_pModelCom, m_pDevice, m_pContext)))
		return E_FAIL;

#ifdef _DEBUG
	Set_DbgHitIdx(0);
#endif

	return S_OK;
}

void CBoss_Festival::On_SuperArmor(_bool bOn)
{
	if (bOn)
	{
		Start_LoopEffect(m_EffectKeys[EFFECT_KEY::FURY], "Effect_Boss_Furry", "Bip001-Spine1");
		Set_RimAmount(0.5f);
		Set_RimProfile(1);
		m_pWeapon->Set_RimAmount(0.5f);
		m_pWeapon->Set_RimProfile(1);
	}
	else
	{
		Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::FURY]);
		Set_RimAmount(0.f);
		m_pWeapon->Set_RimAmount(0.f);
	}
}

void CBoss_Festival::On_Groggy(_bool bGroggy)
{
	if (true == bGroggy)
	{
		m_HitBoxes.On_StateChanged();
	}
	else
	{
		Stop_LoopImmediate(m_EffectKeys[EFFECT_KEY::SYMBOL]);
		m_bFatalReady = false;
	}
}

void CBoss_Festival::Move_Flat(_fvector vWorldDir, _float fSpeed, _float fTimeDelta)
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vDir = XMVectorSetY(vWorldDir, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
		return;

	vDir = XMVector3Normalize(vDir);

	const _vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorAdd(vPos, XMVectorScale(vDir, fSpeed * fTimeDelta)));
}

CBoss_Festival* CBoss_Festival::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBoss_Festival* pInstance = new CBoss_Festival(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBoss_Festival");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBoss_Festival::Clone(void* pArg)
{
	CBoss_Festival* pInstance = new CBoss_Festival(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBoss_Festival");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBoss_Festival::Free()
{
	m_pGameInstance->UnSubscribe_AllEvent(this);

	__super::Free();
	m_HitBoxes.Free();
}
