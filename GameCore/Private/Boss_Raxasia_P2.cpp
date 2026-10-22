#include "Boss_Raxasia_P2.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "CCT.h"
#include "Model.h"
#include "Mesh.h"
#include "Animator.h"
#include "Effect.h"
#include "Monster_Defines.h"
#include "Monster_Include.h"
#include "Player.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CBoss_Raxasia_P2, CAIObject)
END_REFLECTION(CBoss_Raxasia_P2)
#endif

namespace
{
    const _int P2_ENTRY_PATTERN = 67;
    const _int P2_INTRO_PATTERN = 6;

    static _bool In_AngleRange(_float fAngle, _float fStart, _float fEnd)
    {
        if (fStart <= fEnd)
            return (fAngle >= fStart && fAngle <= fEnd);

        return (fAngle >= fStart || fAngle <= fEnd);
    }

    const string BGM_BATTLE_INTRO_P2 = "MU_MS_Raxasia_PH2_Ver2_Intro";
    const string BGM_BATTLE_KEY_P2 = "MU_MS_Raxasia_PH2_Ver2";
    const string CUTSCENE_RAXASIA_DEAD = "Raxasia_Dead";

    // 원본이 RMS -14dBFS 로 다른 전투곡보다 크므로 낮춰서 맞춘다.
    const _float BGM_BATTLE_VOLUME_P2 = 0.22f;
    const _float BGM_FADE_SEC = 1.5f;
}

CBoss_Raxasia_P2::CBoss_Raxasia_P2(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CAIObject{ pDevice, pContext }
{
    m_eActorType = ACTOR_TYPE::BOSS;
    m_strMonsterName = L"완성된 자 락사시아";
}

CBoss_Raxasia_P2::CBoss_Raxasia_P2(const CBoss_Raxasia_P2& Prototype)
    : CAIObject{ Prototype }
    , m_EffectKeys{ Prototype.m_EffectKeys }
{
    m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBoss_Raxasia_P2::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Raxasia_P2.json";

    m_NotifyKeys[Hash_String(L"SET_DEAD")] = NOTIFY_KEY::SET_DEAD;
    m_NotifyKeys[Hash_String(L"Symbol_Start")] = NOTIFY_KEY::FATAL_READY;
    m_NotifyKeys[Hash_String(L"ShieldBreak")] = NOTIFY_KEY::SHEILD_BREAK;
    m_NotifyKeys[Hash_String(L"HitOn_BoltRange")] = NOTIFY_KEY::BOLT_RANGE;
    m_NotifyKeys[Hash_String(L"Fire_BoltSector")] = NOTIFY_KEY::FIRE_SECTOR;
    m_NotifyKeys[Hash_String(L"Land_BoltSector")] = NOTIFY_KEY::LAND_SECTOR;
    m_NotifyKeys[Hash_String(L"FIre_GroundWave")] = NOTIFY_KEY::FIRE_GROUNDWAVE;
    m_NotifyKeys[Hash_String(L"Fire_Thunderstroke")] = NOTIFY_KEY::FIRE_THUNDERSTROKE;
    m_NotifyKeys[Hash_String(L"Fire_ElectricBall")] = NOTIFY_KEY::FIRE_ELECTRICBALL;

    m_NotifyKeys[Hash_String(L"Sword_Thunder_On")] = NOTIFY_KEY::SWORD_THUNDER_ON;
    m_NotifyKeys[Hash_String(L"Sword_Thunder_Off")] = NOTIFY_KEY::SWORD_THUNDER_OFF;
    m_NotifyKeys[Hash_String(L"Trail_On")] = NOTIFY_KEY::TRAIL_ON;
    m_NotifyKeys[Hash_String(L"Trail_Off")] = NOTIFY_KEY::TRAIL_OFF;
    m_NotifyKeys[Hash_String(L"Elec_Trail_On")] = NOTIFY_KEY::ELEC_TRAIL_ON;
    m_NotifyKeys[Hash_String(L"Body_Thunder_On")] = NOTIFY_KEY::BODY_THUNDER_ON;
    m_NotifyKeys[Hash_String(L"Body_Thunder_Off")] = NOTIFY_KEY::BODY_THUNDER_OFF;
    m_NotifyKeys[Hash_String(L"Run_Trail_On")] = NOTIFY_KEY::RUN_TRAIL_ON;
    m_NotifyKeys[Hash_String(L"Run_Trail_Off")] = NOTIFY_KEY::RUN_TRAIL_OFF;

    m_EffectKeys[EFFECT_KEY::FURY] = Hash_String(L"LOOP_FURY");
    m_EffectKeys[EFFECT_KEY::SYMBOL] = Hash_String(L"LOOP_SYMBOL");

    return S_OK;
}

HRESULT CBoss_Raxasia_P2::Initialize(void* pArg)
{
    if (nullptr != pArg)
    {
        auto pDesc = static_cast<MONSTER_DESC*>(pArg);
        m_fAttackGap = pDesc->fAttackGap;
        Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });
    }

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    CAIObject::CHARACTER_SETUP_DESC SetupDesc{};
    SetupDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Raxasia_P2");
    SetupDesc.strAnimFolderTag = TEXT("Raxasia_P2");
    SetupDesc.strAnimTag = TEXT("Raxasia_P2");
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

    Collect_AuthoredStates();

    if (FAILED(Ready_LinkTable()))
        return E_FAIL;

    if (FAILED(Ready_Weapon()))
        return E_FAIL;

    if (FAILED(Ready_Shield()))
        return E_FAIL;

    if (FAILED(Ready_HitBoxes()))
        return E_FAIL;

    m_fGroggyWindowFallback = 30.f;

    Capture_SpawnPos();
    m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::END_CUTSCENE_RAXASIA_DEAD, this);


    m_ShowcaseSeq = { 6,   61,   32,  36,  106,  42,  44,  99,  49,   52,  80 };
    m_ShowcaseDist = { 9999.f, 40.f, 10.f, 8.f, 30.f, 8.f, 8.f, 5.f, 22.f, 22.f, 15.f };

    m_pSwordFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_Sword_Thunder", m_pWeapon->Get_Model(),
        m_pWeapon->Get_CombinedWorldMatrixPtr());

    Spawn();

    return S_OK;
}

void CBoss_Raxasia_P2::Update(_float fTimeDelta)
{
    // 전환 컷신이 끝나기 전에는 실제 P2의 애니메이션과 AI를 갱신하지 않는다.
    if (false == m_bBattleStarted || Is_AISleep())
        return;

    __super::Update(fTimeDelta);

    Update_Thunderstroke(fTimeDelta);

    if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
        Apply_Movment(fTimeDelta);
}

void CBoss_Raxasia_P2::Physical_Update(_float fFixedTime)
{
    // 컷신 Actor와 겹쳐 있는 동안 실제 P2의 물리 처리를 막는다.
    if (false == m_bBattleStarted)
        return;

    __super::Physical_Update(fFixedTime);
}

void CBoss_Raxasia_P2::Late_Update(_float fTimeDelta)
{
    // BOSS_START 전에는 실제 P2를 렌더 그룹에 등록하지 않는다.
    if (false == m_bBattleStarted || Is_AISleep())
        return;

    __super::Late_Update(fTimeDelta);

    if (Was_AnimStateChanged())
    {
        m_HitBoxes.On_StateChanged();
        m_HitBoxes.Set_HitType(HIT_TYPE::Normal);

        if (nullptr != m_pWeapon)
            m_pWeapon->Clear_HitSuccess();
        if (nullptr != m_pShield)
            m_pShield->Clear_HitSuccess();
    }
    m_HitBoxes.Update(m_pGameInstance, this, fTimeDelta);

    if (m_bWeaponLive && nullptr != m_pWeapon)
        m_pWeapon->Late_Update(fTimeDelta);

    if (nullptr != m_pShield)
        m_pShield->Late_Update(fTimeDelta);

#ifdef _DEBUG
    Debug_MoveLog(fTimeDelta);
#endif
}

HRESULT CBoss_Raxasia_P2::Render()
{
    return __super::Render();
}

void CBoss_Raxasia_P2::Action_Notify(_uint64 iHashKey)
{
    if (iHashKey == Hash_String(L"ChargeStart"))
    {
        Begin_Charge();
        return;
    }

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
        case NOTIFY_KEY::SET_DEAD:		        On_Fallen();		        break;
        case NOTIFY_KEY::FATAL_READY:	        On_GroggyStart();	        break;
        case NOTIFY_KEY::SHEILD_BREAK:	        Break_Shield();		        break;
        case NOTIFY_KEY::BOLT_RANGE:	        Fire_BoltRange();	        break;
        case NOTIFY_KEY::FIRE_SECTOR:	        Fire_BoltSector(true);	    break;
        case NOTIFY_KEY::LAND_SECTOR:	        Fire_BoltSector(false);	    break;
        case NOTIFY_KEY::FIRE_GROUNDWAVE:       Fire_GroundWave();          break;
        case NOTIFY_KEY::FIRE_THUNDERSTROKE:    Reserve_Thunderstroke();    break;
        case NOTIFY_KEY::FIRE_ELECTRICBALL:     Fire_ElectricBall();        break;
        case NOTIFY_KEY::SWORD_THUNDER_ON:      Turn_On_Sword();            break;
        case NOTIFY_KEY::SWORD_THUNDER_OFF:     Turn_Off_Sword();           break;
        case NOTIFY_KEY::TRAIL_ON:              Turn_On_Normal_Trail();     break;
        case NOTIFY_KEY::TRAIL_OFF:             Turn_Off_Trail();           break;
        case NOTIFY_KEY::ELEC_TRAIL_ON:         Turn_On_Elec_Trail();       break;
        case NOTIFY_KEY::BODY_THUNDER_ON:       Turn_On_Body_Thunder();     break;
        case NOTIFY_KEY::BODY_THUNDER_OFF:      Turn_Off_Body_Thunder();    break;
        case NOTIFY_KEY::RUN_TRAIL_ON:          Turn_On_Run_Trail();        break;
        case NOTIFY_KEY::RUN_TRAIL_OFF:         Turn_Off_Run_Trail();       break;
        default: break;
        }
        return;
    }

    m_HitBoxes.Handle_Notify(iHashKey);
}

