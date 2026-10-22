#pragma once
#include "Core_Defines.h"
#include "Character.h"
#include "IPoolable.h"
#include "AnimationData.h"
#include "Monster_Defines.h"
#include "Monster_Enums.h"


NS_BEGIN(Engine)
class CBehaviorTree;
class CBT_BlackBoard;
class CBT_FunctionRegistry;
class CNavigation;
class CAnimator;
class CCT;
class CEffect;

#ifdef _DEBUG
class CDebugCapsule;
#endif // _DEBUG

NS_END

NS_BEGIN(GameCore)
class CStatComponent_AI;
class CMovement_AI;
class CHitBox_Helper;

class GAMECORE_DLL CAIObject abstract : public CCharacter , public IPoolable
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CAIObject)
#endif

public:	
	typedef struct tagAIObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float		fHP = { 100.f };
		_float		fMaxHP = { 100.f };
		_float		fGroggy = { 0.f };
		_float		fMaxGroggy = { 100.f };
		_float		fGroggyDecayDelay = { 2.f };
		_float		fGroggyDecayRate = { 25.f };
		_float		fGroggyCooldownMax = { 1.f };
		_float		fDetectRange = { 10.f };
		_float		fAttackRange = { 2.f };
		_float		fLeashRange = { 30.f };
		_float		fFrontCos = { 0.5f };	
		_uint		iProtoLevel = { ETOUI(LEVEL::GAMEPLAY)};
		_float		fReaggroRange = { 8.f };
		_float		fReengageDelay = { 0.6f };
		SPAWN_MONSTER_TYPE	eMonsterType = { SPAWN_MONSTER_TYPE::END };

		class Engine::CNavigation* pNavigation = { nullptr };
	} AIOBJECT_DESC;

	typedef struct tagCharacterSetupDesc
	{
		_wstring strShaderTag = { };
		_wstring strModelPrototypeTag = { };
		_wstring strAnimFolderTag = { };
		_wstring strAnimTag = { };
		_uint	 iModelLevel = { };
		const char* pRootBoneName = { "Root" };

		_bool		bUseCCT = { false };
		CCT_TYPE	eControllerType = { CCT_TYPE::CAPSULE };
		_float		fCCTRadius = { 0.8f };
		_float		fCCTHeight = { 1.2f };
		_float3		vCCTBoxHalf = { 0.5f, 0.5f, 0.5f };
		_float		fCCTStepOffset = { 0.3f };
		_float		fCCTSlopeLimitDeg = { 45.f };
		_float		fRunSpeed = { 2.5f };
		_float		fSprintSpeed = { 4.f };
		_float		fMoveAccel = { 10.f };
		_float		fMoveDecel = { 12.f };
		_float		fGroundStick = { 0.f };
	}CHARACTER_SETUP_DESC;

	typedef struct tagLoopEffectEntry
	{
		CEffect* pEffect = { nullptr };
		_bool bAnimScoped = { false };
		_uint64 iOwnerStateHash = { 0 };
	} LOOP_EFFECT_ENTRY;

protected :
	CAIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CAIObject(const CAIObject& Prototype);
	virtual ~CAIObject() = default;

