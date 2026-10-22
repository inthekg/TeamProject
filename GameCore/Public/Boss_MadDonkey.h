#pragma once
#include "AIObject.h"
#include "BT_Node.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CDissolve;
NS_END

NS_BEGIN(GameCore)

class CWeapon_Monster;

class GAMECORE_DLL CBoss_MadDonkey final : public CAIObject, public IListener
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CBoss_MadDonkey)
#endif

public:
	typedef struct tagMadDonkeyDesc : public CAIObject::AIOBJECT_DESC
	{
		_wstring	strModelProtoTag = { TEXT("Prototype_Component_Model_MadDonKey") };
		_wstring	strAnimFolderTag = { TEXT("MadDonKey") };
		_wstring	strWeaponProtoTag = { TEXT("Prototype_Component_Model_Weapon_GreatSaw") };
		string		strWeaponSocketBone = { "BN_Weapon_R" };
		string		strWeaponTrailTopBone = { "FX_WP_Trail_R_1_1" };
		string		strWeaponTrailBotBone = { "FX_WP_Trail_R_1_3" };
		string		strWeaponTrailFx = { "Effect_Monster_Trail" };
		const char* pRootBoneName = { "Base" };

		_float		fShortAttackRange = { 4.f };
		_float		fFarAttackRange = { 8.f };
		_float		fRunChaseRange = { 6.f };
		_float		fAttackGap = { 0.5f };
		_float		fMoveDuration = { 1.2f };
		_float		fFatalFront = { 2.f };
		_float		fFatalBack = { 2.f };

		_float		fRunSpeed = { 2.2f };
		_float		fSprintSpeed = { 3.5f };

		_float		fSweepRadius = { 0.5f };
		_float		fSweepHalfHeight = { 1.2f };
		_float		fWeaponDamage = { 40.f };

		_uint		iSpawnID = { UINT_MAX };
		_int		iSpawnNumber = { 1 };
		_uint		iTargetDoorSpawnID = { UINT_MAX };
		_float3		vExitGoalPos = {};

		_float		fCCTRadius = { 0.6f };
		_float		fCCTHeight = { 1.6f };
	} MONSTER_DESC;

