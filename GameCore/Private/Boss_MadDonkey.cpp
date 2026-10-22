#include "Boss_MadDonkey.h"
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

#include "StatComponent_AI.h"
#include "Movement_AI.h"
#include "NPC.h"
#include "DialogueData.h"

#include "Dissolve.h"
#include "DropItem.h"
#include "Player.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CBoss_MadDonkey, CAIObject)
END_REFLECTION(CBoss_MadDonkey)
#endif

namespace
{
	typedef struct tagDonkeyPick
	{
		_int	iID;
		_float	fWeight;
		_float	fBandMin;
		_float	fBandMax;
		_float	fAttackLength;
	} DONKEY_PICK;

	const DONKEY_PICK s_Picks[] =
	{
		{ 1, 1.0f, 0.0f,  4.5f, 2.8f },
		{ 2, 1.0f, 3.0f,  5.0f, 3.2f },
		{ 3, 0.6f, 5.0f,  7.5f, 6.1f },
		{ 4, 1.0f, 3.0f,  5.0f, 2.5f },
		{ 5, 1.0f, 3.5f,  5.5f, 3.4f },
		{ 6, 1.0f, 0.0f,  4.5f, 3.1f },
		{ 7, 1.0f, 0.0f,  4.0f, 2.3f },
	};

	const _int PICK_COUNT = static_cast<_int>(sizeof(s_Picks) / sizeof(DONKEY_PICK));

	const string DIALOGUE_ID_BATTLE = "Boss.MadDonkey.PreBattle";
	const string DIALOGUE_ID_GATE_CHECK = "Boss.MadDonkey.Gate.Check";
	const string DIALOGUE_ID_GATE_PASS = "Boss.MadDonkey.Gate.Pass";
	const string DIALOGUE_ID_GATE_DENY = "Boss.MadDonkey.Gate.Deny";

	const _wstring DROP_ITEM_NAME = L"Item_Helmet_MadDonkey";

	const string BGM_BATTLE_KEY = "MU_MS_Stalker_Battle_BGM";
	const _float BGM_FADE_SEC = 1.5f;

	const _float EXIT_WALK_TIME = 4.f;
	const _float EXIT_DISSOLVE_DURATION = 1.5f;

	const _float DEAD_DISSOLVE_DELAY = 4.f;
	const _float DEAD_DISSOLVE_DURATION = 1.5f;

	const _float4 DISSOLVE_EDGE_COLOR = { 0.9f, 0.35f, 0.1f, 6.f };

	const _uint64 SYMBOL_FX_KEY = Hash_String(L"LOOP_SYMBOL");

	const _int LINK_ON_CLIP_END = -1;

	typedef struct tagDonkeyLink
	{
		_int	iFrom;
		_int	iFrameStart;
		_int	iFrameEnd;
		_int	iTo;
	} DONKEY_LINK;

	const DONKEY_LINK s_Links[] =
	{
		{   1,               130, 140, 101 },
		{ 101,               140, 160, 102 },
		{   4, LINK_ON_CLIP_END,   0, 103 },
		{   5,               120, 130, 104 },
	};

	const _int LINK_COUNT = static_cast<_int>(sizeof(s_Links) / sizeof(DONKEY_LINK));

	_float Pattern_Length(_int iID)
	{
		for (_int i = 0; i < PICK_COUNT; ++i)
			if (s_Picks[i].iID == iID)
				return s_Picks[i].fAttackLength;
		return 0.f;
	}
}

CBoss_MadDonkey::CBoss_MadDonkey(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
	m_eActorType = ACTOR_TYPE::BOSS;
	m_strMonsterName = L"당나귀 광인";
}

CBoss_MadDonkey::CBoss_MadDonkey(const CBoss_MadDonkey& Prototype)
	: CAIObject{ Prototype }
	, m_AttackData{ Prototype.m_AttackData }
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBoss_MadDonkey::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_MadDonkey.json";

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	if (FAILED(Ready_AttackData()))
		return E_FAIL;

	return S_OK;
}

HRESULT CBoss_MadDonkey::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CBoss_MadDonkey");
		return E_FAIL;
	}

	MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);

	m_fShortAttackRange = pDesc->fShortAttackRange;
	m_fFarAttackRange = pDesc->fFarAttackRange;
	m_fRunChaseRange = pDesc->fRunChaseRange;
	m_fAttackGap = pDesc->fAttackGap;
	m_fMoveDuration = pDesc->fMoveDuration;
	m_strWeaponSocketBone = pDesc->strWeaponSocketBone;
	m_fSweepRadius = pDesc->fSweepRadius;
	m_fSweepHalfHeight = pDesc->fSweepHalfHeight;
	Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	CHARACTER_SETUP_DESC CharDesc{};
	CharDesc.strModelPrototypeTag = pDesc->strModelProtoTag;
	CharDesc.strAnimFolderTag = pDesc->strAnimFolderTag;
	CharDesc.strAnimTag = pDesc->strAnimFolderTag;
	CharDesc.pRootBoneName = pDesc->pRootBoneName;
	CharDesc.bUseCCT = true;
	CharDesc.fCCTRadius = pDesc->fCCTRadius;
	CharDesc.fCCTHeight = pDesc->fCCTHeight;
	CharDesc.fRunSpeed = pDesc->fRunSpeed;
	CharDesc.fSprintSpeed = pDesc->fSprintSpeed;
	CharDesc.iModelLevel = pDesc->iProtoLevel;

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_Weapon(*pDesc)))
		return E_FAIL;

	if (FAILED(Ready_Dissolve()))
		return E_FAIL;

	Capture_SpawnPos();

	m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::UI_START, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST, this);

	Spawn();

	return S_OK;
}

