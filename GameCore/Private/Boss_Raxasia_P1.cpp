#include "Boss_Raxasia_P1.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "CCT.h"
#include "Model.h"
#include "Mesh.h"
#include "Animator.h"

#include "Monster_Defines.h"
#include "Monster_Include.h"
#include "Player.h"
#include "Effect.h"
#include "TargetComponent.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CBoss_Raxasia_P1, CAIObject)
ADD_PROPERTY_SETTER(CBoss_Raxasia_P1, m_iDbgHitIdx, Int, Set_DebugHitIndex, _int)
ADD_PROPERTY_SETTER(CBoss_Raxasia_P1, m_fDbgHitRadius, Float, Set_DebugHitRadius, _float)
ADD_PROPERTY_SETTER(CBoss_Raxasia_P1, m_fDbgHitHalf, Float, Set_DebugHitHalf, _float)
END_REFLECTION(CBoss_Raxasia_P1)
#endif

namespace
{
    struct RAXA_PAT
    {
        _int   iID;
        _float fNear;
        _float fFar;
        _float fWeight;
    };

    const RAXA_PAT s_P1[] = {
        {  1, 0.5f,  6.3f, 1.0f },
        {  2, 7.8f, 12.3f, 0.7f },
        {  3, 5.7f, 11.5f, 0.6f },
        {  4, 5.7f, 11.5f, 0.6f },
        {  5, 1.2f,  5.4f, 1.0f },
        {  6, 1.4f,  4.0f, 0.9f },
        {  7, 4.8f,  9.6f, 0.8f },
        {  8, 2.5f,  9.7f, 0.9f },
        {  9, 2.1f,  4.4f, 0.8f },
        { 10, 3.7f,  9.2f, 0.5f },
    };

    const string BGM_BATTLE_INTRO_P1 = "MU_MS_Rocksasia_PH1_Intro_16_";
    const string BGM_BATTLE_KEY_P1 = "MU_MS_Rocksasia_PH1";
    const string CUTSCENE_RAXASIA_PHASE2 = "Raxasia_Phase2";

    // 원본이 RMS -22dBFS 로 다른 곡보다 6~9dB 낮게 마스터링돼 있어 볼륨으로 보정한다.
    const _float BGM_BATTLE_VOLUME_P1 = 1.f;
    const _float BGM_FADE_SEC = 1.5f;
}
CBoss_Raxasia_P1::CBoss_Raxasia_P1(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CAIObject{ pDevice, pContext }
{
    m_eActorType = ACTOR_TYPE::BOSS;
    m_strMonsterName = L"완성된 자 락사시아";
}

CBoss_Raxasia_P1::CBoss_Raxasia_P1(const CBoss_Raxasia_P1& Prototype)
    : CAIObject{ Prototype }
    , m_EffectKeys{ Prototype.m_EffectKeys }
{
    m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBoss_Raxasia_P1::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Raxasia_P1.json";

    m_NotifyKeys[Hash_String(L"SET_DEAD")] = NOTIFY_KEY::SET_DEAD;
    m_NotifyKeys[Hash_String(L"Symbol_Start")] = NOTIFY_KEY::FATAL_READY;
    m_NotifyKeys[Hash_String(L"NextMotion")] = NOTIFY_KEY::NEXTMOTION;
    m_NotifyKeys[Hash_String(L"HitOn_BoltRange")] = NOTIFY_KEY::BOLT_RANGE;
    m_NotifyKeys[Hash_String(L"Land_BoltMine")] = NOTIFY_KEY::LAND_MINE;
    m_NotifyKeys[Hash_String(L"Land_MineStart")] = NOTIFY_KEY::LAND_MINESTART;
    m_NotifyKeys[Hash_String(L"Land_MineEnd")] = NOTIFY_KEY::LAND_MINEEND;
    m_NotifyKeys[Hash_String(L"Land_BoltSector")] = NOTIFY_KEY::LAND_SECTOR;
    m_NotifyKeys[Hash_String(L"Player_Grab")] = NOTIFY_KEY::PLAYER_GRAB;
    m_NotifyKeys[Hash_String(L"Player_Throw")] = NOTIFY_KEY::PLAYER_THROW;
    m_NotifyKeys[Hash_String(L"ShieldBreak")] = NOTIFY_KEY::SHEILD_BREAK;
    m_NotifyKeys[Hash_String(L"Sword_Thunder_On")] = NOTIFY_KEY::SWORD_THUNDER_ON;
    m_NotifyKeys[Hash_String(L"Sword_Thunder_Off")] = NOTIFY_KEY::SWORD_THUNDER_OFF;
    m_NotifyKeys[Hash_String(L"Trail_On")] = NOTIFY_KEY::TRAIL_ON;
    m_NotifyKeys[Hash_String(L"Trail_Off")] = NOTIFY_KEY::TRAIL_OFF;
    m_NotifyKeys[Hash_String(L"Elec_Trail_On")] = NOTIFY_KEY::ELEC_TRAIL_ON;
    m_EffectKeys[EFFECT_KEY::FURY] = Hash_String(L"LOOP_FURY");
    m_EffectKeys[EFFECT_KEY::SYMBOL] = Hash_String(L"LOOP_SYMBOL");

    return S_OK;
}

HRESULT CBoss_Raxasia_P1::Initialize(void* pArg)
{
    if (nullptr != pArg)
    {
        auto pDesc = static_cast<MONSTER_DESC*>(pArg);
        m_fShortAttackRange = pDesc->fShortAttackRange;
        m_fFarAttackRange = pDesc->fFarAttackRange;
        m_fAttackGap = pDesc->fAttackGap;
        Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });
    }

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    CAIObject::CHARACTER_SETUP_DESC SetupDesc{};
    SetupDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Raxasia_P1");
    SetupDesc.strAnimFolderTag = TEXT("Raxasia_P1");
    SetupDesc.strAnimTag = TEXT("Raxasia_P1");
    SetupDesc.pRootBoneName = "Root";
    SetupDesc.bUseCCT = true;
    SetupDesc.fCCTRadius = 0.8f;
    SetupDesc.fCCTHeight = 1.8f;
    SetupDesc.fRunSpeed = 1.25f;
    SetupDesc.fSprintSpeed = 2.0f;
    SetupDesc.fGroundStick = -2.f;

    if (nullptr != pArg)
    {
        auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);
        SetupDesc.iModelLevel = pDesc->iProtoLevel;
    }
    else
    {
        SetupDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
    }

    if (FAILED(Ready_Character(SetupDesc)))
        return E_FAIL;

    if (FAILED(Ready_Weapon()))
        return E_FAIL;

    if (FAILED(Ready_Shield()))
        return E_FAIL;

    if (FAILED(Ready_HitBoxes()))
        return E_FAIL;

    m_pMatGrabSocket = m_pModelCom->Get_Bone_CombinedMatPtr(m_GRAB_BONE_NAME.c_str());
    if (m_pMatGrabSocket == nullptr)
    {
        MSG_BOX("CBoss_Raxasia_P1::Initialize : Failed to Find Grab Bone");
        return E_FAIL;
    }

    Capture_SpawnPos();
    m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);

    Spawn();

    return S_OK;
}

void CBoss_Raxasia_P1::Update(_float fTimeDelta)
{
    // 컷신이 끝나기 전에는 실제 보스의 애니메이션과 AI를 갱신하지 않는다.
    if (false == m_bBattleStarted || Is_AISleep())
        return;

    __super::Update(fTimeDelta);

    if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
        Apply_Movment(fTimeDelta);
}

void CBoss_Raxasia_P1::Physical_Update(_float fFixedTime)
{
    // 컷신용 Actor와 겹쳐 있는 동안 실제 보스의 물리 처리를 막는다.
    if (false == m_bBattleStarted)
        return;

    __super::Physical_Update(fFixedTime);
}

void CBoss_Raxasia_P1::Late_Update(_float fTimeDelta)
{
    // Late_Update의 RenderGroup 등록도 막아 컷신 중에는 실제 보스를 한 개도 그리지 않는다.
    if (false == m_bBattleStarted || Is_AISleep())
        return;

    __super::Late_Update(fTimeDelta);

    if (Was_AnimStateChanged())
    {
        m_HitBoxes.On_StateChanged();
        m_HitBoxes.Set_HitType(HIT_TYPE::Normal);
    }
    m_HitBoxes.Update(m_pGameInstance, this, fTimeDelta);

    if (m_bWeaponLive && nullptr != m_pWeapon)
        m_pWeapon->Late_Update(fTimeDelta);

    if (nullptr != m_pShield)
        m_pShield->Late_Update(fTimeDelta);

#ifdef _DEBUG
    //Debug_BrainLog(fTimeDelta);
    Debug_RejectLog();
    Debug_MeasureAttack(fTimeDelta);
#endif
}