void CBoss_Raxasia_P2::On_Groggy(_bool bGroggy)
{
    if (true == bGroggy)
    {
        m_HitBoxes.On_StateChanged();

        if (nullptr != m_pShield)
            m_pShield->Set_Physics_InScene(false);

        m_bAttackCommit = false;
        m_bAttackStarted = false;
        m_iPendingPatternID = -1;
        m_iCurPatternID = -1;
        m_fCommitTimer = 0.f;

        m_ThunderReserves.clear();

        Set_AnimInt(L"PatternID", -1);
    }
    else
    {
        Stop_LoopImmediate(m_EffectKeys[EFFECT_KEY::SYMBOL]);
        m_bFatalReady = false;

        if (nullptr != m_pShield && false == m_pShield->Is_NoHp())
            m_pShield->Set_Physics_InScene(true);
    }
}

void CBoss_Raxasia_P2::On_SuperArmor(_bool bOn)
{
    if (true == bOn)
    {
        Set_RimAmount(0.5f);
        Set_RimProfile(1);

        if (nullptr != m_pWeapon)
        {
            m_pWeapon->Set_RimAmount(0.5f);
            m_pWeapon->Set_RimProfile(1);
        }

        if (nullptr != m_pShield)
        {
            m_pShield->Set_RimAmount(0.5f);
            m_pShield->Set_RimProfile(1);
        }
    }
    else
    {
        Set_RimAmount(0.f);

        if (nullptr != m_pWeapon)
            m_pWeapon->Set_RimAmount(0.f);

        if (nullptr != m_pShield)
            m_pShield->Set_RimAmount(0.f);
    }
}

void CBoss_Raxasia_P2::Begin_Battle()
{
    // Phase2 컷신 종료 프레임부터 실제 P2의 물리, 갱신, 렌더링을 함께 활성화한다.
    if (false == m_bBattleStarted)
    {
        Turn_On_AI();

        if (nullptr != m_pShield)
            m_pShield->Set_Physics_InScene(true);
    }

    m_bBattleStarted = true;
    m_bPerceived = true;
    m_bDetected = true;

    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iPendingPatternID = -1;
    m_iCurPatternID = -1;

    m_pGameInstance->Play_BGM_Sequence(BGM_BATTLE_INTRO_P2, BGM_BATTLE_KEY_P2, BGM_BATTLE_VOLUME_P2);
}

void CBoss_Raxasia_P2::OnEvent(EVENT_TYPE eType, Event_Desc* pAdata)
{
    switch (eType)
    {
    case Engine::EVENT_TYPE::BOSS_START:
        if (true == Is_MyBossStart(pAdata))
            Begin_Battle();
        break;
    default:
        break;
    }
}

void CBoss_Raxasia_P2::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    UNREFERENCED_PARAMETER(pData);

    if (EVENT_TYPE_MAP_OBJECT::END_CUTSCENE_RAXASIA_DEAD != eType || false == m_bDead)
        return;

    // Dead 컷신 정상 종료 후 P2를 먼저 풀로 반환한 다음 최종 클리어를 알린다.
    Request_Despawn();

    BossClear_EventDesc ClearDesc{};
    ClearDesc.eMonsterType = SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2;
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, &ClearDesc);
}

void CBoss_Raxasia_P2::Spawn()
{
    __super::Spawn();

    // 맵에는 미리 배치하되 Phase2 컷신 종료 전까지 컷신 모델만 보이도록 대기한다.
    m_bBattleStarted = false;

    if (nullptr != m_pShield)
        m_pShield->Reset_Shield();

    m_bShieldBreaking = false;
    m_fShieldBreakGuard = 0.f;

    m_bDead = false;
    m_bPerceived = false;
    m_bWeaponLive = true;

    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iCurPatternID = -1;
    m_iShowcaseIdx = 0;

    m_fCommitTimer = 0.f;
    m_fIdleTimer = 0.f;

    m_fRunnerTime = 0.f;
    m_PatternLastUsed.clear();

    m_bCharging = false;
    m_fChargeTimer = 0.f;
    m_fAngleToTarget = 0.f;
    m_fDistToTarget = 9999.f;
    m_fStepTimer = 0.f;
    m_fStuckTimer = 0.f;
    m_iActiveFWarp = -1;
    m_iComboEntry = -1;
    m_iStepPattern = -1;

    // 대기 중에는 CCT, Animator, 방패 물리를 Scene에서 제외한다.
    Turn_Off_AI();

    if (nullptr != m_pShield)
        m_pShield->Set_Physics_InScene(false);
}

void CBoss_Raxasia_P2::Despawn()
{
    // 사망 컷신이 실제 P2 대신 렌더되도록 즉시 갱신과 렌더 등록을 차단한다.
    m_bBattleStarted = false;
    m_ThunderReserves.clear();

    Turn_Off_Trail();
    Turn_Off_Sword();
    Turn_Off_Body_Thunder();
    Turn_Off_Run_Trail();

    if (nullptr != m_pShield)
        m_pShield->Set_Physics_InScene(false);

    __super::Despawn();
}


_bool CBoss_Raxasia_P2::Can_FatalBack() const
{
    return false;
}

void CBoss_Raxasia_P2::Apply_ShieldBlock(_float fDamage)
{
    if (nullptr == m_pStatCom || true == m_bDead)
        return;

    m_pStatCom->Apply_Damage(fDamage);
    m_pStatCom->Set_SinceHit(0.f);
}

void CBoss_Raxasia_P2::On_ShieldBroken()
{
    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iPendingPatternID = -1;
    m_iCurPatternID = -1;

    Set_AnimInt(L"PatternID", -1);
    Begin_StaggerMotion();
}

_bool CBoss_Raxasia_P2::Get_BladeGroundPos(_vector& OutPos)
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

void CBoss_Raxasia_P2::Break_Shield()
{
    if (nullptr != m_pShield && m_pShield->Is_NoHp())
        m_pShield->Set_Broke(true);
}

void CBoss_Raxasia_P2::Begin_StaggerMotion()
{
    m_HitBoxes.On_StateChanged();

    m_bShieldBreaking = true;
    m_fShieldBreakGuard = 0.3f;

    Request_AnimAction(L"DAMAGED");
}

void CBoss_Raxasia_P2::Turn_On_Sword()
{
    if (m_pSwordFx != nullptr)
    {
        m_pSwordFx->Stop();
        m_pSwordFx = nullptr;
    }

    m_pSwordFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_Sword_Thunder", m_pWeapon->Get_Model(),
        m_pWeapon->Get_CombinedWorldMatrixPtr());
}

void CBoss_Raxasia_P2::Turn_Off_Sword()
{
    if (nullptr != m_pSwordFx)
    {
        m_pSwordFx->Stop();
        m_pSwordFx = nullptr;
    }
}

void CBoss_Raxasia_P2::Turn_On_Normal_Trail()
{
    static_cast<CWeapon_Raxasia*>(m_pWeapon)->Start_Trail();
}

void CBoss_Raxasia_P2::Turn_On_Elec_Trail()
{
    static_cast<CWeapon_Raxasia*>(m_pWeapon)->Start_Elec_Trail();
}

void CBoss_Raxasia_P2::Turn_Off_Trail()
{
    if (nullptr != m_pWeapon)
        static_cast<CWeapon_Raxasia*>(m_pWeapon)->Stop_Trail();
}

void CBoss_Raxasia_P2::Turn_On_Body_Thunder()
{
    if (m_pBodyFx != nullptr)
    {
        m_pBodyFx->Stop();
        m_pBodyFx = nullptr;
    }

    m_pBodyFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_Body_Thunder",
        m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
}

void CBoss_Raxasia_P2::Turn_Off_Body_Thunder()
{
    if (nullptr != m_pBodyFx)
    {
        m_pBodyFx->Stop();
        m_pBodyFx = nullptr;
    }
}

void CBoss_Raxasia_P2::Turn_On_Run_Trail()
{
    if (m_pRightFootFx != nullptr)
    {
        m_pRightFootFx->Stop();
        m_pRightFootFx = nullptr;
    }

    if (m_pLeftFootFx != nullptr)
    {
        m_pLeftFootFx->Stop();
        m_pLeftFootFx = nullptr;
    }

    const _float4x4* pRight = m_pModelCom->Get_Bone_CombinedMatPtr("Root");
    const _float4x4* pLeft = m_pModelCom->Get_Bone_CombinedMatPtr("Bip001-Pelvis");

    m_pRightFootFx = m_pGameInstance->Play_Effect("Effect_Raxasia_Run_Trail", pRight, m_pTransformCom->Get_WorldMatrixPtr());
    m_pLeftFootFx = m_pGameInstance->Play_Effect("Effect_Raxasia_Run_Trail", pLeft, m_pTransformCom->Get_WorldMatrixPtr());
}