HRESULT CBoss_MadDonkey::Ready_NotifyKeys()
{
	m_NotifyKeys[Hash_String(L"HitOn_Weapon")] = NOTIFY_KEY::WEAPON_ON;
	m_NotifyKeys[Hash_String(L"HitOff_Weapon")] = NOTIFY_KEY::WEAPON_OFF;
	m_NotifyKeys[Hash_String(L"SET_DEAD")] = NOTIFY_KEY::SET_DEAD;
	m_NotifyKeys[Hash_String(L"CHECK_DEAD")] = NOTIFY_KEY::CHECK_DEAD;
	m_NotifyKeys[Hash_String(L"Symbol_Start")] = NOTIFY_KEY::FATAL_READY;

	m_HitTypeKeys[Hash_String(L"HitType_Normal")] = HIT_TYPE::Normal;
	m_HitTypeKeys[Hash_String(L"HitType_Heavy")] = HIT_TYPE::Heavy;
	m_HitTypeKeys[Hash_String(L"HitType_HeavyUnGuard")] = HIT_TYPE::HeavyUnGuard;
	m_HitTypeKeys[Hash_String(L"HitType_Fury")] = HIT_TYPE::Fury;

	return S_OK;
}

HRESULT CBoss_MadDonkey::Ready_AttackData()
{
	m_AttackData[Hash_String(L"ATTACK_01")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_02")] = { 1200.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_03")] = { 1200.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_04")] = { 1100.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_05")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_06")] = { 1100.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_07")] = { 1200.f, HIT_TYPE::Normal };

	m_AttackData[Hash_String(L"COMBO_01")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_02")] = { 1000.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_03")] = { 1100.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_04")] = { 1200.f, HIT_TYPE::Normal };

	return S_OK;
}

HRESULT CBoss_MadDonkey::Ready_Weapon(const MONSTER_DESC& Desc)
{
	if (true == Desc.strWeaponProtoTag.empty())
		return S_OK;

	const _float4x4* pSocket = Get_Bone_CombinedMatPtr(Desc.strWeaponSocketBone.c_str());
	if (nullptr == pSocket)
	{
		MSG_BOX("CBoss_MadDonkey : Weapon Socket Bone Not Found");
		return E_FAIL;
	}

	CWeapon_Melee::MELEE_DESC wd{};
	wd.pOwner = this;
	wd.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	wd.pSocketBoneMatrix = pSocket;
	wd.strModelTag = Desc.strWeaponProtoTag;
	wd.iLevelID = ETOUI(LEVEL::GAMEPLAY);
	wd.strTrailTopBoneName = Desc.strWeaponTrailTopBone;
	wd.strTrailBottomBoneName = Desc.strWeaponTrailBotBone;
	wd.strTrailFxTag = Desc.strWeaponTrailFx;
	wd.fSweepRadius = Desc.fSweepRadius;
	wd.fSweepHalfHeight = Desc.fSweepHalfHeight;
	wd.fDamage = Desc.fWeaponDamage;
	wd.bRender = true;
	wd.bTrailUse = true;

	if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon_Melee"), TEXT("Obj_Weapon"), &wd)))
		return E_FAIL;

	m_pWeapon = static_cast<CWeapon_Monster*>(Get_PartObject(TEXT("Obj_Weapon")));

	return S_OK;
}

HRESULT CBoss_MadDonkey::Ready_Dissolve()
{
	CDissolve::DISSOLVE_DESC DissolveDesc{};
	DissolveDesc.fStartDelay = DEAD_DISSOLVE_DELAY;
	DissolveDesc.fDuration = DEAD_DISSOLVE_DURATION;
	DissolveDesc.fEndDelay = 0.f;
	DissolveDesc.vEdgeColor = DISSOLVE_EDGE_COLOR;

	m_pDissolveCom = CDissolve::Create(&DissolveDesc);

	if (nullptr == m_pDissolveCom)
		return E_FAIL;

	return S_OK;
}

void CBoss_MadDonkey::Update(_float fTimeDelta)
{
	Update_Dissolve(fTimeDelta);

	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
		Apply_Movment(fTimeDelta);
}

void CBoss_MadDonkey::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CBoss_MadDonkey::Late_Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
	{
		m_eCurHitType = HIT_TYPE::Normal;

		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();
	}

	if (nullptr != m_pWeapon)
		m_pWeapon->Late_Update(fTimeDelta);

#ifdef _DEBUG
	Debug_Input(fTimeDelta);
	//Debug_MeasureAttack(fTimeDelta);
#endif
}

HRESULT CBoss_MadDonkey::Render()
{
	return __super::Render();
}

void CBoss_MadDonkey::Spawn()
{
	__super::Spawn();

	m_bDead = false;
	m_bBattleStarted = false;
	m_bTalking = false;
	m_bKnockbackPending = false;

	m_bMoving = false;
	m_bDashMove = false;
	m_bMustAttack = false;
	m_iCurPatternID = 0;
	m_iLastPatternID = 0;
	m_iMoveDir = 0;

	m_fIdleTimer = 0.f;
	m_fMoveTimer = 0.f;
	m_fDistToTarget = 9999.f;

	m_bChasing = true;
	m_fChaseSwitchTimer = 0.f;
	m_bTurning = false;
	m_iTurnDir = static_cast<_int>(MON_DIRECTION::FRONT);

	m_bLinkFired = false;
	m_iLinkOwnerID = -1;

	m_bExiting = false;
	m_bExitPending = false;
	m_bGateAnswered = false;
	m_bGateChecked = false;
	m_fExitTimer = 0.f;
	m_bDissolving = false;
	m_bDropPending = false;
	m_bDeadHandled = false;

	if (nullptr != m_pDissolveCom)
		m_pDissolveCom->Reset();

	Set_Dissolve(0.f, DISSOLVE_EDGE_COLOR);

	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
	Set_AnimBool(L"Spawn", false);
	Set_AnimBool(L"Talk", false);

	if (nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
		m_pWeapon->Set_Render(true);
		m_pWeapon->Set_Dissolve(0.f, DISSOLVE_EDGE_COLOR);
	}
}

void CBoss_MadDonkey::Despawn()
{
	__super::Despawn();

	if (true == m_bTalking)
	{
		m_bTalking = false;
		m_pGameInstance->Broadcast(EVENT_TYPE::UI_END, nullptr);
	}

	Stop_AllLoopEffects();

	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();
}

_bool CBoss_MadDonkey::Has_GatePass() const
{
	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

	if (nullptr == pPlayer)
		return false;

	return pPlayer->Is_DonkeyMask();
}