HRESULT CBoss_Raxasia_P1::Render()
{
    return __super::Render();
}

void CBoss_Raxasia_P1::Action_Notify(_uint64 iHashKey)
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
        switch (it->second)
        {
        case NOTIFY_KEY::SET_DEAD:		    On_P1_Fallen();			break;
        case NOTIFY_KEY::FATAL_READY:	    On_GroggyStart();		break;
        case NOTIFY_KEY::NEXTMOTION:	    Advance_SubPattern();	break;
        case NOTIFY_KEY::BOLT_RANGE:	    Fire_BoltRange();		break;
        case NOTIFY_KEY::LAND_MINE:		    Spawn_BoltMine();		break;
        case NOTIFY_KEY::LAND_SECTOR:	    Fire_BoltSector();		break;
        case NOTIFY_KEY::PLAYER_GRAB:	    Begin_Grab();			break;
        case NOTIFY_KEY::PLAYER_THROW:	    End_Grab();				break;
        case NOTIFY_KEY::SHEILD_BREAK:	    Break_Shield();			break;
        case NOTIFY_KEY::SWORD_THUNDER_ON:  Turn_On_Sword();        break;
        case NOTIFY_KEY::SWORD_THUNDER_OFF: Turn_Off_Sword();       break;
        case NOTIFY_KEY::TRAIL_ON:          Turn_On_Normal_Trail(); break;
        case NOTIFY_KEY::TRAIL_OFF:         Turn_Off_Trail();       break;
        case NOTIFY_KEY::ELEC_TRAIL_ON:     Turn_On_Elec_Trail();   break;
        default:												    break;
        }
        return;
    }

    m_HitBoxes.Handle_Notify(iHashKey);
}

void CBoss_Raxasia_P1::On_Groggy(_bool bGroggy)
{
    if (true == bGroggy)
    {
        m_HitBoxes.On_StateChanged();

        if (nullptr != m_pShield)
            m_pShield->Set_Physics_InScene(false);

        m_bAttackCommit = false;
        m_bAttackStarted = false;
        m_iPendingPatternID = -1;
        m_iSubPatternID = 0;
        m_bApproaching = false;
        m_bRetreating = false;
        m_bStrafing = false;
        m_bStrafeNext = true;
        m_fRepositionTimer = 0.f;
        m_fCommitTimer = 0.f;

        Set_AnimInt(L"PatternID", -1);
        Set_AnimInt(L"SubPattern", 0);
    }
    else
    {
        Stop_LoopImmediate(m_EffectKeys[EFFECT_KEY::SYMBOL]);
        m_bFatalReady = false;

        if (nullptr != m_pShield && false == m_pShield->Is_NoHp())
            m_pShield->Set_Physics_InScene(true);
    }
}

void CBoss_Raxasia_P1::On_SuperArmor(_bool bOn)
{
    if (bOn)
    {
        Set_RimAmount(0.5f);
        Set_RimProfile(1);
        if (nullptr != m_pFuryFx)
            m_pFuryFx->Stop();
        m_pFuryFx = m_pGameInstance->Play_Effect_Model("Effect_Fury_Attack_Model", m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
        m_pWeapon->Set_RimAmount(0.5f);
        m_pWeapon->Set_RimProfile(1);
        m_pShield->Set_RimAmount(0.5f);
        m_pShield->Set_RimProfile(1);
    }
    else
    {
        Set_RimAmount(0.f);
        if (nullptr != m_pFuryFx)
        {
            m_pFuryFx->Stop();
            m_pFuryFx = nullptr;
        }
        m_pWeapon->Set_RimAmount(0.f);
        m_pShield->Set_RimAmount(0.f);
    }
}

void CBoss_Raxasia_P1::On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash)
{
    __super::On_AnimStateChanged(iPrevStateHash, iCurStateHash);

    Turn_Off_Trail();
    Turn_Off_Sword();
}

void CBoss_Raxasia_P1::Begin_Battle()
{
    // 정상 컷신 종료 이벤트를 받은 프레임부터 실제 보스의 갱신과 렌더링을 허용한다.
    if (false == m_bBattleStarted)
    {
        Turn_On_AI();

        if (nullptr != m_pShield)
            m_pShield->Set_Physics_InScene(true);
    }

    m_bBattleStarted = true;
    m_bPerceived = true;
    m_bDetected = true;

    m_eBrain = BRAIN::INTRO_WALK;
    m_fIntroTimer = 0.f;
    m_bStrafing = false;
    m_bStrafeNext = false;
    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iPendingPatternID = -1;

    m_pGameInstance->Play_BGM_Sequence(BGM_BATTLE_INTRO_P1, BGM_BATTLE_KEY_P1, BGM_BATTLE_VOLUME_P1);
}

void CBoss_Raxasia_P1::OnEvent(EVENT_TYPE eType, Event_Desc* pAdata)
{
    switch (eType)
    {
    case Engine::EVENT_TYPE::START_LEVEL:
        break;
    case Engine::EVENT_TYPE::CAMERA_ACTION_BEGIN:
        break;
    case Engine::EVENT_TYPE::CAMERA_ACTION_END:
        break;
    case Engine::EVENT_TYPE::MONSTER_DEAD:
        break;
    case Engine::EVENT_TYPE::SECTOR_CHANGED:
        break;
    case Engine::EVENT_TYPE::BOSS_START:
    {
        if (true == Is_MyBossStart(pAdata))
            Begin_Battle();
    }
    break;
    case Engine::EVENT_TYPE::ACTOR_DESPAWN:
        break;
    case Engine::EVENT_TYPE::REQUEST_RESPAWN:
        break;
    case Engine::EVENT_TYPE::REQUEST_CHANGE_CHAPTER:
        break;
    case Engine::EVENT_TYPE::UPDATE_RESPAWN_POINT:
        break;
    case Engine::EVENT_TYPE::END:
        break;
    default:
        break;
    }
}

void CBoss_Raxasia_P1::Spawn()
{
    __super::Spawn();

    // Chapter에 미리 배치하되 BOSS_START 전까지 컷신 모델만 보이도록 대기한다.
    m_bBattleStarted = false;

    if (m_pShield)
        m_pShield->Reset_Shield();

    m_bShieldBreaking = false;
    m_fShieldBreakGuard = 0.f;

    m_bDead = false;
    m_bPerceived = false;
    m_bWeaponLive = true;

    m_eBrain = BRAIN::INTRO_WALK;
    m_iSubPatternID = 0;
    m_iCurPatternID = 1;
    m_iLastPatternID = 0;

    m_bStrafing = false;
    m_bStrafeNext = false;
    m_bAttackCommit = false;
    m_bAttackStarted = false;

    m_bRetreating = false;
    m_fRepositionTimer = 0.f;
    m_bFrontPrevValid = false;
    m_iRecentCount = 0;
    for (_int r = 0; r < RAXA_RECENT; ++r)
        m_iRecent[r] = 0;

    m_fIntroTimer = 0.f;
    m_fStrafeTimer = 0.f;
    m_fCommitTimer = 0.f;
    m_fIdleTimer = 0.f;

    m_bStingHit = false;
    m_fDistToTarget = 9999.f;
    m_fPendingFar = 0.f;
    m_fPendingNear = 0.f;
    m_iShowcaseIdx = 0;
    m_iStrafeDir = 0;

    // 대기 중에는 CCT·Animator·방패 물리까지 Scene에서 제외해 컷신 Actor와 충돌하지 않게 한다.
    Turn_Off_AI();

    if (nullptr != m_pShield)
        m_pShield->Set_Physics_InScene(false);

    if (nullptr != m_pTarget)
        static_cast<CPlayer*>(m_pTarget)->Get_TargetComponent()->Set_SearchRange(99.f);

    Turn_Off_Trail();
    Turn_Off_Sword();
    if (nullptr != m_pFuryFx)
    {
        m_pFuryFx->Stop();
        m_pFuryFx = nullptr;
    }
}

void CBoss_Raxasia_P1::Despawn()
{
    if (nullptr != m_pTarget)
        static_cast<CPlayer*>(m_pTarget)->Cancel_Grab();

    if (m_pShield)
        m_pShield->Set_Physics_InScene(false);

    __super::Despawn();

}