void CBoss_Raxasia_P2::Turn_Off_Run_Trail()
{
    if (nullptr != m_pRightFootFx)
    {
        m_pRightFootFx->Stop();
        m_pRightFootFx = nullptr;
    }

    if (nullptr != m_pLeftFootFx)
    {
        m_pLeftFootFx->Stop();
        m_pLeftFootFx = nullptr;
    }
}

void CBoss_Raxasia_P2::On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash)
{
    __super::On_AnimStateChanged(iPrevStateHash, iCurStateHash);

    Turn_Off_Trail();
    Turn_Off_Body_Thunder();
    Turn_Off_Run_Trail();
}

void CBoss_Raxasia_P2::Update_BlackBoard(_float fTimeDelta)
{
    __super::Update_BlackBoard(fTimeDelta);

    Update_CombatParams();
    Mirror_ControllerParams();

#ifdef _DEBUG
    Debug_ForceInput();
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

        if (true == m_pBlackBoard->Get<_bool>(L"bGroggy")
            && false == m_pBlackBoard->Get<_bool>(L"bFatal")
            && false == m_bFatalReady
            && nullptr != m_pMovementCom && nullptr != m_pTransformCom)
        {
            _vector vTarget;
            if (true == Get_TargetPos(vTarget))
            {
                m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fGroggyTrackSpeed));
                m_pMovementCom->TurnTo(vTarget, fTimeDelta);
            }
        }

        return;
    }

    if (true == m_bShieldBreaking)
    {
        Set_AnimFloat(L"Speed", 0.f);

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
    Drive_FrameWarp(fTimeDelta);
    Drive_FrameGlide(fTimeDelta);
    Drive_OrbitGlide(fTimeDelta);
    Update_Charge(fTimeDelta);
    Update_LinkRunner(fTimeDelta);
}

void CBoss_Raxasia_P2::Fire_BoltSector(_bool bFull)
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
    Desc.iCount = bFull ? 19 : 10;
    Desc.fStepDeg = bFull ? 20.f : 10.f;
    Desc.bFullCircle = bFull;
    Desc.fDelay = 2.f;
    Desc.pTelegraphFX = nullptr;
    Desc.fSpeed = 40.f;
    Desc.fAccel = 0.f;
    Desc.fRange = 10.f;
    Desc.fRadius = 0.4f;
    Desc.fDamage = 1300.f;
    Desc.eHitType = HIT_TYPE::Heavy;

    CProjectileEmitter* pEmitter = m_pGameInstance->Get_PoolObject<CProjectileEmitter>(&Desc);
    if (nullptr == pEmitter)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Projectile"), pEmitter);
}

void CBoss_Raxasia_P2::Fire_BoltRange()
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

void CBoss_Raxasia_P2::Fire_GroundWave()
{
    _vector vBoss = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vOrigin = vBoss;

    const _int iBone = (nullptr != m_pModelCom) ? m_pModelCom->Get_BoneIndex("BN_Weapon_R") : -1;
    if (iBone >= 0)
    {
        _matrix BoneWorld;
        if (SUCCEEDED(Get_BoneWorldMatrix(ETOUI(iBone), BoneWorld)))
            vOrigin = BoneWorld.r[3];
    }

    vOrigin = XMVectorSetY(vOrigin, XMVectorGetY(vBoss) + 0.1f);

    _vector vAim = m_pTransformCom->Get_State(STATE::LOOK);
    if (nullptr != m_pTarget)
    {
        _vector vTo = XMVectorSubtract(m_pTarget->Get_Transform()->Get_State(STATE::POSITION), vOrigin);
        if (XMVectorGetX(XMVector3LengthSq(XMVectorSetY(vTo, 0.f))) > 1e-6f)
            vAim = vTo;
    }
    vAim = XMVector3Normalize(XMVectorSetY(vAim, 0.f));

    CProjectile::FIRE_DESC fd{};
    fd.eMode = CProjectile::MODE::GROUND_WAVE;
    XMStoreFloat3(&fd.vOrigin, vOrigin);
    XMStoreFloat3(&fd.vDir, vAim);
    fd.vScale = _float3(0.01f, 0.01f, 0.01f);
    fd.fSpeed = 50.f;
    fd.fRange = 80.f;
    fd.fRadius = 0.4f;
    fd.fDamage = 900.f;
    fd.eHitType = HIT_TYPE::Heavy;
    fd.pOwner = this;
    fd.pTarget = m_pTarget;

    CGroundBall* pProjectile = m_pGameInstance->Get_PoolObject<CGroundBall>(&fd);
    if (nullptr == pProjectile)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Projectile"), pProjectile);
}

void CBoss_Raxasia_P2::Fire_ElectricBall()
{
    _vector vOrigin;
    if (false == Get_LockOnWorldPosition(vOrigin))
        vOrigin = m_pTransformCom->Get_State(STATE::POSITION);

    _vector vAim = m_pTransformCom->Get_State(STATE::LOOK);
    if (nullptr != m_pTarget)
    {
        _vector vTo = XMVectorSubtract(m_pTarget->Get_Transform()->Get_State(STATE::POSITION), vOrigin);
        if (XMVectorGetX(XMVector3LengthSq(vTo)) > 1e-6f)
            vAim = vTo;
    }
    vAim = XMVector3Normalize(vAim);

    CProjectile::FIRE_DESC fd{};
    fd.eMode = CProjectile::MODE::SHOT;
    XMStoreFloat3(&fd.vOrigin, vOrigin);
    XMStoreFloat3(&fd.vDir, vAim);
    fd.vScale = _float3(0.01f, 0.01f, 0.01f);
    fd.fSpeed = m_fThunderSpeed;
    fd.fAccel = 5.f;
    fd.fRange = m_fThunderRange;
    fd.fRadius = m_fThunderRadius;
    fd.fDamage = m_fThunderDamage;
    fd.eHitType = HIT_TYPE::Heavy;
    fd.pOwner = this;
    fd.pTarget = m_pTarget;

    CElectricBall* pProjectile = m_pGameInstance->Get_PoolObject<CElectricBall>(&fd);
    if (nullptr == pProjectile)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Projectile"), pProjectile);
}

_vector CBoss_Raxasia_P2::Calc_ThunderPos(const THUNDER_OFFSET& Off) const
{
    _vector vLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    vLook = XMVector3Normalize(vLook);

    const _vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook));

    _vector vBase = m_pTransformCom->Get_State(STATE::POSITION);

    _vector vLockOn;
    if (true == const_cast<CBoss_Raxasia_P2*>(this)->Get_LockOnWorldPosition(vLockOn))
        vBase = vLockOn;

    _vector vOut = vBase;
    vOut = XMVectorAdd(vOut, XMVectorScale(vLook, Off.fForward));
    vOut = XMVectorAdd(vOut, XMVectorScale(vRight, Off.fRight));
    vOut = XMVectorSetY(vOut, XMVectorGetY(vBase) + m_fThunderLift + Off.fUp);

    return vOut;
}

void CBoss_Raxasia_P2::Reserve_Thunderstroke()
{
    if (true == m_ThunderOffsets.empty())
        return;

    const THUNDER_OFFSET& Off = m_ThunderOffsets[m_iThunderIndex % m_ThunderOffsets.size()];
    ++m_iThunderIndex;

    _vector vLookFlat = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vLookFlat)) < 1e-6f)
        vLookFlat = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    vLookFlat = XMVector3Normalize(vLookFlat);

    const _vector vRightFlat = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLookFlat));

    _vector vAim = vLookFlat;
    if (Off.fRight > 0.f)
        vAim = XMVector3Normalize(XMVectorAdd(vLookFlat, vRightFlat));
    else if (Off.fRight < 0.f)
        vAim = XMVector3Normalize(XMVectorSubtract(vLookFlat, vRightFlat));

    THUNDER_RESERVE tReserve{};
    tReserve.fRemain = m_fThunderDelay;
    XMStoreFloat3(&tReserve.vOrigin, Calc_ThunderPos(Off));
    XMStoreFloat3(&tReserve.vDir, vAim);

    if (false == m_strThunderWarnFX.empty())
        m_pGameInstance->Play_Effect(m_strThunderWarnFX, tReserve.vOrigin);

    m_ThunderReserves.push_back(tReserve);
}

void CBoss_Raxasia_P2::Update_Thunderstroke(_float fTimeDelta)
{
    if (true == m_ThunderReserves.empty())
        return;

    if (true == m_bDead || true == Is_Dead())
    {
        m_ThunderReserves.clear();
        return;
    }

    for (auto Iter = m_ThunderReserves.begin(); Iter != m_ThunderReserves.end(); )
    {
        Iter->fRemain -= fTimeDelta;

        if (Iter->fRemain > 0.f)
        {
            ++Iter;
            continue;
        }

        Fire_ReservedThunder(*Iter);
        Iter = m_ThunderReserves.erase(Iter);
    }
}


void CBoss_Raxasia_P2::Fire_ReservedThunder(const THUNDER_RESERVE& tReserve)
{
    CProjectile::FIRE_DESC fd{};
    fd.eMode = CProjectile::MODE::SHOT;
    fd.vScale = _float3(0.01f, 0.01f, 0.01f);
    fd.fSpeed = m_fThunderSpeed;
    fd.fAccel = m_fThunderAccel;
    fd.fRange = m_fThunderRange;
    fd.fRadius = 0.6f;
    fd.fDamage = m_fThunderDamage;
    fd.eHitType = HIT_TYPE::Heavy;
    fd.pOwner = this;
    fd.pTarget = m_pTarget;
    fd.vOrigin = tReserve.vOrigin;
    fd.vDir = tReserve.vDir;

    CElectricBall* pProjectile = m_pGameInstance->Get_PoolObject<CElectricBall>(&fd);
    if (nullptr == pProjectile)
        return;

    m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Projectile"), pProjectile);
}