void CBoss_MadDonkey::Begin_Dialogue()
{
	if (true == m_bBattleStarted || true == m_bTalking || true == m_bExiting)
		return;

	m_bTalking = true;

	Face_Target_Immediate();

	const _bool bGatePass = Has_GatePass();

	Set_AnimBool(L"Talk", true);
	Set_AnimBool(L"GATEPASS", bGatePass);
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
	Request_AnimAction(L"DIALOG");

	CNPC::NPC_DIALOGUE_EVENT_DESC EventDesc{};
	EventDesc.iTargetSpawnID = m_iSpawnID;
	EventDesc.eNPCType = NPC_TYPE::MADDONKEY;
	if (false == Is_Gatekeeper())
		EventDesc.strDialogueID = DIALOGUE_ID_BATTLE;
	else if (false == m_bGateChecked)
		EventDesc.strDialogueID = DIALOGUE_ID_GATE_CHECK;
	else
		EventDesc.strDialogueID = bGatePass ? DIALOGUE_ID_GATE_PASS : DIALOGUE_ID_GATE_DENY;
	EventDesc.bLockCamera = true;

	_vector vLookPos;
	if (true == Get_LockOnWorldPosition(vLookPos))
		XMStoreFloat3(&EventDesc.vSpeakerLookPos, vLookPos);
	else
		XMStoreFloat3(&EventDesc.vSpeakerLookPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_START, &EventDesc);

#ifdef _DEBUG
	_tchar szLog[128];
	swprintf_s(szLog, L"[MadDonkey] Begin_Dialogue  SpawnID=%u  %s\n",
		m_iSpawnID, Is_Gatekeeper() ? L"GATEKEEPER" : L"BOSS");
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Face_Target_Immediate()
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);

	if (XMVectorGetX(XMVector3LengthSq(XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f))) < 1e-6f)
		return;

	m_pTransformCom->LookAt(XMVectorSetY(vTargetPos, XMVectorGetY(vMyPos)));
}

void CBoss_MadDonkey::End_Dialogue()
{
	if (false == m_bTalking)
		return;

	m_bTalking = false;

	Set_AnimBool(L"Talk", false);

#ifdef _DEBUG
	OutputDebugStringW(L"[MadDonkey] End_Dialogue -> Dialog_End\n");
#endif
}

void CBoss_MadDonkey::Open_TargetDoor()
{
	if (UINT_MAX == m_iTargetDoorSpawnID)
		return;

	MAP_GAMEPLAY_ACTIVATE_EVENT_DESC EventDesc{};
	EventDesc.iTargetSpawnID = m_iTargetDoorSpawnID;

	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::DOOR_ACTIVATE, &EventDesc);