_bool CBoss_Raxasia_P1::Can_FatalBack() const
{
    return false;
}

void CBoss_Raxasia_P1::Apply_ShieldBlock(_float fDamage)
{
    if (nullptr == m_pStatCom || true == m_bDead)
        return;

    m_pStatCom->Apply_Damage(fDamage);
    m_pStatCom->Set_SinceHit(0.f);
}

void CBoss_Raxasia_P1::On_ShieldBroken()
{
    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iPendingPatternID = -1;
    m_iSubPatternID = 0;
    m_bStrafeNext = false;

    Set_AnimInt(L"PatternID", -1);
    Begin_StaggerMotion();
}

void CBoss_Raxasia_P1::On_Parried()
{
    if (8 != m_iPendingPatternID)
        return;

    if (true == m_bShieldBreaking)
        return;

    Begin_StaggerMotion();
}

void CBoss_Raxasia_P1::Set_FatalMode(_bool bMode)
{
    __super::Set_FatalMode(bMode);
}

void CBoss_Raxasia_P1::Handle_EffectNotify(const ANIM_NOTIFY* pNotify)
{
    static const _uint64 iGroundFxKey = Hash_String(L"GroundFx");

    if (nullptr != pNotify && pNotify->iNameHash == iGroundFxKey
        && false == pNotify->strPayload.empty())
    {
        _vector vPos;
        if (false == Get_BladeGroundPos(vPos))
            return;

        _float3 vGround;
        XMStoreFloat3(&vGround, vPos);
        m_pGameInstance->Play_Effect(pNotify->strPayload, vGround);
        return;
    }

    __super::Handle_EffectNotify(pNotify);
}

void CBoss_Raxasia_P1::Update_BlackBoard(_float fTimeDelta)
{
    __super::Update_BlackBoard(fTimeDelta);

    Update_CombatParams();
    Mirror_ControllerParams();

#ifdef _DEBUG
    //Debug_BrainLog(fTimeDelta);
    //Debug_GroggyLog();
#endif

    if (true == m_bDead || true == Is_Dead())
    {
        Set_AnimFloat(L"Speed", 0.f);
        return;
    }

    if (true == m_pBlackBoard->Get<_bool>(L"bFatal")
        || true == m_pBlackBoard->Get<_bool>(L"bGroggy")
        || true == Is_AnimInState(L"FATAL"))
    {
        Set_AnimFloat(L"Speed", 0.f);
        if (nullptr != m_pMovementCom)
            m_pMovementCom->Stop_MoveTo();
        return;
    }

    if (true == m_bShieldBreaking)
    {
        Set_AnimFloat(L"Speed", 0.f);
        if (nullptr != m_pMovementCom)
            m_pMovementCom->Stop_MoveTo();

        m_fShieldBreakGuard -= fTimeDelta;
        if (m_fShieldBreakGuard <= 0.f && false == Is_AnimInState(L"DAMAGED"))
            m_bShieldBreaking = false;

        return;
    }

    if (false == m_bPerceived)
    {
        Set_AnimFloat(L"Speed", 0.f);
        return;
    }

    Drive_Steering(fTimeDelta);
    Update_PatternSelect(fTimeDelta);
    Update_FrontCheck(fTimeDelta);
    Update_StingCheck();
}

void CBoss_Raxasia_P1::Apply_ChargeHit(_float fDamage, _float fGroggy)
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

_bool CBoss_Raxasia_P1::Uses_Full_Hit_Reaction() const
{
    return false;
}

HRESULT CBoss_Raxasia_P1::Ready_Weapon()
{
    const _float4x4* pWeaponSocket = Get_Bone_CombinedMatPtr("BN_Weapon_R");
    if (nullptr == pWeaponSocket)
        return S_OK;

    CWeapon_Raxasia::RAXASIA_WEAPON_DESC WeaponDesc{};
    WeaponDesc.pOwner = this;
    WeaponDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    WeaponDesc.pSocketBoneMatrix = pWeaponSocket;
    WeaponDesc.strModelTag = TEXT("Prototype_Component_Model_Weapon_Raxasia");
    WeaponDesc.iLevelID = ETOUI(LEVEL::GAMEPLAY);
    WeaponDesc.strBladeTopBoneName = "FX_WP_Trail_R_1_1";
    WeaponDesc.strBladeBottomBoneName = "FX_WP_Trail_R_1_3";
    WeaponDesc.fSweepRadius = 0.5f;
    WeaponDesc.fSweepHalfHeight = 0.6f;
    WeaponDesc.fDamage = 1100.f;
    WeaponDesc.bRender = true;

    if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_GameObject_Weapon_Raxasia"), TEXT("Obj_Weapon"), &WeaponDesc)))
        return E_FAIL;

    m_pWeapon = static_cast<CWeapon_Monster*>(Get_PartObject(TEXT("Obj_Weapon")));

    if (nullptr != m_pWeapon)
        static_cast<CWeapon_Raxasia*>(m_pWeapon)->Set_Phase(false);

    return S_OK;
}

HRESULT CBoss_Raxasia_P1::Ready_Shield()
{
    const _float4x4* pShieldSocket = Get_Bone_CombinedMatPtr("BN_Shield");
    if (nullptr == pShieldSocket)
        return S_OK;

    CShield_Raxasia::SHIELD_DESC ShieldDesc{};
    ShieldDesc.pOwner = this;
    ShieldDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    ShieldDesc.pSocketBoneMatrix = pShieldSocket;
    ShieldDesc.strModelTag = TEXT("Prototype_Component_Model_Shield_Raxasia");
    ShieldDesc.iLevelID = ETOUI(LEVEL::GAMEPLAY);
    ShieldDesc.strBashTopBoneName = "FX_WP_Trail_R_1_1";
    ShieldDesc.strBashBottomBoneName = "FX_WP_Trail_R_1_3";
    ShieldDesc.fSweepRadius = 0.6f;
    ShieldDesc.fSweepHalfHeight = 0.45f;
    ShieldDesc.vColliderHalf = _float3{ 0.7f, 0.35f, 0.9f };
    ShieldDesc.fShieldHP = 100.f;
    ShieldDesc.fBlockedRatio = 0.2f;
    ShieldDesc.fDamage = 800.f;
    ShieldDesc.bRender = true;

    if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_GameObject_Shield_Raxasia"), TEXT("Obj_Shield"), &ShieldDesc)))
        return E_FAIL;
    m_pShield = static_cast<CShield_Raxasia*>(Get_PartObject(TEXT("Obj_Shield")));

    return S_OK;
}

HRESULT CBoss_Raxasia_P1::Ready_HitBoxes()
{
    m_HitBoxes.Add_Weapon(L"Weapon", m_pWeapon, 1100.f);
    m_HitBoxes.Add_Weapon(L"Shield", m_pShield, 800.f);
    m_HitBoxes.Add_BoneSphere(L"Kick", "Bip001-R-Foot", 1.f, 600.f);

    if (FAILED(m_HitBoxes.Build(m_pModelCom, m_pDevice, m_pContext)))
        return E_FAIL;

#ifdef _DEBUG
    Set_DebugHitIndex(0);
#endif

    return S_OK;
}

void CBoss_Raxasia_P1::Mirror_ControllerParams()
{
    const _bool bDeadNow = m_bDead || Is_Dead();
    m_pBlackBoard->Set<_bool>(L"bDead", bDeadNow && false == m_pBlackBoard->Get<_bool>(L"bFatal"));

    Set_AnimBool(L"Dead", bDeadNow && false == m_bDead);
    Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
    Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
}

void CBoss_Raxasia_P1::Update_CombatParams()
{
    m_fDistToTarget = 9999.f;

    _vector vTarget;
    if (true == Get_TargetPos(vTarget) && nullptr != m_pTransformCom)
    {
        _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
        _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);
        m_fDistToTarget = XMVectorGetX(XMVector3Length(vTo));
    }

    m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", m_bPerceived);
}

