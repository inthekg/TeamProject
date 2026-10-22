#pragma once
#include "AIObject.h"
#include "BT_Node.h"
#include "HitBox_Helper.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CWeapon_Monster;
class CShield_Raxasia;

class GAMECORE_DLL CBoss_Raxasia_P1 final : public CAIObject, public IListener
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CBoss_Raxasia_P1)
#endif // _DEBUG

public:
	enum class EFFECT_KEY { FURY, SYMBOL, END };
	enum class BRAIN { INTRO_WALK, INTRO_RUN, INTRO_JUMP, COMBAT };

	typedef struct tagRaxasiaP1Desc : public CAIObject::AIOBJECT_DESC
	{
		_float fShortAttackRange = { 5.f };
		_float fFarAttackRange = { 12.f };
		_float fAttackGap = { 0.5f };
		_float fFatalFront = { 2.f };
		_float fFatalBack = { 0.f };
	} MONSTER_DESC;

private:
	CBoss_Raxasia_P1(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoss_Raxasia_P1(const CBoss_Raxasia_P1& Prototype);
	virtual ~CBoss_Raxasia_P1() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				On_Groggy(_bool bGroggy) override;
	virtual void				On_SuperArmor(_bool bOn) override;
	virtual void                On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash) override;
	virtual void				Begin_Battle() override;
	virtual void				OnEvent(EVENT_TYPE eType, Event_Desc* pAdata) override;
	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual _bool				Can_FatalBack() const override;
	virtual void				Apply_ShieldBlock(_float fDamage) override;
	virtual void				On_ShieldBroken() override;
	virtual void				On_Parried() override;
	virtual void				Set_FatalMode(_bool bMode) override;
	virtual void                Handle_EffectNotify(const ANIM_NOTIFY* pNotify) override;


public:
	virtual _bool				Get_GrabSocketWorld(_matrix& OutSocketWorld) override;

	_vector						Compute_TrustAnim(_fvector vTargetPos);

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				Apply_ChargeHit(_float fDamage, _float fGroggy) override;
	virtual _bool				Uses_Full_Hit_Reaction() const override;

private:
	HRESULT						Ready_Weapon();
	HRESULT						Ready_Shield();
	HRESULT						Ready_HitBoxes();

	void						Mirror_ControllerParams();
	void						Update_CombatParams();
	void						Drive_Steering(_float fTimeDelta);
	void						Update_PatternSelect(_float fTimeDelta);
	_bool						Is_AnimIdle()const;
	_int						Select_WeightedPattern(_float fDist);

private:
	void						On_P1_Fallen();
	void						On_GroggyStart();
	void						Advance_SubPattern();
	_int						Pick_OppositeSide(_float fRight) const;
	void						Fire_BoltRange();
	void						Spawn_BoltMine();
	void						Fire_BoltSector();
	
	/* Effect 관련 함수 */
	void						Turn_On_Sword();
	void						Turn_Off_Sword();
	void						Turn_On_Normal_Trail();
	void						Turn_On_Elec_Trail();
	void						Turn_Off_Trail();

	void						Begin_Grab();
	void						End_Grab();

	void						Break_Shield();
	void						Begin_StaggerMotion();
	void						Update_StingCheck();
	void						Update_FrontCheck(_float fTimeDelta);

	_bool						Get_BladeGroundPos(_vector& OutPos);


private:
	CWeapon_Monster* m_pWeapon = { nullptr };
	CShield_Raxasia* m_pShield = { nullptr };
	CHitBox_Helper				m_HitBoxes;