#ifdef _DEBUG
	_tchar szLog[96];
	swprintf_s(szLog, L"[MadDonkey] DOOR_ACTIVATE -> SpawnID %u\n", m_iTargetDoorSpawnID);
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Begin_Exit()
{
	if (true == m_bExiting || nullptr == m_pTransformCom)
		return;

	m_bExiting = true;
	m_fExitTimer = EXIT_WALK_TIME;

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Stop_MoveTo();

#ifdef _DEBUG
	_tchar szLog[160];
	swprintf_s(szLog, L"[MadDonkey] Begin_Exit  goal=(%.2f, %.2f, %.2f)\n",
		m_vExitGoal.x, m_vExitGoal.y, m_vExitGoal.z);
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Drive_Exit(_float fTimeDelta)
{
	Set_AnimBool(L"Turn", false);

	if (m_fExitTimer <= 0.f)
	{
		Set_AnimFloat(L"Speed", 0.f);

		if (false == m_bDissolving)
			Begin_Dissolve(0.f, EXIT_DISSOLVE_DURATION);

		return;
	}

	m_fExitTimer -= fTimeDelta;

	if (nullptr == m_pMovementCom)
		return;

	const _vector vGoal = XMLoadFloat3(&m_vExitGoal);

	Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
	Set_AnimFloat(L"Speed", 1.f);

	m_pMovementCom->TurnTo(vGoal, fTimeDelta);
	m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
	m_pMovementCom->Set_MoveTarget(vGoal);
}

void CBoss_MadDonkey::Begin_Dissolve(_float fStartDelay, _float fDuration)
{
	if (nullptr == m_pDissolveCom || true == m_bDissolving)
		return;

	m_pDissolveCom->Reset();
	m_pDissolveCom->Set_Timing(fStartDelay, fDuration, 0.f);
	m_pDissolveCom->Set_Color(DISSOLVE_EDGE_COLOR);
	m_pDissolveCom->Begin(CDissolve::DISSOLVE_MODE::DISSOLVE_OUT);

	m_bDissolving = true;
}

void CBoss_MadDonkey::Update_Dissolve(_float fTimeDelta)
{
	if (nullptr == m_pDissolveCom || false == m_bDissolving)
		return;

	m_pDissolveCom->Update(fTimeDelta);

	Set_Dissolve(m_pDissolveCom->Get_Amount(), m_pDissolveCom->Get_Color());

	if (nullptr != m_pWeapon)
		m_pWeapon->Set_Dissolve(m_pDissolveCom->Get_Amount(), m_pDissolveCom->Get_Color());

	if (false == m_pDissolveCom->Is_Finished())
		return;

	m_bDissolving = false;

	if (true == m_bDropPending)
	{
		m_bDropPending = false;
		Spawn_DropItem();
	}

	Request_Despawn();
}

void CBoss_MadDonkey::Spawn_DropItem()
{
	if (nullptr == m_pGameInstance || nullptr == m_pTransformCom)
		return;

	_float3 vPosition;
	XMStoreFloat3(&vPosition, m_pTransformCom->Get_State(STATE::POSITION));

	CDropItem::DROP_ITEM_DESC Desc{};
	Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	Desc.vTranslate = _float4(vPosition.x, vPosition.y, vPosition.z, 1.f);
	Desc.eItemType = ITEM_TYPE::BODY_PART;
	Desc.strItemName = DROP_ITEM_NAME;
	Desc.iItemCount = 1;
	Desc.vTriggerHalfExtents = _float3(2.f, 2.f, 2.f);

	CDropItem* pItem = m_pGameInstance->Clone_Prototype<CDropItem>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_DropItem"), &Desc);

	if (nullptr == pItem)
		return;

	if (FAILED(m_pGameInstance->Add_GameObject(static_cast<_uint>(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_MapObject"), pItem)))
		Safe_Release(pItem);
}

void CBoss_MadDonkey::Apply_SpawnDesc(void* pArg)
{
	__super::Apply_SpawnDesc(pArg);

	if (nullptr == pArg)
		return;

	auto pDesc = static_cast<MONSTER_DESC*>(pArg);

	m_iSpawnID = pDesc->iSpawnID;
	m_iSpawnNumber = (0 == pDesc->iSpawnNumber) ? 1 : pDesc->iSpawnNumber;
	m_iTargetDoorSpawnID = pDesc->iTargetDoorSpawnID;
	m_vExitGoal = pDesc->vExitGoalPos;

	// SpawnNumber로 보스와 문지기가 갈리므로 ActorType과 이동속도도 매번 다시 정한다.
	m_eActorType = Is_Gatekeeper() ? ACTOR_TYPE::MONSTER : ACTOR_TYPE::BOSS;

	if (nullptr != m_pMovementCom)
	{
		m_pMovementCom->Set_Speed(SPEED_TYPE::RUN, pDesc->fRunSpeed);
		m_pMovementCom->Set_Speed(SPEED_TYPE::SPRINT, pDesc->fSprintSpeed);
	}
}

void CBoss_MadDonkey::Begin_Battle()
{
	if (true == m_bBattleStarted)
		return;

	m_bBattleStarted = true;

	if (m_bTalking)
		m_pGameInstance->Broadcast(EVENT_TYPE::UI_END, nullptr);

	m_bTalking = false;

	Set_AnimBool(L"Talk", false);
	Set_AnimBool(L"Spawn", true);

	m_bSpawned = true;
	m_bAggro = true;
	m_bDetected = true;

	m_bChasing = (m_fDistToTarget > m_fFarAttackRange);
	m_fChaseSwitchTimer = 0.f;

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Stop_MoveTo();

	m_pBlackBoard->Set<_bool>(L"bSpawned", true);
	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", true);

	m_pGameInstance->Play_BGM(BGM_BATTLE_KEY, 0.4f, BGM_FADE_SEC);

#ifdef _DEBUG
	OutputDebugStringW(L"[MadDonkey] Begin_Battle\n");
#endif
}

void CBoss_MadDonkey::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	if (EVENT_TYPE::BOSS_START == eType)
	{
		if (false == Is_MyBossStart(pData))
			return;

		if (true == Is_Gatekeeper())
			return;

		Begin_Battle();
		return;
	}

	if (EVENT_TYPE::UI_START == eType)
	{
		if (nullptr == pData)
		{
#ifdef _DEBUG
			OutputDebugStringW(L"[MadDonkey] UI_START payload null - ignored\n");
#endif
			return;
		}

		const UISTART_EVENT_DESC* pDesc = static_cast<const UISTART_EVENT_DESC*>(pData);

		if (m_iSpawnID != pDesc->iTargetSpawnID)
			return;

		Begin_Dialogue();
		return;
	}
}

void CBoss_MadDonkey::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	if (EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST == eType)
	{
		const NPC_DIALOGUE_SERVICE_REQUEST_DESC* pRequest = static_cast<const NPC_DIALOGUE_SERVICE_REQUEST_DESC*>(pData);

		if (nullptr == pRequest || m_iSpawnID != pRequest->iTargetSpawnID || NPC_TYPE::MADDONKEY != pRequest->eNPCType)
			return;

		if (NPC_DIALOGUE_SERVICE_TYPE::DONKEY_GATE_ANSWER == pRequest->eServiceType)
			m_bGateAnswered = true;

		return;
	}

	if (EVENT_TYPE_MAP_OBJECT::NPC_TALK_END != eType || nullptr == pData)
		return;

	const CNPC::NPC_DIALOGUE_EVENT_DESC* pDesc = static_cast<const CNPC::NPC_DIALOGUE_EVENT_DESC*>(pData);

	if (m_iSpawnID != pDesc->iTargetSpawnID || NPC_TYPE::MADDONKEY != pDesc->eNPCType)
		return;

	if (true == Is_Gatekeeper() && true == m_bGateAnswered && false == m_bGateChecked)
	{
		m_bGateChecked = true;
		m_bTalking = false;

		Begin_Dialogue();
		return;
	}

	End_Dialogue();

	if (false == Is_Gatekeeper() || true == m_bExiting || true == m_bExitPending)
		return;

	if (false == Has_GatePass())
		return;

	m_bExitPending = true;
}

void CBoss_MadDonkey::Action_Notify(_uint64 iHashKey)
{
	static const _uint64 iBattleStart = Hash_String(L"Battle_Start");
	static const _uint64 iBackToSpawn = Hash_String(L"BackToSpawn");

	if (iHashKey == iBackToSpawn)
	{
		Request_AnimAction(L"SPAWN");
		return;
	}

	if (iHashKey == iBattleStart)
	{
		if (true == Is_Gatekeeper())
			return;

		Begin_Battle();
		Broadcast_BossStart();
		return;
	}

	auto itType = m_HitTypeKeys.find(iHashKey);
	if (itType != m_HitTypeKeys.end())
	{
		m_eCurHitType = itType->second;
		return;
	}

	auto it = m_NotifyKeys.find(iHashKey);
	if (it == m_NotifyKeys.end())
		return;

	switch (it->second)
	{
	case NOTIFY_KEY::WEAPON_ON:
	{
		if (nullptr == m_pWeapon)
			break;

		_float fDamage = -1.f;
		HIT_TYPE eType = m_eCurHitType;

		if (nullptr != m_pAnimator)
		{
			auto ita = m_AttackData.find(m_pAnimator->Get_CurControllerStateHash());
			if (ita != m_AttackData.end())
			{
				if (ita->second.fDamage >= 0.f)
					fDamage = ita->second.fDamage;
				if (ita->second.eHitType != HIT_TYPE::Normal)
					eType = ita->second.eHitType;
			}
		}

		m_pWeapon->Attack_Begin(fDamage, eType);
	}
	break;

	case NOTIFY_KEY::WEAPON_OFF:
		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();
		break;

	case NOTIFY_KEY::SET_DEAD:
		m_bDead = true;
		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();
		Stop_AllLoopEffects();
		__super::On_SetDead();
		break;

	case NOTIFY_KEY::CHECK_DEAD:
		On_CheckDead();
		break;

	case NOTIFY_KEY::FATAL_READY:
	{
		const _vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
		const _vector vOff = XMVector3Rotate(XMLoadFloat3(&m_vFatalOffsetFront), m_pTransformCom->Get_Quat());

		_float3 vWorldPos;
		XMStoreFloat3(&vWorldPos, XMVectorAdd(vPos, vOff));
		vWorldPos.y += 0.1f;

		Start_LoopEffect_World(SYMBOL_FX_KEY, "Effect_Fatal_Symbol", vWorldPos);

		m_bFatalReady = true;
		Begin_GroggyWindow(m_fGroggyWindowFallback);
	}
	break;

	default:
		break;
	}
}

void CBoss_MadDonkey::On_Groggy(_bool bGroggy)
{
	if (true == bGroggy)
	{
		Cancel_CombatState();
		return;
	}

	Stop_LoopImmediate(SYMBOL_FX_KEY);
	m_bFatalReady = false;
}

void CBoss_MadDonkey::On_Groggy_Triggered()
{
	Cancel_CombatState();
}

void CBoss_MadDonkey::On_Freeze(_bool bFreeze)
{
	if (false == bFreeze)
		return;

	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();
}

void CBoss_MadDonkey::On_Perceive_Attacker(_bool bPreFatal)
{
	if (true == Is_Gatekeeper())
		return;

	__super::On_Perceive_Attacker(bPreFatal);

	if (true == bPreFatal || true == m_bBattleStarted)
		return;

	Begin_Battle();
	Broadcast_BossStart();
}

void CBoss_MadDonkey::Broadcast_BossStart()
{
	BossClear_EventDesc StartDesc{};
	StartDesc.eMonsterType = m_eMonsterType;

	m_pGameInstance->Broadcast(EVENT_TYPE::BOSS_START, &StartDesc);
}

_bool CBoss_MadDonkey::Take_Damage(const HitInfo& hitInfo)
{
	if (true == Is_Gatekeeper())
		return false;

	return __super::Take_Damage(hitInfo);
}

_bool CBoss_MadDonkey::Can_FatalFront() const
{
	if (true == Is_Gatekeeper())
		return false;

	if (m_pBlackBoard->Get<_bool>(L"bDead") || m_pBlackBoard->Get<_bool>(L"bFatal"))
		return false;

	if (m_pStatCom->Is_OnGroggyCooldown())
		return false;

	return m_bFatalReady;
}

_bool CBoss_MadDonkey::Can_FatalBack() const
{
	return false;
}

void CBoss_MadDonkey::Cancel_CombatState()
{
	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();

	m_eCurHitType = HIT_TYPE::Normal;

	m_bMoving = false;
	m_bDashMove = false;
	m_bMustAttack = false;
	m_bApproaching = false;
	m_iPendingPatternID = -1;
	m_bLinkFired = true;
	m_fIdleTimer = 0.f;
}

void CBoss_MadDonkey::On_CheckDead()
{
	m_pStatCom->Set_FatalDying(false);

	if (Is_Dead())
		m_bDead = true;

	Set_Groggy(false);
	m_pBlackBoard->Set<_bool>(L"bFatal", false);
	Stop_LoopImmediate(SYMBOL_FX_KEY);
	m_bFatalReady = false;

	m_pStatCom->Reset_Groggy();
	m_pStatCom->Set_Staggerable(false);

	Request_AnimAction(L"FATAL_AFTER");
}

void CBoss_MadDonkey::Record_HitType(const HitInfo& hitInfo)
{
	__super::Record_HitType(hitInfo);

	if (HIT_TYPE::KnockBack != hitInfo.eHitType)
		return;

	Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));
	m_bKnockbackPending = true;
}

void CBoss_MadDonkey::On_SuperArmor(_bool bOn)
{
	if (true == bOn)
	{
		Set_RimProfile(1);
		Set_RimAmount(0.5f);

		if (nullptr != m_pWeapon)
		{
			m_pWeapon->Set_RimProfile(1);
			m_pWeapon->Set_RimAmount(0.5f);
		}
		return;
	}

	Set_RimAmount(0.f);

	if (nullptr != m_pWeapon)
		m_pWeapon->Set_RimAmount(0.f);
}

void CBoss_MadDonkey::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	Update_CombatParams(fTimeDelta);

	if (true == m_bExitPending)
	{
		if (true == Is_AnimInState(L"DIALOG_GATE_SUCCESS"))
		{
			Set_AnimFloat(L"Speed", 0.f);
			Mirror_ControllerParams();
			return;
		}

		m_bExitPending = false;

		Open_TargetDoor();
		Begin_Exit();
	}

	if (true == m_bExiting)
	{
		Drive_Exit(fTimeDelta);
		Mirror_ControllerParams();
		return;
	}

	if (false == m_bBattleStarted)
	{
		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();
		return;
	}

#ifdef _DEBUG
	if (true == m_bDbgMeasuring)
	{
		m_pBlackBoard->Set<_bool>(L"bAttackReady", false);
		m_fIdleTimer = 0.f;
		m_bMoving = false;
		Drive_Steering(fTimeDelta);
		Mirror_ControllerParams();
		return;
	}
#endif

	if (true == m_pBlackBoard->Get<_bool>(L"bFatal"))
	{
		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();
		return;
	}

	if (true == m_bDead || m_pStatCom->Is_Dead())
	{
		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();

		if (false == m_bDeadHandled)
		{
			m_bDeadHandled = true;

			BossClear_EventDesc ClearDesc{};
			ClearDesc.eMonsterType = SPAWN_MONSTER_TYPE::BOSS_DONKEY;
			m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, &ClearDesc);

			m_bDropPending = true;
			Begin_Dissolve(DEAD_DISSOLVE_DELAY, DEAD_DISSOLVE_DURATION);
		}

		const _bool bInDeadPose =
			Is_AnimInState(L"DEAD_B") || Is_AnimInState(L"DEAD_F") ||
			Is_AnimInState(L"FATAL_DEAD_B") || Is_AnimInState(L"FATAL_DEAD_F");
		if (false == bInDeadPose)
			Request_AnimAction(L"DEAD");

		return;
	}

	if (true == m_pBlackBoard->Get<_bool>(L"bGroggy"))
	{
		Set_AnimFloat(L"Speed", 0.f);
		m_pBlackBoard->Set<_bool>(L"bHit", false);
		m_fHitTimer = 0.f;
		m_bMoving = false;
		m_bMustAttack = false;
		m_bApproaching = false;
		m_iPendingPatternID = -1;
		Mirror_ControllerParams();
		return;
	}

	if (true == m_bKnockbackPending)
	{
		m_bKnockbackPending = false;

		m_bMoving = false;
		m_bMustAttack = false;
		m_bApproaching = false;
		m_iPendingPatternID = -1;
		m_bLinkFired = true;

		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();

		Fire_Knockback();
		Mirror_ControllerParams();
		return;
	}

	Update_ComboLink();
	Update_PatternSelect(fTimeDelta);
	Drive_Steering(fTimeDelta);
	Mirror_ControllerParams();
}