void CBoss_Raxasia_P1::Update_PatternSelect(_float fTimeDelta)
{
    if (false == m_bPerceived)
        return;

    m_pBlackBoard->Set<_bool>(L"bInAttackRange", false);
    m_pBlackBoard->Set<_bool>(L"bAttackReady", false);

    const _bool bIdle = Is_AnimIdle();

    if (BRAIN::INTRO_WALK == m_eBrain)
        return;

    if (BRAIN::INTRO_RUN == m_eBrain)
    {
        if (m_fDistToTarget <= m_fJumpRange)
        {
            m_eBrain = BRAIN::INTRO_JUMP;
            m_bAttackStarted = false;
            m_fCommitTimer = 0.f;
        }
        return;
    }

    if (BRAIN::INTRO_JUMP == m_eBrain)
    {
        if (false == m_bAttackStarted)
        {
            m_fCommitTimer += fTimeDelta;

            m_pBlackBoard->Set<_bool>(L"bInAttackRange", true);
            m_pBlackBoard->Set<_bool>(L"bAttackReady", true);
            Set_AnimInt(L"PatternID", m_iJumpPatternID);
            Set_AnimInt(L"SubPattern", 0);
            Set_AnimTrigger(L"PatternStart");
            Request_AnimAction(L"ATTACK");

            if (true == Was_AnimActionAccepted())
            {
                m_bAttackStarted = true;
                m_fCommitTimer = 0.f;
            }
            else if (m_fCommitTimer > 3.f)
            {
                m_eBrain = BRAIN::COMBAT;
                m_fCommitTimer = 0.f;
                m_iPendingPatternID = -1;
                m_bStrafeNext = true;
                m_iShowcaseIdx = 0;
            }
        }
        else if (true == bIdle)
        {
            m_eBrain = BRAIN::COMBAT;
            m_bAttackStarted = false;
            m_iPendingPatternID = -1;
            m_bStrafeNext = true;
            m_iShowcaseIdx = 0;
        }
        return;
    }

    if (true == m_bAttackCommit)
    {
        if (false == m_bAttackStarted)
        {

            m_bStingHit = false;
            Set_AnimBool(L"Sting", false);
            if (nullptr != m_pWeapon)
                m_pWeapon->Clear_HitSuccess();

            m_pBlackBoard->Set<_bool>(L"bInAttackRange", true);
            m_pBlackBoard->Set<_bool>(L"bAttackReady", true);
            Set_AnimInt(L"PatternID", m_iPendingPatternID);
            Set_AnimInt(L"SubPattern", m_iSubPatternID);
            Set_AnimTrigger(L"PatternStart");
            Request_AnimAction(L"ATTACK");

            if (true == Was_AnimActionAccepted())
                m_bAttackStarted = true;
        }
        else if (true == bIdle)
        {
            m_bAttackCommit = false;
            m_bAttackStarted = false;
            m_iPendingPatternID = -1;
            m_iSubPatternID = 0;
            m_bStrafeNext = true;
        }
        return;
    }

    if (true == m_bStrafing)
    {
        m_fStrafeTimer -= fTimeDelta;
        if (m_fStrafeTimer > 0.f)
            return;
        m_bStrafing = false;
    }

    if (m_iPendingPatternID < 0)
    {
        if (true == m_bStrafeNext)
        {
            m_bStrafeNext = false;
            m_bStrafing = true;
            m_iStrafeDir = (rand() % 2 == 0)
                ? static_cast<_int>(MON_DIRECTION::LEFT)
                : static_cast<_int>(MON_DIRECTION::RIGHT);
            m_fStrafeTimer = m_fStrafeDuration;
            return;
        }

        const _int iP1Count = static_cast<_int>(sizeof(s_P1) / sizeof(RAXA_PAT));
        while (m_iShowcaseIdx < iP1Count && s_P1[m_iShowcaseIdx].iID == m_iJumpPatternID)
            ++m_iShowcaseIdx;

        if (m_iShowcaseIdx < iP1Count)
        {
            m_iPendingPatternID = s_P1[m_iShowcaseIdx].iID;
            m_fPendingNear = s_P1[m_iShowcaseIdx].fNear;
            m_fPendingFar = s_P1[m_iShowcaseIdx].fFar;
            ++m_iShowcaseIdx;
        }
        else
        {
            m_iPendingPatternID = Select_WeightedPattern(m_fDistToTarget);
        }

        m_iSubPatternID = 0;
        m_bApproaching = false;
        m_bRetreating = false;
        m_fRepositionTimer = 0.f;
    }

    m_fRepositionTimer += fTimeDelta;
    const _bool bTimedOut = (m_fRepositionTimer >= m_fRepositionMax);

    if (false == bTimedOut && m_fDistToTarget > m_fPendingFar + m_fAttackRangeTol)
    {
        m_bApproaching = true;
        m_bRetreating = false;
        return;
    }

    if (false == bTimedOut && m_fDistToTarget < m_fPendingNear - m_fAttackRangeTol)
    {
        m_bApproaching = false;
        m_bRetreating = true;
        return;
    }

    m_bApproaching = false;
    m_bRetreating = false;
    m_bAttackCommit = true;
    m_bAttackStarted = false;
}


_vector CBoss_Raxasia_P1::Compute_TrustAnim(_fvector vTargetPos)
{
    if (nullptr == m_pWeapon || nullptr == m_pTransformCom)
        return vTargetPos;

    _vector vTip;
    if (false == static_cast<CWeapon_Raxasia*>(m_pWeapon)->Get_BladeTipWorld(vTip))
        return vTargetPos;

    _vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vFwd = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    _vector vBlade = XMVectorSetY(XMVectorSubtract(vTip, vPos), 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vFwd)) < 1e-6f
        || XMVectorGetX(XMVector3LengthSq(vBlade)) < 1e-6f)
        return vTargetPos;

    vFwd = XMVector3Normalize(vFwd);
    vBlade = XMVector3Normalize(vBlade);

    const _float fDot = XMVectorGetX(XMVector3Dot(vFwd, vBlade));
    const _float fCrossY = XMVectorGetY(XMVector3Cross(vFwd, vBlade));
    const _float fTheta = atan2f(fCrossY, fDot);

    _vector vToTarget = XMVectorSetY(XMVectorSubtract(vTargetPos, vPos), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vToTarget)) < 1e-6f)
        return vTargetPos;

    _vector qComp = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), -fTheta);
    _vector vAimDir = XMVector3Rotate(vToTarget, qComp);

    return XMVectorAdd(vPos, vAimDir);
}

void CBoss_Raxasia_P1::Drive_Steering(_float fTimeDelta)
{
    if (nullptr == m_pMovementCom)
        return;

    _vector vTarget;
    const _bool bHasTarget = Get_TargetPos(vTarget);

    if (true == Is_HitActive())
    {
        Set_AnimFloat(L"Speed", 0.f);
        m_pMovementCom->Stop_MoveTo();

        if (true == bHasTarget && true == Can_AnimRotate())
        {
            _vector vAim = vTarget;
            if (8 == m_iPendingPatternID)
                vAim = Compute_TrustAnim(vTarget);

            m_pMovementCom->TurnTo(vAim, fTimeDelta);
        }
        return;
    }

    if (true == bHasTarget && true == Can_AnimRotate())
        m_pMovementCom->TurnTo(vTarget, fTimeDelta);

    switch (m_eBrain)
    {
    case BRAIN::INTRO_WALK:
        m_fIntroTimer += fTimeDelta;
        if (true == bHasTarget && true == Can_AnimMove())
        {
            Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
            Set_AnimFloat(L"Speed", m_fWalkAnimSpeed);
            m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
            m_pMovementCom->Set_MoveTarget(vTarget);
        }
        if (m_fIntroTimer >= m_fIntroWalkTime)
            m_eBrain = BRAIN::INTRO_RUN;
        break;

    case BRAIN::INTRO_RUN:
        if (true == bHasTarget && true == Can_AnimMove())
        {
            Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
            Set_AnimFloat(L"Speed", m_fRunAnimSpeed);
            m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
            m_pMovementCom->Set_MoveTarget(vTarget);
        }
        break;

    case BRAIN::INTRO_JUMP:
        Set_AnimFloat(L"Speed", 0.f);
        m_pMovementCom->Stop_MoveTo();
        break;

    case BRAIN::COMBAT:
        if (true == m_bStrafing && true == bHasTarget && true == Can_AnimMove())
        {
            Set_AnimInt(L"Direction", m_iStrafeDir);
            Set_AnimFloat(L"Speed", m_fWalkAnimSpeed);
            m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
            m_pMovementCom->Orbit(m_iStrafeDir == static_cast<_int>(MON_DIRECTION::LEFT) ? 0 : 1, vTarget, fTimeDelta);
        }
        else if (true == m_bApproaching && true == bHasTarget && true == Can_AnimMove())
        {
            Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
            Set_AnimFloat(L"Speed", m_fRunAnimSpeed);
            m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
            m_pMovementCom->Set_MoveTarget(vTarget);
        }
        else if (true == m_bRetreating && true == bHasTarget && true == Can_AnimMove())
        {
            _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
            _vector vAway = XMVectorSetY(XMVectorSubtract(vMy, vTarget), 0.f);
            Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::BACK));
            Set_AnimFloat(L"Speed", m_fWalkAnimSpeed);
            m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
            m_pMovementCom->Set_MoveDir(vAway);
        }
        else
        {
            Set_AnimFloat(L"Speed", 0.f);
            m_pMovementCom->Stop_MoveTo();
        }
        break;
    }
}