public :
	/* Component */
	CBehaviorTree*			Get_BehaviorTree() const { return m_pBehaviorTree; }
	CBT_BlackBoard*			Get_BlackBoard() const { return m_pBlackBoard;}
	CStatComponent_AI*		Get_StatComponent() const { return m_pStatCom; }
	CMovement_AI*			Get_MovementCom() const { return m_pMovementCom; }

	const _wstring&			Get_MonsterName() const { return m_strMonsterName; }
	SPAWN_MONSTER_TYPE		Get_MonsterType() const { return m_eMonsterType; }

	/* Target Setting */
	void					Set_Target(CGameObject* pObject) { m_pTarget = pObject; }
	CGameObject*			Get_Target() const { return m_pTarget; }
	_bool					Get_TargetPos(_vector& vOut) const;
	_bool					Is_Detected() const { return m_bDetected; }
	virtual _bool			Is_Lockable() const { return true; }
	
	/* Move Control */
	void					Set_AISleep(_bool bAISleep ) { m_bAISleep = bAISleep; }
	void					Set_Freeze(_bool bFreeze);
	
	_bool					Is_Frozen() const;

	/* Chase Over */
	void					Cancel_Leash();
	_bool					Is_Returning() const { return m_bReturning; }
	
	

	 /* Stat*/
	 virtual void			Set_Groggy(_bool bGroggy);
	 _bool					Is_Groggy() const;
	 _bool					Is_Groggy_Full() const;

	 /* Damaged */
	 _bool					Is_HitActive() const;
	 _bool					Is_SuperArmor() const;
	 _bool					Is_Dead() const;
	 _bool					Is_GroundSweepActive() const;

	 /* Fatal */
	 _bool					Get_FatalPoint_Front(_vector& vOut) const;
	 _bool					Get_FatalPoint_Back(_vector& vOut) const;
	 virtual _bool			Can_FatalFront() const;
	 virtual _bool			Can_FatalBack() const;
	 void					Set_FatalOffset(const _float3& vFront, const _float3& vBack);
	 virtual void			Set_FatalMode(_bool bMode);
	 void                   Fatal_Start();
	 void                   Fatal_End();
	 void                   Fatal_Wait();	 

	 const string*			Get_GroundSparkFx() const;
	 virtual CHitBox_Helper* Get_HitBoxes() { return nullptr; }
	 /* Grab */
	 virtual _bool			Get_GrabSocketWorld(_matrix& OutSocketWorld) { return false; }

	 _bool					Is_RecentlyHit() const { return m_fHudHitTimer > 0.f; }
	 
public :
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;

	/* Poolable */
public:
	virtual _uint			Release() override;
	virtual void            Activate(void* pArg = nullptr) override;
	virtual void            InActivate() override;
	virtual void			Set_InActive() override;
	void                    Request_Despawn();

public:
	void					Set_Dissolve(_float fAmount, const _float4& vColor);

protected :
	void					Turn_On_AI();
	void					Turn_Off_AI();
	virtual void			Spawn();
	virtual void			Despawn();
	_bool					Is_MyBossStart(Engine::Event_Desc* pData) const;
	virtual void			Apply_SpawnDesc(void* pArg);

public :
	/* Reference */
	virtual void			Begin_Battle() {}
	virtual void			On_CheckDead() {}
	virtual void			On_SetDead();
	virtual void			On_Freeze(_bool bFreeze) {}	
	virtual void			On_Groggy(_bool bGroggy) {}
	virtual void			Revive() {};
	virtual void			On_SuperArmor(_bool bOn) {}
	virtual void			On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash);
	/* Animator */
	virtual void			Play_Animation(const _wstring& strKey);
	string					Resolve_NotifyKey(const ANIM_NOTIFY* pNotify, const string& strPrefix) const;
	void					Handle_SoundNotify(const ANIM_NOTIFY* pNotify);
	void					Handle_FootStepNotify(const ANIM_NOTIFY* pNotify);
	void					Handle_CamShakeNotify(const ANIM_NOTIFY* pNotify);
	virtual void			Action_Notify(_uint64 iHashKey) {}
	virtual _bool			Handle_CustomNotify(const ANIM_NOTIFY* pNotify);
	virtual void			Effect_Notify(_uint64 iHashKey) {}
	virtual void			Sound_Notify(string& payLoad);
	virtual _bool			Is_AnimationFinished() const;	

	void					Drive_LoopSounds();
	void					Drive_CamShakeWindows();
	void					Stop_AllCamShakes();
	void					Stop_AllLoopSounds();
	

#pragma region Animator Compact
	void					Set_AnimBool(const _wstring& strKey, _bool bValue);
	void					Set_AnimInt(const _wstring& strKey, _int iValue);
	void					Set_AnimFloat(const _wstring& strKey, _float fValue);
	void					Set_AnimTrigger(const _wstring& strKey);
	void					Request_AnimAction(const _wstring& strAction);
	void					Request_AnimState(const _wstring& strState);
	void					Request_AnimSequenceEnd();
	_bool					Was_AnimActionAccepted() const;
	_bool					Was_AnimStateChanged() const;
	_bool					Is_AnimInState(const _wstring& strState) const;
	_bool					Can_AnimMove() const;
	_bool					Can_AnimRotate() const;