HRESULT CBoss_Raxasia_P2::Ready_LinkTable()
{
    m_Links.clear();

    const _wstring strPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Raxasia_P2_LinkTable.json";

    ifstream File(strPath);
    if (false == File.is_open())
        return E_FAIL;

    json Root = {};
    File >> Root;

    if (false == Root.contains("Edges") || false == Root["Edges"].is_array())
        return E_FAIL;

    for (const json& E : Root["Edges"])
    {
        P2_LINK Link{};
        Link.iFrom = E["FromID"].get<_int>();
        Link.iTo = E["ToID"].get<_int>();
        Link.bUseRange = E["UseRange"].get<_bool>();
        Link.fDistMin = E["DistMin"].get<_float>();
        Link.fDistMax = E["DistMax"].get<_float>();
        Link.bUsePercent = E["UsePercent"].get<_bool>();
        Link.fPercent = E["Percent"].get<_float>();
        Link.bUseAngle = E["UseAngle"].get<_bool>();
        Link.fAngleStart = E["AngleStart"].get<_float>();
        Link.fAngleEnd = E["AngleEnd"].get<_float>();
        Link.bUseCool = E["UseCool"].get<_bool>();
        Link.fCool = E["Cool"].get<_float>();
        Link.iPriority = E["Priority"].get<_int>();
        Link.iWinStart = E.contains("WinStart") ? E["WinStart"].get<_int>() : -1;
        Link.iWinEnd = E.contains("WinEnd") ? E["WinEnd"].get<_int>() : -1;

        m_Links.push_back(Link);
    }

    m_Entries.clear();

    if (true == Root.contains("Entries") && true == Root["Entries"].is_array())
    {
        for (const json& En : Root["Entries"])
        {
            P2_ENTRY Entry{};
            Entry.iPattern = En["PatternID"].get<_int>();
            Entry.fDistMin = En["DistMin"].get<_float>();
            Entry.fDistMax = En["DistMax"].get<_float>();

            m_Entries.push_back(Entry);
        }
    }

    m_Warps.clear();

    if (true == Root.contains("Warps") && true == Root["Warps"].is_array())
    {
        for (const json& Wp : Root["Warps"])
        {
            P2_WARP Warp{};
            Warp.iPattern = Wp["PatternID"].get<_int>();
            Warp.fOffset = Wp["Offset"].get<_float>();
            Warp.fSpeed = Wp["Speed"].get<_float>();

            m_Warps.push_back(Warp);
        }
    }

    m_TurnSpeed.clear();

    if (true == Root.contains("Turn") && true == Root["Turn"].is_array())
    {
        for (const json& Tn : Root["Turn"])
        {
            const _int iPattern = Tn["PatternID"].get<_int>();
            const _float fDeg = Tn["Deg"].get<_float>();
            m_TurnSpeed[iPattern] = XMConvertToRadians(fDeg);
        }
    }

    m_RotWindows.clear();

    if (true == Root.contains("RotWin") && true == Root["RotWin"].is_array())
    {
        for (const json& Rw : Root["RotWin"])
        {
            P2_ROTWIN W{};
            W.iPattern = Rw["PatternID"].get<_int>();
            W.iF0 = Rw["F0"].get<_int>();
            W.iF1 = Rw["F1"].get<_int>();
            W.fRad = XMConvertToRadians(Rw["Deg"].get<_float>());
            m_RotWindows.push_back(W);
        }
    }

    m_FWarps.clear();

    if (true == Root.contains("FWarp") && true == Root["FWarp"].is_array())
    {
        for (const json& Fw : Root["FWarp"])
        {
            P2_FWARP W{};
            W.iPattern = Fw["PatternID"].get<_int>();
            W.iF0 = Fw["F0"].get<_int>();
            W.iF1 = Fw["F1"].get<_int>();
            W.fOffset = Fw["Offset"].get<_float>();
            W.fSpeed = Fw["Speed"].get<_float>();
            W.iChance = Fw.value("Chance", 100);
            m_FWarps.push_back(W);
        }
    }

    m_FGlides.clear();

    if (true == Root.contains("FGlide") && true == Root["FGlide"].is_array())
    {
        for (const json& Fg : Root["FGlide"])
        {
            P2_FGLIDE W{};
            W.iPattern = Fg["PatternID"].get<_int>();
            W.iF0 = Fg["F0"].get<_int>();
            W.iF1 = Fg["F1"].get<_int>();
            W.fSpeed = Fg["Speed"].get<_float>();
            W.fOffset = Fg["Offset"].get<_float>();
            m_FGlides.push_back(W);
        }
    }

    m_OrbitGlides.clear();
    if (true == Root.contains("OrbitGlide") && true == Root["OrbitGlide"].is_array())
    {
        for (const json& Og : Root["OrbitGlide"])
        {
            P2_ORBITGLIDE W{};
            W.iPattern = Og.contains("PatternID") ? Og["PatternID"].get<_int>() : -1;
            W.iDir = Og.contains("Dir") ? Og["Dir"].get<_int>() : 1;
            W.iF0 = Og.contains("F0") ? Og["F0"].get<_int>() : 0;
            W.iF1 = Og.contains("F1") ? Og["F1"].get<_int>() : 0;
            W.fSpeed = Og.contains("Speed") ? Og["Speed"].get<_float>() : 0.f;
            m_OrbitGlides.push_back(W);
        }
    }

    m_StepHoming.clear();

    if (true == Root.contains("StepHoming") && true == Root["StepHoming"].is_array())
    {
        for (const json& Sh : Root["StepHoming"])
            m_StepHoming.insert(Sh.get<_int>());
    }

    m_MoveScale.clear();

    if (true == Root.contains("MoveScale") && true == Root["MoveScale"].is_array())
    {
        for (const json& Ms : Root["MoveScale"])
        {
            const _int iPattern = Ms["PatternID"].get<_int>();
            m_MoveScale[iPattern] = Ms["Scale"].get<_float>();
        }
    }

    if (true == Root.contains("StepInterval"))
        m_fStepInterval = Root["StepInterval"].get<_float>();

    if (true == Root.contains("HomeStep"))
        m_fHomeStep = Root["HomeStep"].get<_float>();

    if (true == Root.contains("HomeStopDist"))
        m_fHomeStopDist = Root["HomeStopDist"].get<_float>();

    if (true == Root.contains("HomeMode"))
        m_iHomeMode = Root["HomeMode"].get<_int>();

    if (true == Root.contains("HomeRatio"))
        m_fHomeRatio = Root["HomeRatio"].get<_float>();

    if (true == Root.contains("GlideMinGap"))
        m_fGlideMinGap = Root["GlideMinGap"].get<_float>();

    m_OrbitBlink.clear();

    if (true == Root.contains("OrbitBlink") && true == Root["OrbitBlink"].is_array())
    {
        for (const json& Ob : Root["OrbitBlink"])
            m_OrbitBlink[Ob["PatternID"].get<_int>()] = Ob["Dir"].get<_int>();
    }

    if (true == Root.contains("OrbitDist"))
        m_fOrbitDist = Root["OrbitDist"].get<_float>();

    if (true == Root.contains("OrbitAngle"))
        m_fOrbitAngle = Root["OrbitAngle"].get<_float>();

    m_PatternGroup.clear();

    if (true == Root.contains("SkillGroup") && true == Root["SkillGroup"].is_array())
    {
        for (const json& Sg : Root["SkillGroup"])
            m_PatternGroup[Sg["PatternID"].get<_int>()] = Sg["Group"].get<_int>();
    }

    if (true == Root.contains("GroupCooldown"))
        m_fGroupCooldown = Root["GroupCooldown"].get<_float>();

    m_ChargeIn.clear();

    if (true == Root.contains("ChargeIn") && true == Root["ChargeIn"].is_array())
    {
        for (const json& Ci : Root["ChargeIn"])
            m_ChargeIn.insert(Ci.get<_int>());
    }

    m_FaceMove.clear();

    if (true == Root.contains("FaceMove") && true == Root["FaceMove"].is_array())
    {
        for (const json& Fm : Root["FaceMove"])
            m_FaceMove.insert(Fm.get<_int>());
    }

    if (true == Root.contains("FaceSpeed"))
        m_fFaceSpeed = Root["FaceSpeed"].get<_float>();

    if (true == Root.contains("Thunderstroke"))
    {
        const json& Th = Root["Thunderstroke"];
        m_fThunderDelay = Th.contains("Delay") ? Th["Delay"].get<_float>() : 1.0f;
        m_fThunderLift = Th.contains("Lift") ? Th["Lift"].get<_float>() : 0.3f;
        m_fThunderSpeed = Th.contains("Speed") ? Th["Speed"].get<_float>() : 14.f;
        m_fThunderRange = Th.contains("Range") ? Th["Range"].get<_float>() : 40.f;
        m_fThunderRadius = Th.contains("Radius") ? Th["Radius"].get<_float>() : 0.4f;
        m_fThunderDamage = Th.contains("Damage") ? Th["Damage"].get<_float>() : 20.f;
        m_strThunderWarnFX = Th.contains("WarnFX") ? Th["WarnFX"].get<string>() : string();

        m_ThunderOffsets.clear();
        if (true == Th.contains("Offsets") && true == Th["Offsets"].is_array())
        {
            for (const json& Of : Th["Offsets"])
            {
                THUNDER_OFFSET Tof{};
                Tof.fRight = Of.contains("Right") ? Of["Right"].get<_float>() : 0.f;
                Tof.fForward = Of.contains("Forward") ? Of["Forward"].get<_float>() : 0.f;
                Tof.fUp = Of.contains("Up") ? Of["Up"].get<_float>() : 0.f;
                m_ThunderOffsets.push_back(Tof);
            }
        }
    }

#ifdef _DEBUG
    m_DbgClipNames.clear();
    if (true == Root.contains("Nodes") && true == Root["Nodes"].is_array())
    {
        for (const json& Nd : Root["Nodes"])
        {
            const _int iPattern = Nd["PatternID"].get<_int>();
            const string strClip = Nd.value("Clip", string());
            m_DbgClipNames[iPattern] = _wstring(strClip.begin(), strClip.end());
        }
    }

    {
        wchar_t szBuf[192];
        swprintf_s(szBuf, L"[P2][DBG] LinkTable loaded: RotWin=%d FWarp=%d FGlide=%d Turn=%d Entries=%d\n",
            static_cast<_int>(m_RotWindows.size()), static_cast<_int>(m_FWarps.size()),
            static_cast<_int>(m_FGlides.size()),
            static_cast<_int>(m_TurnSpeed.size()), static_cast<_int>(m_Entries.size()));
        OutputDebugStringW(szBuf);
    }
#endif

    return S_OK;
}