_bool CBoss_Raxasia_P1::Is_AnimIdle() const
{
    return Is_AnimInState(L"IDLE");
}

_int CBoss_Raxasia_P1::Select_WeightedPattern(_float fDist)
{
    const _int iCount = static_cast<_int>(sizeof(s_P1) / sizeof(RAXA_PAT));

    _float fw[16] = {};
    _float fTotal = 0.f;

    _int   iNearest = 0;
    _float fNearestNear = s_P1[0].fNear;

    for (_int i = 0; i < iCount; ++i)
    {
        if (s_P1[i].fNear < fNearestNear)
        {
            fNearestNear = s_P1[i].fNear;
            iNearest = i;
        }

        if (fDist < s_P1[i].fNear)
        {
            fw[i] = 0.f;
            continue;
        }

        _float fPref = 1.f;
        if (fDist > s_P1[i].fFar)
            fPref = 1.f / (1.f + m_fFarFalloff * (fDist - s_P1[i].fFar));

        _float w = s_P1[i].fWeight * fPref;

        for (_int r = 0; r < m_iRecentCount; ++r)
        {
            if (s_P1[i].iID == m_iRecent[r])
                w *= 0.15f;
        }

        fw[i] = w;
        fTotal += w;
    }

    _int iPick = iNearest;

    if (fTotal > 0.f)
    {
        _float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
        for (_int i = 0; i < iCount; ++i)
        {
            r -= fw[i];
            if (r <= 0.f) { iPick = i; break; }
        }
    }

    m_iLastPatternID = s_P1[iPick].iID;
    m_fPendingNear = s_P1[iPick].fNear;
    m_fPendingFar = s_P1[iPick].fFar;

    for (_int r = RAXA_RECENT - 1; r > 0; --r)
        m_iRecent[r] = m_iRecent[r - 1];
    m_iRecent[0] = s_P1[iPick].iID;
    if (m_iRecentCount < RAXA_RECENT)
        ++m_iRecentCount;

    return s_P1[iPick].iID;
}

void CBoss_Raxasia_P1::On_P1_Fallen()
{
    __super::On_SetDead();

    m_bDead = true;
    m_HitBoxes.On_StateChanged();

    // 실제 P1은 컷신 Actor와 겹치지 않도록 먼저 풀 비활성화 대상으로 넘긴다.
    Request_Despawn();

    // P1 사망은 클리어가 아니라 P2 전환 컷신을 시작하는 지점이다.
    CUTSCENE_REQUEST CutSceneRequest{};
    CutSceneRequest.strAssetName = CUTSCENE_RAXASIA_PHASE2;
    CutSceneRequest.bPlayOnce = true;
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P2, &CutSceneRequest);

    // 에셋 누락 등으로 컷신 재생에 실패해도 전투 진행 자체는 막지 않는다.
    if (FAILED(CutSceneRequest.hResult))
    {
        BossClear_EventDesc StartDesc{};
        StartDesc.eMonsterType = SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2;
        m_pGameInstance->Broadcast(EVENT_TYPE::BOSS_START, &StartDesc);
    }
}

void CBoss_Raxasia_P1::On_GroggyStart()
{
    _vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vOff = XMVector3Rotate(XMLoadFloat3(&m_vFatalOffsetFront), m_pTransformCom->Get_Quat());
    _float3 vWorldPos;
    XMStoreFloat3(&vWorldPos, XMVectorAdd(vPos, vOff));
    vWorldPos.y += 0.1f;
    Start_LoopEffect_World(m_EffectKeys[EFFECT_KEY::SYMBOL], "Effect_Fatal_Symbol", vWorldPos);
    Begin_GroggyWindow(5.f);
    m_bFatalReady = true;
}

void CBoss_Raxasia_P1::Advance_SubPattern()
{
    ++m_iSubPatternID;

    _int iDir = static_cast<_int>(MON_DIRECTION::FRONT);

    _vector vTarget;
    if (true == Get_TargetPos(vTarget) && nullptr != m_pTransformCom)
    {
        _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
        _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vTo)) > 1e-6f)
        {
            vTo = XMVector3Normalize(vTo);
            _vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
            _vector vRight = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::RIGHT), 0.f));
            const _float fFwd = XMVectorGetX(XMVector3Dot(vTo, vLook));
            const _float fRgt = XMVectorGetX(XMVector3Dot(vTo, vRight));

            if (2 == m_iPendingPatternID)
            {
                iDir = Pick_OppositeSide(fRgt);
            }
            else if (8 == m_iPendingPatternID)
            {
                if (m_fDistToTarget < m_fStingBackDist)
                    iDir = static_cast<_int>(MON_DIRECTION::BACK);
                else if (m_fDistToTarget > m_fStingFrontDist)
                    iDir = static_cast<_int>(MON_DIRECTION::FRONT);
                else
                    iDir = Pick_OppositeSide(fRgt);
            }
            else if (fabsf(fFwd) >= fabsf(fRgt))
            {
                iDir = (fFwd >= 0.f)
                    ? static_cast<_int>(MON_DIRECTION::FRONT)
                    : static_cast<_int>(MON_DIRECTION::BACK);
            }
            else
            {
                iDir = (fRgt >= 0.f)
                    ? static_cast<_int>(MON_DIRECTION::RIGHT)
                    : static_cast<_int>(MON_DIRECTION::LEFT);
            }
        }
    }

    Set_AnimInt(L"Direction", iDir);
    Set_AnimInt(L"SubPattern", m_iSubPatternID);
    Set_AnimTrigger(L"PatternStart");
    Request_AnimAction(L"ATTACK");

#ifdef _DEBUG
    Debug_ArmReject(L"Advance_SubPattern", 2, iDir);
#endif
}

_int CBoss_Raxasia_P1::Pick_OppositeSide(_float fRight) const
{
    if (fRight > m_fSideDeadZone)
        return static_cast<_int>(MON_DIRECTION::LEFT);

    return static_cast<_int>(MON_DIRECTION::RIGHT);
}

void CBoss_Raxasia_P1::Fire_BoltRange()
{
    CBoltSector::ARM_DESC Desc{};
    Desc.pOwner = this;
    Desc.pTarget = m_pTarget;
    Desc.fDelay = 0.0f;
    Desc.fDuration = 0.2f;
    Desc.fRadius = 8.f;
    Desc.fHalfAngleCos = -1.1f;
    Desc.fDamage = 1300.f;
    Desc.eHitType = HIT_TYPE::KnockBack;
    Desc.iType = 1;
    CBoltSector* pSector = m_pGameInstance->Get_PoolObject<CBoltSector>(&Desc);
    if (nullptr == pSector)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_BoltSector"), pSector);
}

void CBoss_Raxasia_P1::Spawn_BoltMine()
{
    _vector vPos;
    if (false == Get_BladeGroundPos(vPos))
        return;

    CBoltMine::ARM_DESC Desc{};
    XMStoreFloat3(&Desc.vPosition, vPos);
    Desc.fDelay = 2.f;
    Desc.fLingerDelay = 1.f;
    Desc.vPosition.y += 0.05f;
    Desc.fRadius = 2.f;
    Desc.fDamage = 700.f;
    Desc.eHitType = HIT_TYPE::Heavy;
    Desc.pOwner = this;

    CBoltMine* pMine = m_pGameInstance->Get_PoolObject<CBoltMine>(&Desc);
    if (nullptr == pMine)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_BoltMine"), pMine);
}