void CBoss_MadDonkey::Update_CombatParams(_float fTimeDelta)
{
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

	if (false == m_bBattleStarted)
	{
		m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", false);
		m_pBlackBoard->Set<_bool>(L"bInAttackRange", false);
		return;
	}

	const _float fReach = m_fFarAttackRange;

	m_fChaseSwitchTimer += fTimeDelta;
	if (m_fChaseSwitchTimer >= m_fChaseDwell)
	{
		if (false == m_bChasing && m_fDistToTarget > fReach + m_fChaseMargin)
		{
			m_bChasing = true;
			m_fChaseSwitchTimer = 0.f;
		}
		else if (true == m_bChasing && m_fDistToTarget <= fReach)
		{
			m_bChasing = false;
			m_fChaseSwitchTimer = 0.f;
		}
	}

	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", true);
	m_pBlackBoard->Set<_bool>(L"bInAttackRange", false == m_bChasing);
}

void CBoss_MadDonkey::Mirror_ControllerParams()
{
	Set_AnimBool(L"Dead", (m_pBlackBoard->Get<_bool>(L"bDead") || m_bDead) && false == m_pStatCom->Is_FatalDying());
	Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
	Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
	Set_AnimBool(L"TargetCheck", m_pBlackBoard->Get<_bool>(L"bPlayerInDetect") && m_pBlackBoard->Get<_bool>(L"bPlayerInFront"));
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
	Set_AnimBool(L"Spawn", m_bBattleStarted || m_bExiting || m_bExitPending);
}