void CBoss_Raxasia_P2::Collect_AuthoredStates()
{
    m_AuthoredStates.clear();

    const _wstring strPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/Models/Raxasia_P2/Raxasia_P2_AnimClipSettings.json";

    ifstream File(strPath);
    if (false == File.is_open())
        return;

    json Root = {};
    File >> Root;

    if (false == Root.contains("Controller") || false == Root["Controller"].is_object())
        return;

    const json& Controller = Root["Controller"];

    const _uint64 iPatternHash = Hash_String(L"PatternID");

    unordered_set<_uint64> AttackStates;

    if (true == Controller.contains("ActionStateMaps") && true == Controller["ActionStateMaps"].is_array())
    {
        for (const json& Map : Controller["ActionStateMaps"])
        {
            if (Map.value("ActionName", string()) == "ATTACK")
                AttackStates.emplace(Map.value("StateHash", 0ull));
        }
    }

    if (false == Controller.contains("States") || false == Controller["States"].is_array())
        return;

    for (const json& State : Controller["States"])
    {
        if (static_cast<_int>(ANIM_STATE_ENTRY_MODE::REQUEST) != State.value("EntryMode", 0))
            continue;

        const _uint64 iNameHash = State.value("NameHash", 0ull);

        if (AttackStates.end() == AttackStates.find(iNameHash))
            continue;

        if (false == State.contains("Conditions") || false == State["Conditions"].is_array())
            continue;

        for (const json& Cond : State["Conditions"])
        {
            if (iPatternHash != Cond.value("ParameterHash", 0ull))
                continue;

            if (static_cast<_int>(ANIM_CONTROLLER_COMPARE_TYPE::EQUAL) != Cond.value("CompareType", 0))
                continue;

            m_AuthoredStates.emplace(Cond.value("IntValue", 0));
        }
    }
}

HRESULT CBoss_Raxasia_P2::Ready_Weapon()
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
    WeaponDesc.fDamage = 1200.f;
    WeaponDesc.bRender = true;
    WeaponDesc.bPhase2 = true;

    if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_GameObject_Weapon_Raxasia"), TEXT("Obj_Weapon"), &WeaponDesc)))
        return E_FAIL;

    m_pWeapon = static_cast<CWeapon_Monster*>(Get_PartObject(TEXT("Obj_Weapon")));

    return S_OK;
}

HRESULT CBoss_Raxasia_P2::Ready_Shield()
{
    const _float4x4* pShieldSocket = Get_Bone_CombinedMatPtr("BN_Weapon_L");
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

HRESULT CBoss_Raxasia_P2::Ready_HitBoxes()
{
    m_HitBoxes.Add_Weapon(L"Weapon", m_pWeapon, 1200.f);
    m_HitBoxes.Add_Weapon(L"Shield", m_pShield, 800.f);
    m_HitBoxes.Add_BoneSphere(L"Body", "Bip001-Spine1", 2.5f, 700.f);
    m_HitBoxes.Add_BoneSphere(L"Root", "Root", 2.5f, 700.f);
    m_HitBoxes.Add_BoneSphere(L"KickLeft", "Bip001-L-Foot", 0.4f, 600.f);
    m_HitBoxes.Add_BoneSphere(L"KickRight", "Bip001-R-Foot", 0.4f, 600.f);

    if (FAILED(m_HitBoxes.Build(m_pModelCom, m_pDevice, m_pContext)))
        return E_FAIL;

    return S_OK;
}

void CBoss_Raxasia_P2::Mirror_ControllerParams()
{
    const _bool bDeadNow = m_bDead || Is_Dead();
    m_pBlackBoard->Set<_bool>(L"bDead", bDeadNow && false == m_pBlackBoard->Get<_bool>(L"bFatal"));

    Set_AnimBool(L"Dead", bDeadNow && false == m_bDead);
    Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
    Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
}

void CBoss_Raxasia_P2::Update_CombatParams()
{
    m_fDistToTarget = 9999.f;
    m_fAngleToTarget = 0.f;

    _vector vTarget;
    if (true == Get_TargetPos(vTarget) && nullptr != m_pTransformCom)
    {
        _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
        _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);
        m_fDistToTarget = XMVectorGetX(XMVector3Length(vTo));

        if (m_fDistToTarget > 0.001f)
        {
            _vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
            _vector vDir = XMVector3Normalize(vTo);

            const _float fCos = XMVectorGetX(XMVector3Dot(vLook, vDir));
            const _float fCross = XMVectorGetY(XMVector3Cross(vLook, vDir));

            _float fAngle = XMConvertToDegrees(atan2f(fCross, fCos));
            if (fAngle < 0.f)
                fAngle += 360.f;

            m_fAngleToTarget = fAngle;
        }
    }

    m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", m_bPerceived);
}

_bool CBoss_Raxasia_P2::Pattern_HasRotWindows(_int iPatternID) const
{
    if (iPatternID < 0)
        return false;

    for (const P2_ROTWIN& W : m_RotWindows)
    {
        if (W.iPattern == iPatternID)
            return true;
    }

    return false;
}

_bool CBoss_Raxasia_P2::Find_RotSpeed(_int iPatternID, _int iFrame, _float& fOutRad) const
{
    for (const P2_ROTWIN& W : m_RotWindows)
    {
        if (W.iPattern != iPatternID)
            continue;

        if (iFrame < W.iF0 || iFrame > W.iF1)
            continue;

        fOutRad = W.fRad;
        return true;
    }

    return false;
}

void CBoss_Raxasia_P2::Drive_Steering(_float fTimeDelta)
{
    if (nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    Set_AnimFloat(L"Speed", 0.f);

    _vector vTarget;
    if (false == Get_TargetPos(vTarget))
        return;

    if (m_iShowcaseIdx < m_ShowcaseSeq.size() && m_iCurPatternID < 0)
    {
        _float fNear = 0.f, fFar = 5.f;
        Get_ShowcaseBand(m_iShowcaseIdx, fNear, fFar);

        if (m_fDistToTarget > fFar)
        {
            Set_AnimFloat(L"Speed", m_fIntroRunSpeed);
            m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fFaceSpeed));
            m_pMovementCom->TurnTo(vTarget, fTimeDelta);
            return;
        }

        if (m_fDistToTarget < fNear)
        {
            _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
            _vector vAway = XMVectorAdd(vMy, XMVectorSubtract(vMy, vTarget));

            Set_AnimFloat(L"Speed", m_fIntroRunSpeed);
            m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fFaceSpeed));
            m_pMovementCom->TurnTo(vAway, fTimeDelta);
            return;
        }
    }

    if (true == Is_StepHoming(m_iCurPatternID))
    {
        if (m_iCurPatternID != m_iStepPattern)
        {
            m_iStepPattern = m_iCurPatternID;
            m_fStepTimer = 0.f;
            XMStoreFloat4(&m_vStepTarget, vTarget);
        }

        m_fStepTimer += fTimeDelta;
        if (m_fStepTimer >= m_fStepInterval)
        {
            m_fStepTimer = 0.f;
            XMStoreFloat4(&m_vStepTarget, vTarget);
        }

        if ((nullptr != m_pWeapon && true == m_pWeapon->Is_HitSuccess())
            || (nullptr != m_pShield && true == m_pShield->Is_HitSuccess()))
            return;

        _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
        _vector vDir = XMVectorSetY(XMVectorSubtract(XMLoadFloat4(&m_vStepTarget), vMy), 0.f);
        const _float fDist = XMVectorGetX(XMVector3Length(vDir));

        if (fDist > m_fHomeStopDist)
        {
            vDir = XMVector3Normalize(vDir);
            const _float fYaw = atan2f(XMVectorGetX(vDir), XMVectorGetZ(vDir));
            _vector vTargetQuat = XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f);

            _float fRatio;
            if (1 == m_iHomeMode)
            {
                fRatio = m_fHomeRatio;
            }
            else
            {
                _float fDot = fabsf(XMVectorGetX(XMQuaternionDot(m_pTransformCom->Get_Quat(), vTargetQuat)));
                if (fDot > 1.f)
                    fDot = 1.f;
                const _float fAngle = 2.f * acosf(fDot);
                fRatio = (fAngle > 0.0001f) ? min(1.f, m_fHomeStep / fAngle) : 1.f;
            }

            m_pTransformCom->Smooth_Turn(vTargetQuat, fRatio);
        }

        return;
    }

    m_iStepPattern = -1;

    if (m_FaceMove.end() != m_FaceMove.find(m_iCurPatternID))
    {
        m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fFaceSpeed));
        m_pMovementCom->TurnTo(vTarget, fTimeDelta);
        return;
    }

    if (true == Pattern_HasRotWindows(m_iCurPatternID))
    {
        const _int iFrame = (nullptr != m_pAnimator) ? static_cast<_int>(m_pAnimator->Get_LocalFrame()) : -1;

        _float fRad = 0.f;
        if (true == Find_RotSpeed(m_iCurPatternID, iFrame, fRad))
        {
            m_pTransformCom->Set_RotationPerSec(fRad);
            m_pMovementCom->TurnTo(vTarget, fTimeDelta);
        }

        return;
    }

    if (false == Can_AnimRotate())
        return;

    auto iter = m_TurnSpeed.find(m_iCurPatternID);
    if (m_TurnSpeed.end() != iter)
        m_pTransformCom->Set_RotationPerSec(iter->second);

    m_pMovementCom->TurnTo(vTarget, fTimeDelta);
}