void CBoss_Raxasia_P1::Fire_BoltSector()
{
    _vector vOrigin;
    if (false == Get_BladeGroundPos(vOrigin))
        return;

    vOrigin = XMVectorSetY(vOrigin, XMVectorGetY(vOrigin) + EMITTER_GROUND_LIFT);

    _vector vAim = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vAim)) < 1e-6f)
        vAim = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    vAim = XMVector3Normalize(vAim);

    CProjectileEmitter::ARM_DESC Desc{};
    Desc.pOwner = this;
    Desc.pTarget = m_pTarget;
    XMStoreFloat3(&Desc.vOrigin, vOrigin);
    XMStoreFloat3(&Desc.vAimDir, vAim);
    Desc.eMode = CProjectile::MODE::FAN;
    Desc.iCount = 10;
    Desc.fStepDeg = 10.f;
    Desc.bFullCircle = false;
    Desc.fDelay = 2.f;
    Desc.pTelegraphFX = nullptr;
    Desc.fSpeed = 50.f;
    Desc.fAccel = 0.f;
    Desc.fRange = 20.f;
    Desc.fRadius = 0.4f;
    Desc.fDamage = 900.f;
    Desc.eHitType = HIT_TYPE::Heavy;

    CProjectileEmitter* pEmitter = m_pGameInstance->Get_PoolObject<CProjectileEmitter>(&Desc);
    if (nullptr == pEmitter)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Projectile"), pEmitter);
}

void CBoss_Raxasia_P1::Turn_On_Sword()
{
    if (m_pSwordFx != nullptr)
    {
        m_pSwordFx->Stop();
        m_pSwordFx = nullptr;
    }

    m_pSwordFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_Sword_Thunder", m_pWeapon->Get_Model(),
        m_pWeapon->Get_CombinedWorldMatrixPtr());
}

void CBoss_Raxasia_P1::Turn_Off_Sword()
{
    if (nullptr != m_pSwordFx)
    {
        m_pSwordFx->Stop();
        m_pSwordFx = nullptr;
    }
}

void CBoss_Raxasia_P1::Turn_On_Normal_Trail()
{
    static_cast<CWeapon_Raxasia*>(m_pWeapon)->Start_Trail();
}

void CBoss_Raxasia_P1::Turn_On_Elec_Trail()
{
    static_cast<CWeapon_Raxasia*>(m_pWeapon)->Start_Elec_Trail();
}

void CBoss_Raxasia_P1::Turn_Off_Trail()
{
    if (nullptr != m_pWeapon)
        static_cast<CWeapon_Raxasia*>(m_pWeapon)->Stop_Trail();
}

void CBoss_Raxasia_P1::Begin_Grab()
{
    if (nullptr == m_pTarget || nullptr == m_pWeapon)
        return;

    CPlayer* pPlayer = static_cast<CPlayer*>(m_pTarget);
    if (nullptr == pPlayer)
        return;

    pPlayer->Start_Grab(this);
}

//월드 * 소켓 본행렬 반환 
_bool CBoss_Raxasia_P1::Get_GrabSocketWorld(_matrix& OutSocketWorld)
{
    if (nullptr == m_pMatGrabSocket || nullptr == m_pModelCom || nullptr == m_pAnimator)
        return false;

    if (Is_InActive() || Is_Dead())
        return false;

    m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

    //LateUpdate에서 본갱신 한프레임 스킵 플래그. 여기서 했으니까 
    m_bSkipBoneUpdateOnce = true;

    OutSocketWorld = XMLoadFloat4x4(m_pMatGrabSocket) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
    return true;

}

void CBoss_Raxasia_P1::End_Grab()
{
    if (nullptr == m_pTarget || nullptr == m_pWeapon)
        return;

    CPlayer* pPlayer = static_cast<CPlayer*>(m_pTarget);
    if (nullptr == pPlayer)
        return;

    pPlayer->End_Grab();
}

void CBoss_Raxasia_P1::Break_Shield()
{
    if (nullptr != m_pShield && m_pShield->Is_NoHp())
        m_pShield->Set_Broke(true);
}

void CBoss_Raxasia_P1::Begin_StaggerMotion()
{
    m_HitBoxes.On_StateChanged();

    m_bShieldBreaking = true;
    m_fShieldBreakGuard = 0.3f;

    Request_AnimAction(L"DAMAGED");
}

void CBoss_Raxasia_P1::Update_StingCheck()
{
    if (true == m_bStingHit)
        return;

    if (nullptr == m_pAnimator || nullptr == m_pWeapon)
        return;

    if (false == m_pAnimator->Is_InCustomWindow(g_strStingWindow))
        return;

    if (false == m_pWeapon->Is_HitSuccess())
        return;

    m_bStingHit = true;

    Set_AnimBool(L"Sting", true);
    Set_AnimTrigger(L"PatternStart");
    Request_AnimAction(L"ATTACK");
}

void CBoss_Raxasia_P1::Update_FrontCheck(_float fTimeDelta)
{
    if (7 != m_iPendingPatternID)
    {
        m_bFrontPrevValid = false;
        return;
    }

    if (nullptr == m_pAnimator || nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    if (false == m_pAnimator->Is_InCustomWindow(L"CheckFrontNearByTarget"))
    {
        m_bFrontPrevValid = false;
        return;
    }

    _vector vTarget = XMVectorZero();
    const _bool bHasTarget = Get_TargetPos(vTarget);
    if (true == bHasTarget)
        m_pMovementCom->TurnTo(vTarget, fTimeDelta);

    if (0 != m_iSubPatternID)
        return;

    _vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vLook)) > 1e-6f)
        vLook = XMVector3Normalize(vLook);

    _vector vCenter = XMVectorAdd(vPos, XMVectorScale(vLook, m_fFrontCheckForward));
    vCenter = XMVectorSetY(vCenter, XMVectorGetY(vPos) + m_fFrontCheckHeight);

    _vector vPrev = (true == m_bFrontPrevValid) ? XMLoadFloat3(&m_vFrontPrevCenter) : vCenter;
    XMStoreFloat3(&m_vFrontPrevCenter, vCenter);
    m_bFrontPrevValid = true;

    _vector vSeg = XMVectorSubtract(vCenter, vPrev);
    _float fSeg = XMVectorGetX(XMVector3Length(vSeg));
    _vector vDir = (fSeg > 1e-4f) ? XMVectorScale(vSeg, 1.f / fSeg) : vLook;

    CollisionResult tResult{};
    const _bool bHit = m_pGameInstance->Sweep_Sphere(m_fFrontCheckRadius, vPrev, vDir, fSeg, FILTER_GROUP::PLAYER, tResult);

#ifdef _DEBUG
    _float fDist = -1.f;
    _float fDot = 0.f;
    if (true == bHasTarget)
    {
        _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vPos), 0.f);
        fDist = XMVectorGetX(XMVector3Length(vTo));
        if (fDist > 1e-4f)
            fDot = XMVectorGetX(XMVector3Dot(XMVectorScale(vTo, 1.f / fDist), vLook));
    }
    //Debug_FrontLog(fDist, fDot, fSeg, bHit, tResult.pOtherActor);
#endif

    if (false == bHit)
        return;

    m_iSubPatternID = 1;
    Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
    Set_AnimInt(L"SubPattern", 1);
    Set_AnimTrigger(L"PatternStart");
    Request_AnimAction(L"ATTACK");

#ifdef _DEBUG
    //Debug_ArmReject(L"Update_FrontCheck", 1, static_cast<_int>(MON_DIRECTION::FRONT));
#endif
}

_bool CBoss_Raxasia_P1::Get_BladeGroundPos(_vector& OutPos)
{
    if (nullptr == m_pWeapon)
        return false;

    CWeapon_Raxasia* pWeapon = static_cast<CWeapon_Raxasia*>(m_pWeapon);

    _matrix matTop, matBottom;
    if (FAILED(pWeapon->Get_BladeMatrix(matTop, matBottom)))
        return false;

    const _vector vTip = matTop.r[3];
    const _vector vBase = matBottom.r[3];

    const _vector vBladeVec = vTip - vBase;
    const _float  fBladeLength = XMVectorGetX(XMVector3Length(vBladeVec));
    if (fBladeLength <= 1e-4f)
        return false;

    _float3 vRayOrigin, vRayDir;
    XMStoreFloat3(&vRayOrigin, vBase);
    XMStoreFloat3(&vRayDir, XMVector3Normalize(vBladeVec));

    CollisionResult tBlade{};
    if (true == m_pGameInstance->Raycast(vRayOrigin, vRayDir, fBladeLength + pWeapon->Get_GroundRayExtend(),
        FILTER_GROUP::GROUND, tBlade, PxQueryFlag::eSTATIC))
    {
        OutPos = XMLoadFloat3(&tBlade.tRaycastData.vHitPosition);
        return true;
    }

    XMStoreFloat3(&vRayOrigin, XMVectorSetY(vTip, XMVectorGetY(vTip) + BLADE_GROUND_PROBE_UP));
    vRayDir = { 0.f, -1.f, 0.f };

    CollisionResult tDown{};
    if (true == m_pGameInstance->Raycast(vRayOrigin, vRayDir, BLADE_GROUND_PROBE_UP + BLADE_GROUND_PROBE_DOWN,
        FILTER_GROUP::GROUND, tDown, PxQueryFlag::eSTATIC))
    {
        OutPos = XMLoadFloat3(&tDown.tRaycastData.vHitPosition);
        return true;
    }

    return false;
}