#pragma endregion

	/* Damage */
	virtual _bool			Take_Damage(const HitInfo& hitInfo) override;
	virtual void			On_Parried() {}

	/* Shield */
	virtual void			Apply_ShieldBlock(_float fDamage) {}
	virtual void			On_ShieldBroken() {}

	/* Root Motion */
	void					Apply_RootMotion(const _float3& vTranslation, const _float4& vRotation);

	/* Attack*/
	_bool					Hit_SweepSphere(_float fRadius, _fvector vPrev, _fvector vCur, 
											 HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER);
	_bool					Hit_SweepCapsule(_float fRadius, _float fHalfHeight,_fvector vPrev, _fvector vCur, _fvector vQuat,
											 HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER);
	void					Hit_OverlapSphere(_float fRadius, _fvector vCenter,
											 HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER);
	_bool					Make_BoneCapsule(_int iBoneTop, _int iBoneBot, _vector& OutCenter, _vector& OutQuat);

	/* Leash */
	_bool					Update_Leash(_float fTimeDelta, _float fDistToTarget);

	_bool Has_LineOfSight(_fvector vFrom, _fvector vTo,
		FILTER_GROUP eBlockers = static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT)),
		_float fRadius = 0.1f) const;

	_bool Can_See(class CGameObject* pTarget, _float fEyeHeight = 1.2f,
		FILTER_GROUP eBlockers = static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT)),
		_float fRadius = 0.1f) const;

public:
	/* Behavior Tree Static Method */
	static class			CBT_FunctionRegistry* Registry();
	static void				Register_ProjectFunctions(Engine::CBT_FunctionRegistry* pReg);
	static void				Release_Registry();

public:
//추가
	_bool					Can_GrappleGrab() const
	{
		return m_bCanGrappleGrab;
	}
	void					Begin_GrappleGrab();
	void					On_GrapplePullStart();
	void					On_GrapplePulled(_fvector vTargetPos);
	void					End_GrappleGrab();

	_float					Get_CCTRadius() const;
#ifdef _DEBUG
public:
	enum class DBG_MOVE : _int { IDLE, CHASE, APPROACH, STRAFE, COMBAT, LEASH, GROGGY, HIT, FATAL, SPAWN, FROZEN, DEAD };

	HRESULT					Set_CCTRadius(_float fRadius);
	HRESULT					Set_CCTHeight(_float fHeight);
	HRESULT					Set_DebugRange(_float fRadius);
public:
	virtual _int			Get_DebugMoveState() const;	
	_float					Get_DebugReachRadius() const;
private:
	void					Rebuild_DebugRange();
	void					Debug_UpdateMoveState();
	_float					m_fCCTRadius = { 1.8f };
	_float					m_fCCTHeight = { 2.f };

	class CDebugCapsule*	m_pDebugRange = { nullptr };
	_float					m_fDbgRange = { 3.f };
	_float					m_fLastDbgRange = { -1.f };
	_float4					m_vDbgColor = { 0.f, 1.f, 0.f, 1.f };
	_int					m_iLastDbgMoveState = { -1 };
	_uint64					m_iLastDbgAnimHash = { 0 };
#endif

#ifdef _DEBUG
public:
	Engine::CNavigation* Get_DebugNavigation() const { return m_pNavigation; }
	const vector<_float3>& Get_DebugNaviPath() const;
	_uint Get_DebugNaviPathIndex() const;
#endif


protected:	
	/* Ready Component */
	HRESULT					Ready_Character(const CHARACTER_SETUP_DESC& Desc);
	HRESULT					Ready_Components();
	HRESULT					Bind_Navigation(class CNavigation* pNavigation);
	HRESULT					Ready_BehaviorTree();
	HRESULT					Load_BehaviorTree(const _wstring& strPath);

	virtual void			Build_BehaviorTree();
	virtual void			Update_BlackBoard(_float fTimeDelta);

	void					Update_Activation();
	void					Compute_HitDir(_fvector vHitPosition);

	virtual _float			Calc_RootMotionScale() const;
	void					Apply_Movment(_float fTimeDelta);
	void					Cancel_All_Attacks();
	void					Begin_Pattern(_int iPatternID, _float fAttackLength);