void CBoss_Raxasia_P2::Drive_FrameWarp(_float fTimeDelta)
{
    if (m_iCurPatternID < 0 || nullptr == m_pAnimator || nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    const _int iFrame = static_cast<_int>(m_pAnimator->Get_LocalFrame());

    _int iWin = -1;
    for (_int i = 0; i < static_cast<_int>(m_FWarps.size()); ++i)
    {
        const P2_FWARP& W = m_FWarps[i];
        if (W.iPattern != m_iCurPatternID)
            continue;
        if (iFrame < W.iF0 || iFrame > W.iF1)
            continue;
        iWin = i;
        break;
    }

    if (iWin < 0)
    {
        m_iActiveFWarp = -1;
        return;
    }

    if (iWin == m_iActiveFWarp)
        return;

    m_iActiveFWarp = iWin;

    const P2_FWARP& W = m_FWarps[iWin];

    if (W.iChance < 100 && (rand() % 100) >= W.iChance)
        return;

    _vector vTarget;
    if (false == Get_TargetPos(vTarget))
        return;

    _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);
    const _float fDist = XMVectorGetX(XMVector3Length(vTo));

    _vector vDest;

    auto iterOrbit = m_OrbitBlink.find(m_iCurPatternID);
    if (m_OrbitBlink.end() != iterOrbit)
    {
        _vector vFromP = XMVectorSetY(XMVectorSubtract(vMy, vTarget), 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vFromP)) < 1e-4f)
            vFromP = XMVectorNegate(vTo);

        const _float fSign = (iterOrbit->second < 0) ? -1.f : 1.f;
        _vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, XMConvertToRadians(m_fOrbitAngle) * fSign, 0.f);
        _vector vRot = XMVector3Normalize(XMVector3Rotate(vFromP, vQuat));

        vDest = XMVectorAdd(vTarget, XMVectorScale(vRot, m_fOrbitDist));
        vDest = XMVectorSetY(vDest, XMVectorGetY(vMy));
    }
    else
    {
        const _float fOffM = W.fOffset * 0.01f;
        if (fDist <= fOffM)
            return;

        _vector vDir = XMVector3Normalize(vTo);
        vDest = XMVectorSubtract(vTarget, XMVectorScale(vDir, fOffM));
        vDest = XMVectorSetY(vDest, XMVectorGetY(vMy));
    }

    _float fTravel = XMVectorGetX(XMVector3Length(XMVectorSubtract(vDest, vMy)));
    _float fDur = fTravel / (W.fSpeed * 0.01f);
    fDur = min(max(fDur, 0.03f), 0.6f);

    m_pMovementCom->Begin_MoveTo(vDest, m_pTransformCom->Get_Quat(), fDur);
}

void CBoss_Raxasia_P2::Drive_FrameGlide(_float fTimeDelta)
{
    if (m_iCurPatternID < 0 || nullptr == m_pAnimator || nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    if (m_OrbitBlink.end() != m_OrbitBlink.find(m_iCurPatternID))
        return;

    const _int iFrame = static_cast<_int>(m_pAnimator->Get_LocalFrame());

    const P2_FGLIDE* pWin = nullptr;
    for (const P2_FGLIDE& W : m_FGlides)
    {
        if (W.iPattern != m_iCurPatternID)
            continue;
        if (iFrame < W.iF0 || iFrame > W.iF1)
            continue;
        pWin = &W;
        break;
    }

    if (nullptr == pWin)
        return;

    _vector vTarget;
    if (false == Get_TargetPos(vTarget))
        return;

    _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);
    const _float fDist = XMVectorGetX(XMVector3Length(vTo));

    _float fOffM = pWin->fOffset * 0.01f;
    if (m_ChargeIn.end() == m_ChargeIn.find(m_iCurPatternID))
    {
        if (fOffM < m_fGlideMinGap)
            fOffM = m_fGlideMinGap;
    }
    if (fDist <= fOffM)
        return;

    m_pMovementCom->Set_HomingSpeed(pWin->fSpeed * 0.01f);
    m_pMovementCom->Set_MoveDir(vTo);

#ifdef _DEBUG
    if (m_iDbgGlideWin != pWin->iF0)
    {
        m_iDbgGlideWin = pWin->iF0;
        wchar_t szBuf[160];
        swprintf_s(szBuf, L"[P2][GLIDE] pat=%d frame=%d win=%d-%d dist=%.2f homeSpd=%.1f off=%.1f\n",
            m_iCurPatternID, iFrame, pWin->iF0, pWin->iF1, fDist, pWin->fSpeed * 0.01f, fOffM);
        OutputDebugStringW(szBuf);
    }
#endif
}

void CBoss_Raxasia_P2::Drive_OrbitGlide(_float fTimeDelta)
{
    if (m_iCurPatternID < 0 || nullptr == m_pAnimator || nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    const _int iFrame = static_cast<_int>(m_pAnimator->Get_LocalFrame());

    const P2_ORBITGLIDE* pWin = nullptr;
    for (const P2_ORBITGLIDE& W : m_OrbitGlides)
    {
        if (W.iPattern == m_iCurPatternID && iFrame >= W.iF0 && iFrame <= W.iF1)
        {
            pWin = &W;
            break;
        }
    }
    if (nullptr == pWin)
        return;

    _vector vTarget;
    if (false == Get_TargetPos(vTarget))
        return;

    const _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);

    _vector vFromP = XMVectorSetY(XMVectorSubtract(vMy, vTarget), 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vFromP)) < 1e-4f)
        vFromP = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
    vFromP = XMVector3Normalize(vFromP);

    const _float fSign = (pWin->iDir >= 0) ? 1.f : -1.f;
    const _vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, XMConvertToRadians(m_fOrbitAngle) * fSign, 0.f);
    const _vector vRot = XMVector3Normalize(XMVector3Rotate(vFromP, vQuat));
    _vector vDest = XMVectorAdd(vTarget, XMVectorScale(vRot, m_fOrbitDist));

    const _vector vTo = XMVectorSetY(XMVectorSubtract(vDest, vMy), 0.f);

    m_pMovementCom->Set_HomingSpeed(pWin->fSpeed * 0.01f);
    m_pMovementCom->Set_MoveDir(vTo);

    m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fFaceSpeed));
    m_pMovementCom->TurnTo(vTarget, fTimeDelta);
}

_float CBoss_Raxasia_P2::Calc_RootMotionScale() const
{
    if (m_FaceMove.end() != m_FaceMove.find(m_iCurPatternID))
        return 0.f;

    if (m_iCurPatternID >= 0 && nullptr != m_pAnimator)
    {
        const _int iFrame = static_cast<_int>(m_pAnimator->Get_LocalFrame());

        for (const P2_FWARP& W : m_FWarps)
        {
            if (W.iPattern == m_iCurPatternID && iFrame >= W.iF0 && iFrame <= W.iF1)
                return 0.f;
        }

        for (const P2_ORBITGLIDE& W : m_OrbitGlides)
        {
            if (W.iPattern == m_iCurPatternID && iFrame >= W.iF0 && iFrame <= W.iF1)
                return 0.f;
        }
    }

    _float fScale = __super::Calc_RootMotionScale();

    auto iter = m_MoveScale.find(m_iCurPatternID);
    if (m_MoveScale.end() != iter)
        fScale *= iter->second;

    return fScale;
}

void CBoss_Raxasia_P2::Begin_Charge()
{
    m_bCharging = true;
    m_fChargeTimer = 0.f;

    Commit_Pattern(107);
}

void CBoss_Raxasia_P2::Update_Charge(_float fTimeDelta)
{
    if (false == m_bCharging)
        return;

    m_fChargeTimer += fTimeDelta;

    if (m_fChargeTimer >= m_fChargeHold)
        End_Charge();
}

void CBoss_Raxasia_P2::End_Charge()
{
    m_bCharging = false;

    Commit_Pattern(41);
}