void CBoss_MadDonkey::Update_PatternSelect(_float fTimeDelta)
{
	if (m_iPendingPatternID >= 0)
	{
		if (m_fDistToTarget > m_fPendingAttackLength + m_fAttackRangeTol)
		{
			m_bApproaching = true;
			return;
		}
		m_bApproaching = false;
		m_iCurPatternID = m_iPendingPatternID;
		m_iPendingPatternID = -1;
		m_iLinkOwnerID = m_iCurPatternID;
		m_bLinkFired = false;
		Fire_Attack();
		return;
	}

	if (true == m_bMoving)
	{
		m_fMoveTimer -= fTimeDelta;

		const _bool bDashDone = (true == m_bDashMove)
			&& false == Is_AnimInState(L"DASH_B")
			&& false == Is_AnimInState(L"DASH_L")
			&& false == Is_AnimInState(L"DASH_R");

		if (m_fMoveTimer <= 0.f || true == bDashDone)
		{
			m_bMoving = false;
			m_bDashMove = false;
			m_bMustAttack = true;
			m_fIdleTimer = m_fAttackGap;
		}
		return;
	}

	const _bool bIdle = Is_AnimIdle();
	m_fIdleTimer = bIdle ? (m_fIdleTimer + fTimeDelta) : 0.f;

	if (false == bIdle || m_fIdleTimer < m_fAttackGap)
		return;

	const _int iAct = Pick_Action(m_fDistToTarget, false == m_bMustAttack);

	if (iAct < PICK_COUNT)
	{
		m_bMustAttack = false;
		m_iLastPatternID = s_Picks[iAct].iID;
		Begin_Pattern(s_Picks[iAct].iID, s_Picks[iAct].fAttackLength);

#ifdef _DEBUG
		_tchar szLog[128];
		swprintf_s(szLog, L"[DonkeyBegin] P%d  len=%.1f  dist=%.2f\n",
			s_Picks[iAct].iID, s_Picks[iAct].fAttackLength, m_fDistToTarget);
		OutputDebugStringW(szLog);
#endif
		return;
	}

	Fire_FinishMove();
}

_int CBoss_MadDonkey::Pick_Action(_float fDist, _bool bAllowMove)
{
	_float fW[PICK_COUNT + 1] = {};
	_float fTotal = 0.f;

	for (_int i = 0; i < PICK_COUNT; ++i)
	{
		_float w = s_Picks[i].fWeight;

		if (fDist < s_Picks[i].fBandMin || fDist > s_Picks[i].fBandMax)
			w *= m_fOffBandScale;

		if (s_Picks[i].iID == m_iLastPatternID)
			w *= m_fRepeatPenalty;

		fW[i] = w;
		fTotal += w;
	}

	if (fTotal <= 0.f)
		return PICK_COUNT;

	if (true == bAllowMove)
	{
		fW[PICK_COUNT] = m_fMoveWeight;
		fTotal += m_fMoveWeight;
	}

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	_int iPick = PICK_COUNT;
	for (_int i = 0; i <= PICK_COUNT; ++i)
	{
		r -= fW[i];
		if (r <= 0.f) { iPick = i; break; }
	}

#ifdef _DEBUG
	_tchar szLog[256];
	_tchar szW[192] = {};
	for (_int i = 0; i < PICK_COUNT; ++i)
	{
		_tchar szOne[32];
		swprintf_s(szOne, L"P%d %.0f%%  ", s_Picks[i].iID, 100.f * fW[i] / fTotal);
		wcscat_s(szW, szOne);
	}
	swprintf_s(szLog, L"[DonkeyPick] dist=%.2f  %sMOVE %.0f%%  -> %s\n",
		fDist, szW, 100.f * fW[PICK_COUNT] / fTotal,
		(iPick < PICK_COUNT) ? L"ATTACK" : L"MOVE");
	OutputDebugStringW(szLog);
#endif

	return iPick;
}