#pragma region Damaged Compact
	virtual _bool			Should_Ignore_Damage(const HitInfo& hitInfo) { return false; }
	virtual _bool			Handles_FatalEnd() const { return true; }
	virtual void			On_Fatal_Begin() {}
	virtual _bool			Blocks_Damage_While_Groggy() const { return false; }
	virtual void			On_Hit_Pre(const HitInfo& hitInfo) {}
	virtual void			On_Hit_Post(const HitInfo& hitInfo) {}
	virtual _bool			Uses_Full_Hit_Reaction() const { return true; }
	virtual void			On_Groggy_Triggered() {}
	virtual void			Record_HitType(const HitInfo& hitInfo);
	virtual void			On_Perceive_Attacker(_bool bPreFatal);
	virtual void			Apply_ChargeHit(_float fDamage, _float fGroggy);
	_bool					Consume_HitRefresh() { const _bool bWas = m_bHitRefreshPending; m_bHitRefreshPending = false; return bWas; }
#pragma endregion


	/* Spawn */
	_vector					Get_SpawnPos() const { return XMLoadFloat4(&m_vSpawnPos); }
	_bool					Is_AISleep() const { return m_bAISleep; }
	void					Capture_SpawnPos();
	
	/* Running Animator ( Tick -> Notify -> RootMotion )  In Update */
	void					Drive_Animator(_float fTimeDelta);
	virtual _float			Get_RootMotionScale() const { return 1.f; }
	void					Drive_SuperArmor();

#pragma region Effect Compact
	virtual void			Handle_EffectNotify(const ANIM_NOTIFY* pNotify);
	CEffect*				Start_LoopEffect_World(_uint64 iKey, const string& strFxName, const _float3& vWorldPos, _bool bAnimScoped = false);
	CEffect*				Start_LoopEffect(_uint64 iKey, const string& strFxName, const string& strBoneName, const _float3& vOffset = {}, _bool bAnimScoped = false);
	void					Stop_LoopEffect(_uint64 iKey);
	void					Stop_LoopImmediate(_uint64 iKey);
	void					Stop_AllLoopEffects();

	void					Stop_AnimScopedLoopEffects(_uint64 iKeepStateHash);
	_uint64					Get_CurAnimStateHash() const;
#pragma endregion

	void					Begin_GroggyWindow(_float fSeconds);
	void					Tick_GroggyWindow(_float fTimeDelta);
	virtual void			On_GroggyTimeout() { Set_Groggy(false); }

	_bool					Can_Perceive_Target() const;
	void					Update_Aggro_WithLOS(_float fDist, _float fTimeDelta);

protected:
	CBehaviorTree*		m_pBehaviorTree			= { nullptr };
	CBT_BlackBoard*		m_pBlackBoard			= { nullptr };
	CGameObject*		m_pTarget				= { nullptr };
	CNavigation*		m_pNavigation			= { nullptr };
	CAnimator*			m_pAnimator				= { nullptr };
	CCT*				m_pCCT					= { nullptr };
	CStatComponent_AI*	m_pStatCom				= { nullptr };
	CMovement_AI*		m_pMovementCom			= { nullptr };
	_wstring			m_strBTPath				= {};
	vector<_bool>		m_MeshVisibles			= {};	

	_bool				m_bReturning			= { false };
	_bool				m_bReturningPrev		= { false };
	_bool				m_bDetected				= { false };
	_bool				m_bAISleep				= { false };
	_bool				m_bSuperArmorPrev		= { false };
	_bool				m_bFatalReady			= { false };
	_bool				m_bPoolReady			= { false };
	_bool				m_bSpawnCaptured		= { false };
	_bool				m_bFrozen				= { false };
	_bool				m_bSpawned				= { false };
	_bool				m_bAggro				= { false };
	_bool				m_bApproaching			= { false };
	_uint64				m_iCurAnimStateHash		= { 0 };

	unordered_map<_uint64, NOTIFY_KEY>				m_NotifyKeys;
	unordered_map<_uint64, HIT_TYPE>				m_HitTypeKeys;
	unordered_map<_uint64, LOOP_EFFECT_ENTRY>		m_LoopEffects;
	unordered_map<_uint64, _uint>					m_LoopSounds;
	unordered_set<_uint64>							m_CamShakes;