_bool CBoss_Raxasia_P2::Is_StepHoming(_int iPatternID) const
{
    return m_StepHoming.find(iPatternID) != m_StepHoming.end();
}

_bool CBoss_Raxasia_P2::Is_AnimIdle() const
{
    return Is_AnimInState(L"IDLE");
}

_bool CBoss_Raxasia_P2::Is_CurClipLooping() const
{
    if (nullptr == m_pAnimator)
        return false;

    const ANIM_CLIP_SETTING* pClip = m_pAnimator->Get_CurClip();

    return (nullptr != pClip) ? pClip->bLoop : false;
}

_bool CBoss_Raxasia_P2::Has_PatternState(_int iPatternID) const
{
    return m_AuthoredStates.find(iPatternID) != m_AuthoredStates.end();
}

void CBoss_Raxasia_P2::Get_ShowcaseBand(_uint iIdx, _float& fNear, _float& fFar) const
{
    fNear = 0.f;
    fFar = (iIdx < m_ShowcaseDist.size()) ? m_ShowcaseDist[iIdx] : 5.f;

    if (iIdx >= m_ShowcaseSeq.size())
        return;

    const _int iPat = m_ShowcaseSeq[iIdx];

    for (const P2_ENTRY& Entry : m_Entries)
    {
        if (Entry.iPattern == iPat)
        {
            fNear = Entry.fDistMin;
            fFar = Entry.fDistMax;
            return;
        }
    }
}

_bool CBoss_Raxasia_P2::In_LinkWindow(_int iFromID, _int iToID) const
{
    if (nullptr == m_pAnimator)
        return false;

    const _int iFrame = static_cast<_int>(m_pAnimator->Get_LocalFrame());

    for (const P2_LINK& Link : m_Links)
    {
        if (Link.iFrom != iFromID || Link.iTo != iToID)
            continue;

        if (Link.iWinStart < 0)
            continue;

        if (iFrame >= Link.iWinStart && iFrame <= Link.iWinEnd)
            return true;
    }

    return false;
}

_int CBoss_Raxasia_P2::Pick_NextLink(_int iFromID, _bool bWindowOnly)
{
    const _int iFrame = (nullptr != m_pAnimator) ? static_cast<_int>(m_pAnimator->Get_LocalFrame()) : -1;

    vector<const P2_LINK*> Cand;

    for (const P2_LINK& Link : m_Links)
    {
        if (Link.iFrom != iFromID)
            continue;

        if (true == bWindowOnly)
        {
            if (Link.iWinStart < 0)
                continue;

            if (iFrame < Link.iWinStart || iFrame > Link.iWinEnd)
                continue;
        }

        if (false == Has_PatternState(Link.iTo))
            continue;

        Cand.push_back(&Link);
    }

    _bool bRangedInBand = false;

    for (const P2_LINK* pLink : Cand)
    {
        if (false == pLink->bUseRange)
            continue;

        if (m_fDistToTarget < pLink->fDistMin || m_fDistToTarget > pLink->fDistMax)
            continue;

        bRangedInBand = true;
        break;
    }

    vector<const P2_LINK*> Pass;

    for (const P2_LINK* pLink : Cand)
    {
        if (true == bRangedInBand && false == pLink->bUseRange)
            continue;

        if (true == pLink->bUseRange)
        {
            if (m_fDistToTarget < pLink->fDistMin || m_fDistToTarget > pLink->fDistMax)
                continue;
        }

        if (true == pLink->bUseAngle)
        {
            if (false == In_AngleRange(m_fAngleToTarget, pLink->fAngleStart, pLink->fAngleEnd))
                continue;
        }

        if (true == pLink->bUseCool && pLink->fCool > 0.f)
        {
            auto iter = m_PatternLastUsed.find(pLink->iTo);

            if (m_PatternLastUsed.end() != iter && m_fRunnerTime - iter->second < pLink->fCool)
                continue;
        }

        Pass.push_back(pLink);
    }

    vector<const P2_LINK*> Fresh;

    for (const P2_LINK* pLink : Pass)
    {
        auto iter = m_PatternLastUsed.find(pLink->iTo);

        if (m_PatternLastUsed.end() != iter && m_fRunnerTime - iter->second < m_fRepeatCooldown)
            continue;

        auto iterG = m_PatternGroup.find(pLink->iTo);
        if (m_PatternGroup.end() != iterG)
        {
            auto iterGU = m_GroupLastUsed.find(iterG->second);
            if (m_GroupLastUsed.end() != iterGU && m_fRunnerTime - iterGU->second < m_fGroupCooldown)
                continue;
        }

        Fresh.push_back(pLink);
    }

    if (false == Fresh.empty())
        Pass = Fresh;

    vector<const P2_LINK*> Ready;

    for (const P2_LINK* pLink : Pass)
    {
        if (false == pLink->bUsePercent)
        {
            Ready.push_back(pLink);
            continue;
        }

        auto iter = m_LinkRoll.find(pLink->iTo);

        if (m_LinkRoll.end() == iter)
        {
            const _float fRoll = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * 100.f;
            iter = m_LinkRoll.emplace(pLink->iTo, (fRoll <= pLink->fPercent)).first;
        }

        if (true == iter->second)
            Ready.push_back(pLink);
    }

    if (true == Ready.empty())
        return -1;

    _float fTotal = 0.f;
    for (const P2_LINK* pLink : Ready)
        fTotal += 1.f / static_cast<_float>(m_PatternUseCount[pLink->iTo] + 1);

    if (fTotal <= 0.f)
        return Ready.front()->iTo;

    _float fR = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;

    for (const P2_LINK* pLink : Ready)
    {
        fR -= 1.f / static_cast<_float>(m_PatternUseCount[pLink->iTo] + 1);
        if (fR <= 0.f)
            return pLink->iTo;
    }

    return Ready.back()->iTo;
}


void CBoss_Raxasia_P2::Commit_Entry()
{
    m_iCurPatternID = -1;

    if (m_iShowcaseIdx < m_ShowcaseSeq.size())
    {
        while (m_iShowcaseIdx < m_ShowcaseSeq.size() && false == Has_PatternState(m_ShowcaseSeq[m_iShowcaseIdx]))
            ++m_iShowcaseIdx;

        if (m_iShowcaseIdx < m_ShowcaseSeq.size())
        {
            _float fNear = 0.f, fFar = 5.f;
            Get_ShowcaseBand(m_iShowcaseIdx, fNear, fFar);

            if (m_fDistToTarget > fFar || m_fDistToTarget < fNear)
                return;

            Commit_Pattern(m_ShowcaseSeq[m_iShowcaseIdx]);
            ++m_iShowcaseIdx;
            return;
        }
    }

    vector<_int> Cand;

    for (const P2_ENTRY& Entry : m_Entries)
    {
        if (false == Has_PatternState(Entry.iPattern))
            continue;

        if (m_fDistToTarget < Entry.fDistMin || m_fDistToTarget > Entry.fDistMax)
            continue;

        Cand.push_back(Entry.iPattern);
    }

    if (true == Cand.empty())
    {
        Commit_Pattern(P2_ENTRY_PATTERN);
        return;
    }

    vector<_int> Fresh;

    for (_int iPattern : Cand)
    {
        auto iter = m_PatternLastUsed.find(iPattern);

        if (m_PatternLastUsed.end() != iter && m_fRunnerTime - iter->second < m_fRepeatCooldown)
            continue;

        auto iterG = m_PatternGroup.find(iPattern);
        if (m_PatternGroup.end() != iterG)
        {
            auto iterGU = m_GroupLastUsed.find(iterG->second);
            if (m_GroupLastUsed.end() != iterGU && m_fRunnerTime - iterGU->second < m_fGroupCooldown)
                continue;
        }

        Fresh.push_back(iPattern);
    }

    if (false == Fresh.empty())
        Cand = Fresh;

    _float fTotal = 0.f;
    for (_int iPattern : Cand)
        fTotal += 1.f / static_cast<_float>(m_PatternUseCount[iPattern] + 1);

    _int iPick = Cand.back();

    if (fTotal > 0.f)
    {
        _float fR = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
        for (_int iPattern : Cand)
        {
            fR -= 1.f / static_cast<_float>(m_PatternUseCount[iPattern] + 1);
            if (fR <= 0.f)
            {
                iPick = iPattern;
                break;
            }
        }
    }

    m_iComboEntry = iPick;
    Commit_Pattern(iPick);
}

const CBoss_Raxasia_P2::P2_WARP* CBoss_Raxasia_P2::Find_Warp(_int iPatternID) const
{
    for (const P2_WARP& Warp : m_Warps)
    {
        if (Warp.iPattern == iPatternID)
            return &Warp;
    }

    return nullptr;
}

void CBoss_Raxasia_P2::Begin_SkillMoveWarp(const P2_WARP& Warp)
{
    if (nullptr == m_pMovementCom || nullptr == m_pTransformCom)
        return;

    _vector vTarget;
    if (false == Get_TargetPos(vTarget))
        return;

    _vector vMy = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vTo = XMVectorSetY(XMVectorSubtract(vTarget, vMy), 0.f);

    const _float fDist = XMVectorGetX(XMVector3Length(vTo));
    if (fDist <= Warp.fOffset)
        return;

    _vector vDir = XMVector3Normalize(vTo);
    _vector vDest = XMVectorSubtract(vTarget, XMVectorScale(vDir, Warp.fOffset));
    vDest = XMVectorSetY(vDest, XMVectorGetY(vMy));

    const _float fYaw = atan2f(XMVectorGetX(vDir), XMVectorGetZ(vDir));
    _vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f);

    _float fDur = (fDist - Warp.fOffset) / Warp.fSpeed;
    fDur = min(max(fDur, 0.05f), 0.6f);

    m_pMovementCom->Begin_MoveTo(vDest, vQuat, fDur);
}