void CBoss_MadDonkey::Fire_Attack()
{
	Set_AnimInt(L"PatternID", m_iCurPatternID);
	Set_AnimTrigger(L"PatternStart");
	Request_AnimAction(L"ATTACK");
	m_fIdleTimer = 0.f;

#ifdef _DEBUG
	_tchar szLog[128];
	swprintf_s(szLog, L"[DonkeyFire] P%d  dist=%.2f\n", m_iCurPatternID, m_fDistToTarget);
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Fire_FinishMove()
{
	if (m_fDistToTarget <= m_fDashBackRange)
	{
		m_iMoveDir = static_cast<_int>(MON_DIRECTION::BACK);
		Set_AnimInt(L"Direction", m_iMoveDir);
		Request_AnimAction(L"DASH");
		m_bDashMove = true;
		m_bMoving = true;
		m_fMoveTimer = m_fMoveDuration;
		m_fIdleTimer = 0.f;

#ifdef _DEBUG
		OutputDebugStringW(L"[DonkeyMove] DASH_B\n");
#endif
		return;
	}

	const _int iRoll = rand() % 4;
	m_iMoveDir = (0 == (iRoll & 1)) ? static_cast<_int>(MON_DIRECTION::LEFT)
		: static_cast<_int>(MON_DIRECTION::RIGHT);

	m_bMoving = true;
	m_fMoveTimer = m_fMoveDuration;
	m_fIdleTimer = 0.f;

	m_bDashMove = (iRoll < 2);

	if (true == m_bDashMove)
	{
		Set_AnimInt(L"Direction", m_iMoveDir);
		Request_AnimAction(L"DASH");
	}

#ifdef _DEBUG
	_tchar szLog[96];
	swprintf_s(szLog, L"[DonkeyMove] %s %s\n",
		m_bDashMove ? L"DASH" : L"WALK",
		(static_cast<_int>(MON_DIRECTION::LEFT) == m_iMoveDir) ? L"LEFT" : L"RIGHT");
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Fire_Knockback()
{
	const _int iHitInfo = static_cast<_int>(m_pStatCom->Get_LastHitType());
	Set_AnimInt(L"Direction", m_iHitDir);
	Set_AnimInt(L"HitInfo", iHitInfo);
	Set_AnimTrigger(L"DamageTrigger");
	Request_AnimAction(L"DAMAGE");
}

void CBoss_MadDonkey::Update_ComboLink()
{
	if (nullptr == m_pAnimator)
		return;

	if (m_iCurPatternID != m_iLinkOwnerID)
	{
		m_iLinkOwnerID = m_iCurPatternID;
		m_bLinkFired = false;
	}

	if (true == m_bLinkFired || false == Is_InAttackState())
		return;

	const _uint iFrame = m_pAnimator->Get_LocalFrame();
	const _bool bEnded = m_pAnimator->Is_Ended();

	for (_int i = 0; i < LINK_COUNT; ++i)
	{
		const DONKEY_LINK& Link = s_Links[i];

		if (Link.iFrom != m_iCurPatternID)
			continue;

		if (LINK_ON_CLIP_END == Link.iFrameStart)
		{
			if (false == bEnded)
				continue;
		}
		else
		{
			if (static_cast<_int>(iFrame) < Link.iFrameStart || static_cast<_int>(iFrame) > Link.iFrameEnd)
				continue;
		}

		Fire_ComboLink(Link.iTo);
		return;
	}
}

void CBoss_MadDonkey::Fire_ComboLink(_int iPatternID)
{
	m_bLinkFired = true;
	m_iLastPatternID = m_iCurPatternID;
	m_iCurPatternID = iPatternID;

	m_iPendingPatternID = -1;
	m_bApproaching = false;
	m_bMoving = false;
	m_bMustAttack = false;
	m_fIdleTimer = 0.f;

	Set_AnimInt(L"PatternID", iPatternID);
	Set_AnimTrigger(L"PatternStart");
	Request_AnimAction(L"ATTACK");

#ifdef _DEBUG
	_tchar szLog[160];
	swprintf_s(szLog, L"[DonkeyLink] %d -> %d  dist=%.2f\n", m_iLastPatternID, iPatternID, m_fDistToTarget);
	OutputDebugStringW(szLog);
#endif
}

void CBoss_MadDonkey::Drive_Steering(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);
	Set_AnimBool(L"Turn", false);

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") ||
		m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bDead"))
	{
		m_bTurning = false;
		return;
	}

	if (false == m_bBattleStarted || nullptr == m_pMovementCom)
	{
		m_bTurning = false;
		return;
	}

	_vector vTargetPos;
	const _bool bHasTarget = Get_TargetPos(vTargetPos);

	if (true == m_bChasing)
	{
		m_bTurning = false;

		if (true == bHasTarget && true == Can_AnimMove())
		{
			const _bool bFar = (m_fDistToTarget > m_fRunChaseRange);
			Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
			Set_AnimFloat(L"Speed", bFar ? 3.f : 1.f);
			m_pMovementCom->Set_SpeedType(bFar ? SPEED_TYPE::SPRINT : SPEED_TYPE::RUN);
			m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
			m_pMovementCom->Set_MoveTarget(vTargetPos);
		}
		return;
	}

	if (true == bHasTarget)
		Drive_Facing(vTargetPos, fTimeDelta);
	else
		m_bTurning = false;

	if (true == m_bTurning)
		return;

	if (true == m_bApproaching && true == bHasTarget && true == Can_AnimMove())
	{
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
		Set_AnimFloat(L"Speed", 3.f);
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
		m_pMovementCom->Set_MoveTarget(vTargetPos);
		return;
	}

	if (true == m_bMoving && true == Can_AnimMove())
	{
		Set_AnimInt(L"Direction", m_iMoveDir);
		Set_AnimFloat(L"Speed", 1.f);
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);

		switch (static_cast<MON_DIRECTION>(m_iMoveDir))
		{
		case MON_DIRECTION::LEFT:  m_pMovementCom->Orbit(0, vTargetPos, fTimeDelta); break;
		case MON_DIRECTION::RIGHT: m_pMovementCom->Orbit(1, vTargetPos, fTimeDelta); break;
		default: break;
		}
	}
}