private:
	CBoss_MadDonkey(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoss_MadDonkey(const CBoss_MadDonkey& Prototype);
	virtual ~CBoss_MadDonkey() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				Apply_SpawnDesc(void* pArg) override;
	virtual void				Begin_Battle() override;
	virtual void				OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void				OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
	virtual void				On_SuperArmor(_bool bOn) override;
	virtual void				On_Freeze(_bool bFreeze) override;
	virtual _bool				Can_FatalFront() const override;
	virtual _bool				Can_FatalBack() const override;
	virtual _bool				Take_Damage(const HitInfo& hitInfo) override;

	void						Begin_Dialogue();
	void						Face_Target_Immediate();
	void						End_Dialogue();

	_bool						Is_Gatekeeper() const { return UINT_MAX != m_iTargetDoorSpawnID; }
	_bool						Has_GatePass() const;

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_CheckDead() override;
	virtual void				On_Groggy(_bool bGroggy) override;
	virtual _bool				Uses_Full_Hit_Reaction() const override { return false; }
	virtual void				Record_HitType(const HitInfo& hitInfo) override;
	virtual void				On_Groggy_Triggered() override;
	virtual void				On_Perceive_Attacker(_bool bPreFatal) override;

	HRESULT						Ready_NotifyKeys();
	HRESULT						Ready_AttackData();
	HRESULT						Ready_Weapon(const MONSTER_DESC& Desc);
	HRESULT						Ready_Dissolve();

	void						Update_CombatParams(_float fTimeDelta);
	void						Mirror_ControllerParams();
	void						Update_PatternSelect(_float fTimeDelta);
	void						Update_ComboLink();
	void						Drive_Steering(_float fTimeDelta);
	void						Drive_Facing(_fvector vTargetPos, _float fTimeDelta);

	_int						Pick_Action(_float fDist, _bool bAllowMove);
	void						Fire_Attack();
	void						Fire_ComboLink(_int iPatternID);
	void						Fire_Knockback();
	void						Fire_FinishMove();

	void						Cancel_CombatState();

	_bool						Is_AnimIdle() const;
	_bool						Is_InAttackState() const;

	void						Broadcast_BossStart();

	void						Open_TargetDoor();
	void						Begin_Exit();
	void						Drive_Exit(_float fTimeDelta);
	void						Begin_Dissolve(_float fStartDelay, _float fDuration);
	void						Update_Dissolve(_float fTimeDelta);
	void						Spawn_DropItem();

#ifdef _DEBUG
private:
	void						Debug_Input(_float fTimeDelta);
	void						Debug_MeasureAttack(_float fTimeDelta);
	void						Debug_ResetToOrigin();
#endif

private:
	CWeapon_Monster* m_pWeapon = { nullptr };
	unordered_map<_uint64, ATTACK_DATA>	m_AttackData;

	_bool						m_bDead = { false };
	_bool						m_bBattleStarted = { false };
	_bool						m_bTalking = { false };
	_bool						m_bKnockbackPending = { false };
	_uint						m_iSpawnID = { UINT_MAX };
	_int						m_iSpawnNumber = { 1 };

	string						m_strWeaponSocketBone = { "BN_Weapon_R" };
	_float						m_fSweepRadius = { 0.5f };
	_float						m_fSweepHalfHeight = { 1.2f };

	_int						m_iCurPatternID = { 0 };
	_int						m_iLastPatternID = { 0 };
	_float						m_fIdleTimer = { 0.f };
	_float						m_fDistToTarget = { 9999.f };
	_float						m_fShortAttackRange = { 4.f };
	_float						m_fFarAttackRange = { 8.f };
	_float						m_fRunChaseRange = { 6.f };
	_float						m_fAttackGap = { 0.5f };

	_bool						m_bMoving = { false };
	_bool						m_bDashMove = { false };
	_bool						m_bMustAttack = { false };
	_int						m_iMoveDir = { 0 };
	_float						m_fMoveTimer = { 0.f };
	_float						m_fMoveDuration = { 1.2f };

	_bool						m_bChasing = { true };
	_float						m_fChaseSwitchTimer = { 0.f };
	_float						m_fChaseMargin = { 0.7f };
	_float						m_fChaseDwell = { 0.35f };

	_bool						m_bTurning = { false };
	_int						m_iTurnDir = { 4 };
	_float						m_fTurnStartDeg = { 15.f };
	_float						m_fTurnStopDeg = { 5.f };

	_bool						m_bLinkFired = { false };
	_int						m_iLinkOwnerID = { -1 };

	_float						m_fMoveWeight = { 1.4f };
	_float						m_fRepeatPenalty = { 0.2f };
	_float						m_fOffBandScale = { 0.25f };
	_float						m_fDashBackRange = { 2.5f };

	_uint						m_iTargetDoorSpawnID = { UINT_MAX };
	_float3						m_vExitGoal = {};
	_bool						m_bExiting = { false };
	_bool						m_bExitPending = { false };
	_bool						m_bGateAnswered = { false };
	_bool						m_bGateChecked = { false };
	_float						m_fExitTimer = { 0.f };

	CDissolve* m_pDissolveCom = { nullptr };
	_bool						m_bDissolving = { false };
	_bool						m_bDropPending = { false };
	_bool						m_bDeadHandled = { false };

#ifdef _DEBUG
	_bool						m_bDbgMeasuring = { false };
	_bool						m_bDbgStarted = { false };
	_int						m_iDbgAttackID = { 0 };
	_int						m_iDbgFireDelay = { 0 };
	_float						m_fDbgTimer = { 0.f };
	_float						m_fDbgStartDist = { -1.f };
	_float						m_fDbgScaleMax = { 1.f };
	_float3						m_vDbgStartPos = {};
	_float3						m_vDbgStartLook = {};
	_float						m_fDbgLunge = { -9999.f };
	_float						m_fDbgLungeAtHit = { -9999.f };
	_float						m_fDbgReach = { -9999.f };
#endif

public:
	static CBoss_MadDonkey* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