void CBoss_Raxasia_P2::Commit_Pattern(_int iPatternID)
{
    m_iPendingPatternID = iPatternID;
    m_bAttackCommit = true;
    m_bAttackStarted = false;
    m_fCommitTimer = 0.f;

    m_PatternLastUsed[iPatternID] = m_fRunnerTime;
    m_PatternUseCount[iPatternID] += 1;

    auto iterGroup = m_PatternGroup.find(iPatternID);
    if (m_PatternGroup.end() != iterGroup)
        m_GroupLastUsed[iterGroup->second] = m_fRunnerTime;
}

void CBoss_Raxasia_P2::Update_LinkRunner(_float fTimeDelta)
{
    if (false == m_bPerceived)
        return;

    m_fRunnerTime += fTimeDelta;

    m_pBlackBoard->Set<_bool>(L"bInAttackRange", false);
    m_pBlackBoard->Set<_bool>(L"bAttackReady", false);

    const _bool bIdle = Is_AnimIdle();

    if (true == bIdle)	m_fIdleTimer += fTimeDelta;
    else				m_fIdleTimer = 0.f;

    if (true == m_bAttackCommit && false == m_bAttackStarted)
    {
        m_fCommitTimer += fTimeDelta;

        m_pBlackBoard->Set<_bool>(L"bInAttackRange", true);
        m_pBlackBoard->Set<_bool>(L"bAttackReady", true);
        Set_AnimInt(L"PatternID", m_iPendingPatternID);
        Set_AnimTrigger(L"PatternStart");
        Request_AnimAction(L"ATTACK");

        if (true == Was_AnimActionAccepted())
        {
            m_bAttackStarted = true;
            m_iCurPatternID = m_iPendingPatternID;
            m_fCommitTimer = 0.f;
            m_fStuckTimer = 0.f;
            m_LinkRoll.clear();
            m_iActiveFWarp = -1;
        }
        else if (m_fCommitTimer > m_fCommitTimeOut)
        {
            m_bAttackCommit = false;
            m_iPendingPatternID = -1;
            m_fCommitTimer = 0.f;
        }

        return;
    }

    if (true == m_bAttackCommit && true == m_bAttackStarted)
    {
        m_PatternLastUsed[m_iCurPatternID] = m_fRunnerTime;

        if (m_iComboEntry >= 0)
            m_PatternLastUsed[m_iComboEntry] = m_fRunnerTime;

        const _bool bPlaying = (false == bIdle && false == Is_CurClipLooping());

        _int iNext = -1;

        if (m_iShowcaseIdx < m_ShowcaseSeq.size())
        {
            _uint iPeek = m_iShowcaseIdx;
            while (iPeek < m_ShowcaseSeq.size() && false == Has_PatternState(m_ShowcaseSeq[iPeek]))
                ++iPeek;

            if (iPeek < m_ShowcaseSeq.size() && true == In_LinkWindow(m_iCurPatternID, m_ShowcaseSeq[iPeek]))
            {
                iNext = m_ShowcaseSeq[iPeek];
                m_iShowcaseIdx = iPeek + 1;
            }
        }
        else
        {
            iNext = Pick_NextLink(m_iCurPatternID, bPlaying);
        }

        if (iNext > 0)
        {
            Commit_Pattern(iNext);
            return;
        }

        if (true == bPlaying)
        {
            m_fStuckTimer = 0.f;
            return;
        }

        m_fStuckTimer += fTimeDelta;

        if (true == bIdle || m_fStuckTimer > m_fStartedTimeout)
        {
            m_bAttackCommit = false;
            m_bAttackStarted = false;
            m_iPendingPatternID = -1;
            m_iCurPatternID = -1;
            m_iComboEntry = -1;
            m_fStuckTimer = 0.f;
        }

        return;
    }

    if (false == bIdle && true == Is_CurClipLooping())
    {
        Commit_Entry();
        return;
    }

    if (true == bIdle && m_fIdleTimer >= m_fAttackGap)
        Commit_Entry();
}


#ifdef _DEBUG
void CBoss_Raxasia_P2::Debug_MoveLog(_float fTimeDelta)
{
    if (m_iCurPatternID == m_iDbgLastState)
        return;
    m_iDbgLastState = m_iCurPatternID;

    if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
        return;

    _float3 vPos;
    XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

    _float fMoved = 0.f;
    if (true == m_bDbgPrevValid)
    {
        const _float dx = vPos.x - m_vDbgPrevPos.x;
        const _float dz = vPos.z - m_vDbgPrevPos.z;
        fMoved = sqrtf(dx * dx + dz * dz);
    }
    m_vDbgPrevPos = vPos;
    m_bDbgPrevValid = true;

    const ANIM_CLIP_SETTING* pClip = m_pAnimator->Get_CurClip();

    const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();
    _tchar szState[64] = {};
    if (nullptr != pState)
        MultiByteToWideChar(CP_ACP, 0, pState->strName.c_str(), -1, szState, 64);
    else
        wcscpy_s(szState, L"(null)");

    _tchar szBuf[640] = {};
    swprintf_s(szBuf, L"[P2] cur=%-3d state=%-26s dist=%.2f ang=%3.0f movedPrev=%.2f clipRM=%d scale=%.2f\n",
        m_iCurPatternID,
        szState,
        m_fDistToTarget,
        m_fAngleToTarget,
        fMoved,
        (nullptr != pClip) ? static_cast<_int>(pClip->bUseRootMotion) : -1,
        Calc_RootMotionScale());

    OutputDebugStringW(szBuf);
}

void CBoss_Raxasia_P2::Debug_ForceInput()
{
    if (nullptr == m_pGameInstance)
        return;

    _bool bMoved = false;

    if (true == m_pGameInstance->Key_Down(DIK_ADD))
    {
        for (_int i = 0; i < 200; ++i)
        {
            m_iDbgForceID = (m_iDbgForceID >= 108) ? 1 : m_iDbgForceID + 1;
            if (true == Has_PatternState(m_iDbgForceID))
                break;
        }
        bMoved = true;
    }
    else if (true == m_pGameInstance->Key_Down(DIK_SUBTRACT))
    {
        for (_int i = 0; i < 200; ++i)
        {
            m_iDbgForceID = (m_iDbgForceID <= 1) ? 108 : m_iDbgForceID - 1;
            if (true == Has_PatternState(m_iDbgForceID))
                break;
        }
        bMoved = true;
    }

    if (true == bMoved)
    {
        auto iter = m_DbgClipNames.find(m_iDbgForceID);
        wchar_t szBuf[128];
        swprintf_s(szBuf, L"[P2][DBG] sel = %d  (%s)\n", m_iDbgForceID, (m_DbgClipNames.end() != iter) ? iter->second.c_str() : L"?");
        OutputDebugStringW(szBuf);
    }

    if (true == m_pGameInstance->Key_Down(DIK_MULTIPLY))
        Debug_ForcePattern(m_iDbgForceID);
}

void CBoss_Raxasia_P2::Debug_ForcePattern(_int iPatternID)
{
    if (false == Has_PatternState(iPatternID))
    {
        OutputDebugStringW(L"[P2][DBG] not authored\n");
        return;
    }

    m_bPerceived = true;
    m_bAttackCommit = false;
    m_bAttackStarted = false;
    m_iCurPatternID = -1;
    m_iComboEntry = -1;
    m_LinkRoll.clear();

    Commit_Pattern(iPatternID);

    wchar_t szBuf[64];
    swprintf_s(szBuf, L"[P2][DBG] FORCE %d\n", iPatternID);
    OutputDebugStringW(szBuf);
}
#endif

void CBoss_Raxasia_P2::On_Fallen()
{
    __super::On_SetDead();

    m_bDead = true;
    m_HitBoxes.On_StateChanged();

    // P2 사망 시에는 클리어를 바로 확정하지 않고 Dead 컷신부터 재생한다.
    CUTSCENE_REQUEST CutSceneRequest{};
    CutSceneRequest.strAssetName = CUTSCENE_RAXASIA_DEAD;
    CutSceneRequest.bPlayOnce = true;
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_DEAD, &CutSceneRequest);

    // 컷신 재생 실패 시에는 P2를 정리하고 기존 보스 클리어 흐름으로 복구한다.
    if (FAILED(CutSceneRequest.hResult))
    {
        Request_Despawn();

        BossClear_EventDesc ClearDesc{};
        ClearDesc.eMonsterType = SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2;
        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, &ClearDesc);
    }
}

void CBoss_Raxasia_P2::On_GroggyStart()
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

CBoss_Raxasia_P2* CBoss_Raxasia_P2::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBoss_Raxasia_P2* pInstance = new CBoss_Raxasia_P2(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CBoss_Raxasia_P2");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CBoss_Raxasia_P2::Clone(void* pArg)
{
    CBoss_Raxasia_P2* pInstance = new CBoss_Raxasia_P2(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBoss_Raxasia_P2");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CBoss_Raxasia_P2::Free()
{
    m_pGameInstance->UnSubscribe_AllEvent(this);

    __super::Free();
    m_HitBoxes.Free();
}