protected :
	SPAWN_MONSTER_TYPE	m_eMonsterType			= { SPAWN_MONSTER_TYPE::END};
	_wstring			m_strMonsterName		= {};
	static constexpr _float	ROOTMOTION_OFFSET_TO_METER = 0.01f;
	_float				m_fDetectRange			= { 10.f };
	_float				m_fAttackRange			= { 2.f };
	_float				m_fLeashRange			= { 20.f };
	_float				m_fFrontCos				= { 0.5f };

	_float4				m_vSpawnPos				= {};
	_float3				m_vFatalOffsetFront		= { 0.f, 0.f, 1.5f };
	_float3				m_vFatalOffsetBack		= { 0.f, 0.f, -1.5f };
	_float				m_fLaunchSpeed			= { 10.f };
	_float				m_fLaunchUp				= { 15.f };

	_int				m_iNaviCurCellIndex		= { -1 };

	_int				m_iPendingPatternID		= { -1 };
	_float				m_fPendingAttackLength	= { 0.f };

	_float				m_fAttackRangeTol		= { 0.5f };
	_int				m_iHitDir				= { 0 };

	
	_float				m_fLostTimer			= { 0.f };
	_float				m_fLoseDelay			= { 2.f };
	_float				m_fHitTimer				= { 0.f };
	_float				m_fHitMaxTime			= { 1.5f };
	_bool				m_bHitRefreshPending	= { false };
	HIT_TYPE			m_eCurHitType			= { HIT_TYPE::Normal };

	_float				m_fReengageRange		= { 8.f };
	_float				m_fReengageDelay		= { 0.6f };
	_float				m_fReengageTimer		= { 0.f };

	_float				m_fGroggyWindow			= { 0.f };
	_bool				m_bGroggyWindowActive	= { false };
	_float				m_fGroggyWindowFallback = { 5.f };

	_bool				m_bCanGrappleGrab		= { true };
	// 플레이어가 갱신된 보스의 본 위치를 가져오지 못하는 문제를 해결하기 위해
	// 외부(Get_GrabSocketWorld)에서 본 갱신했으면 한 프레임만 건너뛰도록 하는 플래그
	_bool				m_bSkipBoneUpdateOnce = { false };

	_float				m_fPerceiveFOVDegree	= { 150.f };
	_float				m_fPerceiveEyeHeight	= { 1.2f };
	_float				m_fPerceiveLOSRadius	= { 0.1f };
	FILTER_GROUP		m_ePerceiveBlockers		= static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));
	_float				m_fHudHitTimer			= { 0.f };
	_float				m_fHudHitShowTime		= { 10.f };

	_float				m_fDissolveAmount		= { 0.f };
	_float4				m_vDissolveColor		= { 0.f, 0.f, 0.f, 0.f };
	_float				m_fAggroKeepScale		= { 1.f };
	_float				m_fGroundFxExtend = { 1.5f };

	string				m_strFootStepSurface = { "Stone" };
	_float				m_fCamShakeMaxDist = { 15.f };
	string				m_strGrappleStartCue = { "SE_NPC_BRabbit_NormalMale_MT_Arm_GrappleGun_Start_Cue" };
	string				m_strGrapplePullCue = { "SE_NPC_BRabbit_NormalMale_MT_Arm_GrappleGun_Pull_Cue" };

#ifdef _DEBUG
protected:
	void						Debug_TraceAnim();
	_uint64						m_iDebugLastClipHash = {};

public:
	static void					Toggle_TraceAnim();
	static _bool				s_bTraceAnim;
#endif

public :
	virtual CGameObject* Clone(void* pArg) PURE;
	virtual void		 Free() override;
};

NS_END