private:
	unordered_map<EFFECT_KEY, _uint64> m_EffectKeys;
	const _float4x4*			m_pMatGrabSocket = { nullptr };
	const string				m_GRAB_BONE_NAME = "Raxasia_Grab_Socket";

	_bool						m_bDead = { false };
	_bool						m_bWeaponLive = { true };
	_bool						m_bWasIdle = { true };
	_bool						m_bChasing = { false };
	_bool						m_bTurning = { false };
	_bool						m_bAttackCommit = { false };
	_bool						m_bAttackStarted = { false };
	_bool						m_bStingHit = { false };
	_bool						m_bGrounded = { true };
	_bool						m_bBattleStarted = { false };
	_bool						m_bPerceived = { false };
	_bool						m_bShieldBreaking = { false };
	_float						m_fShieldBreakGuard = { 0.f };

	_int						m_iCurPatternID = { 1 };
	_int						m_iLastPatternID = { 0 };
	_int						m_iSubPatternID = { 0 };

	_float						m_fIdleTimer = { 0.f };
	_float						m_fDistToTarget = { 9999.f };
	_float						m_fShortAttackRange = { 5.f };
	_float						m_fFarAttackRange = { 12.f };
	_float						m_fAttackGap = { 0.5f };
	_float						m_fAimTurnBoost = { 10.f };
	_float						m_fChaseSwitchTimer = { 5.f };
	_float						m_fCommitTimer = { 0.f };
	_float						m_fVerticalVel = { 0.f };

	BRAIN						m_eBrain = { BRAIN::INTRO_WALK };
	_float						m_fIntroTimer = { 0.f };
	_float						m_fIntroWalkTime = { 1.f };
	_float						m_fJumpRange = { 8.f };

	_bool						m_bStrafing = { false };
	_bool						m_bStrafeNext = { false };
	_int						m_iStrafeDir = { 0 };
	_float						m_fStrafeTimer = { 0.f };
	_float						m_fStrafeDuration = { 1.2f };

	_float						m_fPendingNear = { 0.f };
	_float						m_fPendingFar = { 0.f };
	_int						m_iJumpPatternID = { 10 };

	static constexpr _int		RAXA_RECENT = 3;
	static constexpr _float		BLADE_GROUND_PROBE_UP = { 2.f };
	static constexpr _float		BLADE_GROUND_PROBE_DOWN = { 8.f };
	static constexpr _float		EMITTER_GROUND_LIFT = { 0.5f };
	_bool						m_bRetreating = { false };
	_float						m_fRepositionTimer = { 0.f };
	_float						m_fRepositionMax = { 2.f };
	_int						m_iRecent[RAXA_RECENT] = { 0, 0, 0 };
	_int						m_iRecentCount = { 0 };

	_float						m_fFrontCheckRadius = { 1.0f };
	_float						m_fFrontCheckForward = { 1.1f };
	_float						m_fFrontCheckHeight = { 1.0f };
	_float3						m_vFrontPrevCenter = {};
	_bool						m_bFrontPrevValid = { false };

	_float						m_fSideDeadZone = { 0.1f };
	_float						m_fWalkAnimSpeed = { 1.f };
	_float						m_fRunAnimSpeed = { 3.f };
	_float						m_fFarFalloff = { 0.5f };

	_float						m_fStingBackDist = { 2.5f };
	_float						m_fStingFrontDist = { 7.5f };
	_int						m_iShowcaseIdx = { 0 };

	CEffect*					m_pSwordFx = { nullptr };
	CEffect*					m_pFuryFx = { nullptr };
public:
	static CBoss_Raxasia_P1* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;

#ifdef _DEBUG
public:
	HRESULT						Set_DebugHitIndex(_int iIndex);
	HRESULT						Set_DebugHitRadius(_float fRadius);
	HRESULT						Set_DebugHitHalf(_float fHalf);

	void						Debug_BrainLog(_float fTimeDelta);
	void						Debug_GroggyLog();
	void						Debug_MeasureAttack(_float fTimeDelta);
	void						Debug_ResetToOrigin();
	void						Debug_FrontLog(_float fDist, _float fDot, _float fSeg, _bool bHit, CGameObject* pOther);
	void						Debug_ArmReject(const _tchar* pWhere, _int iDelayFrames, _int iDir);
	void						Debug_RejectLog();

private:
	static constexpr _int		DBG_PATTERN_COUNT = 10;

	_int						m_iDbgHitIdx = { 0 };
	_float						m_fDbgHitRadius = { 0.f };
	_float						m_fDbgHitHalf = { 0.f };
	_float						m_fDbgLogTimer = { 0.f };

	const _float4x4*			m_pDbgFootL = { nullptr };
	const _float4x4*			m_pDbgFootR = { nullptr };
	_bool						m_bDbgBonesReady = { false };
	_bool						m_bDbgMeasuring = { false };
	_bool						m_bDbgStarted = { false };
	_bool						m_bDbgFirstHitDone = { false };
	_int						m_iDbgAttackID = { 0 };
	_float3						m_vDbgStartPos = {};
	_float						m_fDbgLunge = { 0.f };
	_float						m_fDbgLungeAtHit = { -1.f };
	_float						m_fDbgBladeNear = { -1.f };
	_float						m_fDbgBladeFar = { 0.f };
	_float						m_fDbgFootNear = { -1.f };
	_float						m_fDbgFootFar = { 0.f };
	_float						m_fDbgBladeAngle = { 999.f };
	_float						m_fDbgAimErr = { 999.f };
	_float						m_fDbgScaleMax = { 1.f };
	_float						m_fDbgStartDist = { -1.f };
	_float						m_fDbgTimer = { 0.f };


	const _tchar*				m_pDbgReqWhere = { nullptr };
	_int						m_iDbgReqTick = { 0 };
	_int						m_iDbgReqPattern = { 0 };
	_int						m_iDbgReqSub = { 0 };
	_int						m_iDbgReqDir = { 0 };
	_float						m_fDbgReqDist = { -1.f };
#endif // _DEBUG

};

NS_END