void CBoss_MadDonkey::Drive_Facing(_fvector vTargetPos, _float fTimeDelta)
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

_bool CBoss_MadDonkey::Is_AnimIdle() const
{
	return Is_AnimInState(L"IDLE");
}

_bool CBoss_MadDonkey::Is_InAttackState() const
{
	if (nullptr == m_pAnimator)
		return false;

	const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();
	if (nullptr == pState)
		return false;

	static const _uint64 iHash[11] =
	{
		Hash_String(L"ATTACK_01"), Hash_String(L"ATTACK_02"), Hash_String(L"ATTACK_03"),
		Hash_String(L"ATTACK_04"), Hash_String(L"ATTACK_05"), Hash_String(L"ATTACK_06"),
		Hash_String(L"ATTACK_07"), Hash_String(L"COMBO_01"),  Hash_String(L"COMBO_02"),
		Hash_String(L"COMBO_03"),  Hash_String(L"COMBO_04"),
	};

	for (_int i = 0; i < 11; ++i)
		if (pState->iNameHash == iHash[i])
			return true;

	return false;
}

#ifdef _DEBUG
void CBoss_MadDonkey::Debug_Input(_float fTimeDelta)
{
	if (true == m_pGameInstance->Key_Down(DIK_NUMPAD8))
		Begin_Dialogue();

	if (true == m_pGameInstance->Key_Down(DIK_NUMPAD9))
		End_Dialogue();
}

void CBoss_MadDonkey::Debug_ResetToOrigin()
{
	const _vector vOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	if (nullptr != m_pCCT)
		m_pCCT->Set_FootPosition(vOrigin);

	if (nullptr != m_pTransformCom)
		m_pTransformCom->Set_State(STATE::POSITION, vOrigin);

	Cancel_All_Attacks();

	m_bDbgMeasuring = false;
	m_bDbgStarted = false;
	m_iDbgFireDelay = 0;
	m_bMoving = false;
	m_bMustAttack = false;
	m_iPendingPatternID = -1;
	m_bApproaching = false;

	OutputDebugStringW(L"[DonkeyRuler] reset to origin\n");
}

void CBoss_MadDonkey::Debug_MeasureAttack(_float fTimeDelta)
{
	if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
		return;

	if (true == m_pGameInstance->Key_Down(DIK_MULTIPLY))
	{
		Debug_ResetToOrigin();
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

		m_iCurPatternID = m_iDbgAttackID;
		m_iLinkOwnerID = m_iDbgAttackID;
		m_bLinkFired = true;

		Set_AnimInt(L"PatternID", m_iDbgAttackID);
		Set_AnimTrigger(L"PatternStart");
		Request_AnimAction(L"ATTACK");

		_tchar szFire[160];
		swprintf_s(szFire, L"[DonkeyRuler] fire P%d  startDist=%.2f\n", m_iDbgAttackID, m_fDbgStartDist);
		OutputDebugStringW(szFire);
		return;
	}

	if (false == m_bDbgMeasuring)
	{
		static const _ubyte byFireKeys[PICK_COUNT] =
		{
			DIK_NUMPAD1, DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPAD4,
			DIK_NUMPAD5, DIK_NUMPAD6, DIK_NUMPAD7
		};

		for (_int i = 0; i < PICK_COUNT; ++i)
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
			m_bDbgStarted = false;
			m_bDbgMeasuring = true;
			m_iDbgFireDelay = 2;

			m_bMoving = false;
			m_bMustAttack = false;
			m_bApproaching = false;
			m_iPendingPatternID = -1;
			break;
		}
		return;
	}

	m_fDbgTimer += fTimeDelta;
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

	if (true == Is_HitActive())
	{
		if (m_fDbgLungeAtHit < -9998.f)
			m_fDbgLungeAtHit = fLunge;

		if (nullptr != m_pWeapon)
		{
			_matrix matTop, matBot;
			if (SUCCEEDED(static_cast<CWeapon_Melee*>(m_pWeapon)->Get_TrailMatrix(matTop, matBot)))
			{
				const _vector vTips[2] = { matTop.r[3], matBot.r[3] };
				for (_int k = 0; k < 2; ++k)
				{
					const _float fFlat = XMVectorGetX(XMVector3Length(
						XMVectorSetY(XMVectorSubtract(vTips[k], vStart), 0.f)));
					if (fFlat + m_fSweepRadius > m_fDbgReach)
						m_fDbgReach = fFlat + m_fSweepRadius;
				}
			}
		}
	}

	const _bool bDone = (true == m_bDbgStarted && true == Is_AnimIdle());
	const _bool bTimeout = (m_fDbgTimer > 20.f);

	if (false == bDone && false == bTimeout)
		return;

	_tchar szLog[384];
	swprintf_s(szLog, L"[DonkeyRuler] P%d  scale=%.2f  startDist=%.2f  reach=%+.2f  lungeAtHit=%+.2f  lungeMax=%+.2f  t=%.2fs%s\n",
		m_iDbgAttackID, m_fDbgScaleMax, m_fDbgStartDist,
		m_fDbgReach, m_fDbgLungeAtHit, m_fDbgLunge, m_fDbgTimer,
		bTimeout ? L"  (TIMEOUT)" : L"");
	OutputDebugStringW(szLog);

	if (m_fDbgReach < -9998.f)
	{
		if (false == m_bDbgStarted)
			OutputDebugStringW(L"                 ATTACK_STATE_NOT_ENTERED (PatternID / ActionStateMaps 확인)\n");
		else
			OutputDebugStringW(L"                 NO_HITACTIVE\n");
	}

	m_bDbgMeasuring = false;
}
#endif

CBoss_MadDonkey* CBoss_MadDonkey::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBoss_MadDonkey* pInstance = new CBoss_MadDonkey(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBoss_MadDonkey");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBoss_MadDonkey::Clone(void* pArg)
{
	CBoss_MadDonkey* pInstance = new CBoss_MadDonkey(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBoss_MadDonkey");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBoss_MadDonkey::Free()
{
	m_pGameInstance->UnSubscribe_AllEvent(this);

	Safe_Release(m_pDissolveCom);

	__super::Free();
}