CBoss_Raxasia_P1* CBoss_Raxasia_P1::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBoss_Raxasia_P1* pInstance = new CBoss_Raxasia_P1(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CBoss_Raxasia_P1");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CBoss_Raxasia_P1::Clone(void* pArg)
{
    CBoss_Raxasia_P1* pInstance = new CBoss_Raxasia_P1(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBoss_Raxasia_P1");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CBoss_Raxasia_P1::Free()
{
    __super::Free();
    m_HitBoxes.Free();
}

#ifdef _DEBUG
HRESULT CBoss_Raxasia_P1::Set_DebugHitIndex(_int iIndex)
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

HRESULT CBoss_Raxasia_P1::Set_DebugHitRadius(_float fRadius)
{
    m_fDbgHitRadius = fRadius;
    m_HitBoxes.Set_Radius(m_iDbgHitIdx, fRadius);
    return S_OK;
}

HRESULT CBoss_Raxasia_P1::Set_DebugHitHalf(_float fHalf)
{
    m_fDbgHitHalf = fHalf;
    m_HitBoxes.Set_HalfHeight(m_iDbgHitIdx, fHalf);
    return S_OK;
}

void CBoss_Raxasia_P1::Debug_BrainLog(_float fTimeDelta)
{
    m_fDbgLogTimer += fTimeDelta;
    if (m_fDbgLogTimer < 0.5f)
        return;
    m_fDbgLogTimer = 0.f;

    const wchar_t* szBrain =
        (BRAIN::INTRO_WALK == m_eBrain) ? L"IWALK" :
        (BRAIN::INTRO_RUN == m_eBrain) ? L"IRUN" :
        (BRAIN::INTRO_JUMP == m_eBrain) ? L"IJUMP" : L"COMBAT";

    const wchar_t* szState =
        Is_AnimInState(L"IDLE") ? L"IDLE" :
        Is_AnimInState(L"WALK") ? L"WALK" :
        Is_AnimInState(L"WALK_FRONT") ? L"WALK_F" :
        Is_AnimInState(L"RUN") ? L"RUN" :
        Is_AnimInState(L"Run_Front") ? L"Run_F" : L"???";

    wchar_t szLog[256];
    swprintf_s(szLog,
        L"[Brain] %s  state=%s  dist=%.1f  idle=%d detect=%d inRange=%d ready=%d  PID=%d commit=%d started=%d strafe=%d\n",
        szBrain, szState, m_fDistToTarget,
        static_cast<_int>(Is_AnimIdle()),
        static_cast<_int>(m_pBlackBoard->Get<_bool>(L"bPlayerInDetect")),
        static_cast<_int>(m_pBlackBoard->Get<_bool>(L"bInAttackRange")),
        static_cast<_int>(m_pBlackBoard->Get<_bool>(L"bAttackReady")),
        m_iPendingPatternID,
        static_cast<_int>(m_bAttackCommit),
        static_cast<_int>(m_bAttackStarted),
        static_cast<_int>(m_bStrafing));
    OutputDebugStringW(szLog);
}

void CBoss_Raxasia_P1::Debug_GroggyLog()
{
    const _bool bBB = (nullptr != m_pBlackBoard) ? m_pBlackBoard->Get<_bool>(L"bGroggy") : false;
    const _bool bStat = (nullptr != m_pStatCom) ? m_pStatCom->Is_Groggy() : false;
    const _bool bInGroggyState = Is_AnimInState(L"GROGGY");

    if (false == bBB && false == bStat && false == bInGroggyState)
        return;

    const wchar_t* szStep = L"-";
    if (nullptr != m_pAnimator)
    {
        const Engine::ANIM_SEQUENCE_STEP* pStep = m_pAnimator->Get_CurSequenceStep();
        if (nullptr != pStep)
        {
            switch (pStep->eStepType)
            {
            case Engine::ANIM_SEQUENCE_STEP_TYPE::START:  szStep = L"START";  break;
            case Engine::ANIM_SEQUENCE_STEP_TYPE::ACTIVE: szStep = L"ACTIVE"; break;
            case Engine::ANIM_SEQUENCE_STEP_TYPE::LOOP:   szStep = L"LOOP";   break;
            case Engine::ANIM_SEQUENCE_STEP_TYPE::END:    szStep = L"END";    break;
            default:                                      szStep = L"?";      break;
            }
        }
    }

    const wchar_t* szState =
        Is_AnimInState(L"GROGGY") ? L"GROGGY" :
        Is_AnimInState(L"IDLE") ? L"IDLE" :
        Is_AnimInState(L"FATAL") ? L"FATAL" : L"other";

    const _float fGauge = (nullptr != m_pStatCom) ? m_pStatCom->Get_CurrentGroggy() : 0.f;
    const _bool  bStag = (nullptr != m_pStatCom) ? m_pStatCom->Is_Staggerable() : false;
    const _bool  bCool = (nullptr != m_pStatCom) ? m_pStatCom->Is_OnGroggyCooldown() : false;
    const _bool  bEnded = (nullptr != m_pAnimator) ? m_pAnimator->Is_Ended() : false;
    const _uint  iFrame = (nullptr != m_pAnimator) ? m_pAnimator->Get_LocalFrame() : 0;

    wchar_t szLog[256];
    swprintf_s(szLog,
        L"[Groggy] BB=%d Stat=%d | state=%s step=%s frame=%u ended=%d | gauge=%.0f stag=%d cool=%d\n",
        static_cast<_int>(bBB),
        static_cast<_int>(bStat),
        szState, szStep, iFrame,
        static_cast<_int>(bEnded),
        fGauge,
        static_cast<_int>(bStag),
        static_cast<_int>(bCool));
    OutputDebugStringW(szLog);
}

void CBoss_Raxasia_P1::Debug_FrontLog(_float fDist, _float fDot, _float fSeg, _bool bHit, CGameObject* pOther)
{
    const _float fReach = m_fFrontCheckForward + m_fFrontCheckRadius;

    wchar_t szLog[256];
    swprintf_s(szLog, L"[FrontCheck] frame=%u  dist=%.2f  dot=%.2f  fSeg=%.3f  reach=%.2f  hit=%d  other=%s\n",
        m_pAnimator->Get_LocalFrame(), fDist, fDot, fSeg, fReach,
        bHit ? 1 : 0,
        (nullptr != pOther) ? L"actor" : L"null");

    OutputDebugStringW(szLog);
}

void CBoss_Raxasia_P1::Debug_ArmReject(const _tchar* pWhere, _int iDelayFrames, _int iDir)
{
    m_pDbgReqWhere = pWhere;
    m_iDbgReqTick = iDelayFrames;
    m_iDbgReqPattern = m_iPendingPatternID;
    m_iDbgReqSub = m_iSubPatternID;
    m_iDbgReqDir = iDir;
    m_fDbgReqDist = m_fDistToTarget;
}

void CBoss_Raxasia_P1::Debug_RejectLog()
{
    if (m_iDbgReqTick <= 0)
        return;

    --m_iDbgReqTick;
    if (m_iDbgReqTick > 0)
        return;

    static const _tchar* szDir[5] = { L"NONE", L"BACK", L"LEFT", L"RIGHT", L"FRONT" };
    const _tchar* pDirName = (m_iDbgReqDir >= 0 && m_iDbgReqDir <= 4) ? szDir[m_iDbgReqDir] : L"?";

    const _bool bAccepted = Was_AnimActionAccepted();

    _tchar szLog[256];
    swprintf_s(szLog, L"[LINK] %s  PID=%d  Sub=%d  Dir=%d(%s)  dist=%.2f  band=[%.1f~%.1f]  link=[%.1f~%.1f]  -> %s\n",
        m_pDbgReqWhere, m_iDbgReqPattern, m_iDbgReqSub, m_iDbgReqDir, pDirName,
        m_fDbgReqDist, m_fPendingNear, m_fPendingFar,
        m_fStingBackDist, m_fStingFrontDist,
        bAccepted ? L"OK" : L"REJECT (no state)");

    OutputDebugStringW(szLog);
}

void CBoss_Raxasia_P1::Debug_ResetToOrigin()
{
    const _vector vOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    if (nullptr != m_pCCT)
        m_pCCT->Set_FootPosition(vOrigin);

    if (nullptr != m_pTransformCom)
        m_pTransformCom->Set_State(STATE::POSITION, vOrigin);

    Cancel_All_Attacks();

    m_bDbgMeasuring = false;
    m_iDbgReqTick = 0;
    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iPendingPatternID = -1;
    m_iSubPatternID = 0;
    m_bFrontPrevValid = false;
    m_bApproaching = false;
    m_bRetreating = false;
    m_bStrafing = false;

    OutputDebugStringW(L"[Ruler] reset to origin\n");
}

void CBoss_Raxasia_P1::Debug_MeasureAttack(_float fTimeDelta)
{
    if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
        return;

    if (false == m_bDbgBonesReady)
    {
        m_pDbgFootL = Get_Bone_CombinedMatPtr("Bip001-L-Foot");
        m_pDbgFootR = Get_Bone_CombinedMatPtr("Bip001-R-Foot");
        m_bDbgBonesReady = true;
    }

    if (true == m_pGameInstance->Key_Down(DIK_F1))
    {
        Debug_ResetToOrigin();
        return;
    }

    if (false == m_bDbgMeasuring)
    {
        static const _ubyte byFireKeys[DBG_PATTERN_COUNT] =
        {
            DIK_NUMPAD1, DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPAD4, DIK_NUMPAD5,
            DIK_NUMPAD6, DIK_NUMPAD7, DIK_NUMPAD8, DIK_NUMPAD9, DIK_NUMPAD0
        };

        for (_int i = 0; i < DBG_PATTERN_COUNT; ++i)
        {
            if (false == m_pGameInstance->Key_Down(byFireKeys[i]))
                continue;

            Cancel_All_Attacks();
            Begin_Battle();

            const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
            XMStoreFloat3(&m_vDbgStartPos, vMyPos);

            m_iDbgAttackID = i + 1;
            m_fDbgLunge = 0.f;
            m_fDbgBladeNear = -1.f;
            m_fDbgBladeFar = 0.f;
            m_fDbgFootNear = -1.f;
            m_fDbgFootFar = 0.f;
            m_fDbgBladeAngle = 999.f;
            m_fDbgAimErr = 999.f;
            m_fDbgScaleMax = 1.f;
            m_fDbgStartDist = -1.f;
            m_fDbgLungeAtHit = -1.f;
            m_fDbgTimer = 0.f;
            m_iDbgReqTick = 0;
            m_bDbgStarted = false;
            m_bDbgFirstHitDone = false;
            m_bDbgMeasuring = true;

            _vector vTargetPos;
            if (true == Get_TargetPos(vTargetPos))
                m_fDbgStartDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f)));

            m_eBrain = BRAIN::COMBAT;
            m_bApproaching = false;
            m_bRetreating = false;
            m_fRepositionTimer = 0.f;
            m_iPendingPatternID = i + 1;
            m_iSubPatternID = 0;
            m_bAttackCommit = true;
            m_bAttackStarted = false;
            break;
        }
        return;
    }

    m_fDbgTimer += fTimeDelta;

    const _float fCurScale = Calc_RootMotionScale();
    if (fCurScale > m_fDbgScaleMax)
        m_fDbgScaleMax = fCurScale;

    const _vector vStart = XMLoadFloat3(&m_vDbgStartPos);

    const _vector vBody = m_pTransformCom->Get_State(STATE::POSITION);
    const _float  fLunge = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vBody, vStart), 0.f)));
    if (fLunge > m_fDbgLunge)
        m_fDbgLunge = fLunge;

    if (false == Is_HitActive())
    {
        if (m_fDbgBladeNear >= 0.f || m_fDbgFootNear >= 0.f)
            m_bDbgFirstHitDone = true;
    }
    else if (false == m_bDbgFirstHitDone)
    {
        if (m_fDbgLungeAtHit < 0.f)
            m_fDbgLungeAtHit = fLunge;

        const _matrix matWorld = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());

        _float fFoot = -1.f;
        if (nullptr != m_pDbgFootL)
        {
            const _matrix matBone = XMLoadFloat4x4(m_pDbgFootL) * matWorld;
            const _float  fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(matBone.r[3], vStart), 0.f)));
            if (fDist > fFoot) fFoot = fDist;
        }
        if (nullptr != m_pDbgFootR)
        {
            const _matrix matBone = XMLoadFloat4x4(m_pDbgFootR) * matWorld;
            const _float  fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(matBone.r[3], vStart), 0.f)));
            if (fDist > fFoot) fFoot = fDist;
        }
        if (fFoot >= 0.f)
        {
            if (m_fDbgFootNear < 0.f || fFoot < m_fDbgFootNear) m_fDbgFootNear = fFoot;
            if (fFoot > m_fDbgFootFar) m_fDbgFootFar = fFoot;
        }

        if (nullptr != m_pWeapon)
        {
            _vector vTip;
            if (true == static_cast<CWeapon_Raxasia*>(m_pWeapon)->Get_BladeTipWorld(vTip))
            {
                const _float fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vTip, vStart), 0.f)));
                if (m_fDbgBladeNear < 0.f || fDist < m_fDbgBladeNear) m_fDbgBladeNear = fDist;
                if (fDist > m_fDbgBladeFar) m_fDbgBladeFar = fDist;

                _vector vBossPos = matWorld.r[3];
                _vector vFwd = XMVector3Normalize(XMVectorSetY(matWorld.r[2], 0.f));
                _vector vBladeDir = XMVector3Normalize(XMVectorSetY(XMVectorSubtract(vTip, vBossPos), 0.f));
                m_fDbgBladeAngle = XMConvertToDegrees(atan2f(
                    XMVectorGetY(XMVector3Cross(vFwd, vBladeDir)),
                    XMVectorGetX(XMVector3Dot(vFwd, vBladeDir))));

                _vector vTgt;
                if (true == Get_TargetPos(vTgt))
                {
                    _vector vToTgt = XMVector3Normalize(XMVectorSetY(XMVectorSubtract(vTgt, vBossPos), 0.f));
                    m_fDbgAimErr = XMConvertToDegrees(atan2f(
                        XMVectorGetY(XMVector3Cross(vToTgt, vBladeDir)),
                        XMVectorGetX(XMVector3Dot(vToTgt, vBladeDir))));
                }
            }
        }
    }

    if (true == m_bAttackStarted)
        m_bDbgStarted = true;

    const _bool bDone = (true == m_bDbgStarted
        && (true == m_bDbgFirstHitDone || false == m_bAttackCommit));
    const _bool bTimeout = (m_fDbgTimer > 15.f);

    if (true == bDone || true == bTimeout)
    {
        const _bool bHitActive = (m_fDbgBladeNear >= 0.f || m_fDbgFootNear >= 0.f);
        _tchar szLog[384];

        if (true == bHitActive)
            swprintf_s(szLog, L"[PatternRuler] P%d  authored=%.2f  hit1_blade=[%.2f~%.2f]  hit1_foot=[%.2f~%.2f]  lungeAtHit=%.2f  bladeAng=%.1f  aimErr=%.1f%s\n",
                m_iDbgAttackID, m_fDbgScaleMax,
                (m_fDbgBladeNear < 0.f ? 0.f : m_fDbgBladeNear), m_fDbgBladeFar,
                (m_fDbgFootNear < 0.f ? 0.f : m_fDbgFootNear), m_fDbgFootFar,
                m_fDbgLungeAtHit,
                m_fDbgBladeAngle, m_fDbgAimErr,
                bTimeout ? L"  (TIMEOUT)" : L"");
        else
            swprintf_s(szLog, L"[PatternRuler] P%d  authored=%.2f  NO_HITACTIVE  lunge=%.2f%s\n",
                m_iDbgAttackID, m_fDbgScaleMax, m_fDbgLunge,
                bTimeout ? L"  (TIMEOUT)" : L"");

        OutputDebugStringW(szLog);

        m_bDbgMeasuring = false;
    }
}

#endif // _DEBUG
