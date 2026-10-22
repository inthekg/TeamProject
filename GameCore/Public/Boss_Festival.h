#pragma once
#include "AIObject.h"
#include "BT_Node.h"
#include "HitBox_Helper.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CWeapon_Monster;

class GAMECORE_DLL CBoss_Festival final : public CAIObject, public IListener
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CBoss_Festival)
#endif // _DEBUG

public:
	enum class EFFECT_KEY { FURY, FIRE, SMOKE1, SMOKE2, SYMBOL, SLIDE, END };

	typedef struct tagFestivalLeaderDesc : public CAIObject::AIOBJECT_DESC
	{
		_float	fShortAttackRange = { 5.f };
		_float	fFarAttackRange = { 12.f };
		_float	fPhase2Ratio = { 0.5f };
		_float	fAttackGap = { 0.5f };
		_float	fFatalFront = { 1.5f };
		_float	fFatalBack = { 0.f };
		string	strWeaponTrailFx = { "Effect_Monster_Trail" };
		_bool	bWeaponTrailUse = { false };
	}MONSTER_DESC;

private:
	CBoss_Festival(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoss_Festival(const CBoss_Festival& Prototype);
	virtual ~CBoss_Festival() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual void				On_Freeze(_bool bFreeze) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual _bool				Can_FatalBack() const override;

	virtual CHitBox_Helper* Get_HitBoxes() override { return &m_HitBoxes; }
	virtual _float				Get_RootMotionScale() const override { return Calc_RootMotionScale(); }

	virtual void				Spawn() override;
	virtual void				Despawn() override;

	virtual void				Begin_Battle() override;
	virtual void				OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;

	virtual void				On_SuperArmor(_bool bOn) override;
	virtual void				On_Groggy(_bool bGroggy) override;

	void						Move_Flat(_fvector vWorldDir, _float fSpeed, _float fTimeDelta);
#ifdef _DEBUG
public:
	HRESULT						Set_DbgHitIdx(_int iIndex);
	HRESULT						Set_DbgHitRadius(_float fRadius);
	HRESULT						Set_DbgHitHalf(_float fHalf);

private:
	void						Debug_MeasureAttack(_float fTimeDelta);
	void						Debug_ResetToOrigin();
#endif

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				Record_HitType(const HitInfo& hitInfo) override;
	virtual _bool				Handles_FatalEnd() const override;
	virtual void				On_Fatal_Begin() override;
	virtual _bool				Should_Ignore_Damage(const HitInfo& hitInfo) override;
	virtual void				On_Perceive_Attacker(_bool bPreFatal) override;
	virtual _bool				Uses_Full_Hit_Reaction() const override;
	virtual void				Apply_ChargeHit(_float fDamage, _float fGroggy) override;

	void						Set_BodyMeshEmissive(const _wstring& strKey, _float fAmount);
	void						Update_PhaseTransition();
	void						Update_PhaseRequest();
	void						Update_CombatParams();
	void						Mirror_ControllerParams();
	void						Drive_Facing(_fvector vTargetPos, _float fTimeDelta);
	void						Drive_Steering(_float fTimeDelta);
	void						Update_PatternSelect(_float fTimeDelta);
	_bool						Is_AnimIdle() const;
	_int						Select_WeightedPattern(_int iPhase, _float fDist);
	HRESULT						Ready_BodyMeshIndex();
	void						Set_BodyMeshVisible(const _wstring& strKey, _bool bVisible);
	vector<_int>				Find_MeshIndices(const _wstring& strMaterialKey) const;

	HRESULT						Ready_Weapon();
	HRESULT						Ready_HitBoxes();




private:
	CWeapon_Monster* m_pWeapon = { nullptr };
	CHitBox_Helper				m_HitBoxes;
	_bool						m_bWeaponLive = { false };

	string						m_strWeaponTrailFx = { "Effect_Monster_Trail" };
	_bool						m_bWeaponTrailUse = { false };

#ifdef _DEBUG
	_int						m_iDbgHitIdx = { 0 };
	_float						m_fDbgHitRadius = { 0.f };
	_float						m_fDbgHitHalf = { 0.f };

	_bool						m_bDbgMeasuring = { false };
	_bool						m_bDbgStarted = { false };
	_bool						m_bDbgFirstHitDone = { false };
	_int						m_iDbgAttackID = { 0 };
	_int						m_iDbgPhase = { 1 };
	_int						m_iDbgFireDelay = { 0 };
	_float						m_fDbgTimer = { 0.f };
	_float						m_fDbgStartDist = { -1.f };
	_float						m_fDbgScaleMax = { 1.f };
	_float3						m_vDbgStartPos = {};
	_float3						m_vDbgStartLook = {};
	_float						m_fDbgLunge = { -9999.f };
	_float						m_fDbgLungeAtHit = { -9999.f };
	_float						m_fDbgNear[5] = {};
	_float						m_fDbgFar[5] = {};
	_bool						m_bDbgSeen[5] = {};
	_float						m_fDbgReach = { -9999.f };
#endif


private:
	unordered_map<_wstring, vector<_uint>>  m_MeshIndexMap;
	unordered_map< EFFECT_KEY, _uint64>		m_EffectKeys;
	vector<_float>				m_MeshEmissive = {};

	_bool						m_bPhase2 = { false };
	_bool						m_bHeadDetached = { false };
	_bool						m_bWasIdle = { true };
	_bool						m_bDead = { false };
	_bool						m_bBattleStarted = { false };
	_int						m_iCurPatternID = { 1 };
	_int						m_iLastPatternID = { 0 };
	_bool						m_bChasing = { false };
	_bool						m_bTurning = { false };

	_float						m_fIdleTimer = { 0.f };
	_float						m_fDistToTarget = { 9999.f };
	_float						m_fShortAttackRange = { 5.f };
	_float						m_fFarAttackRange = { 12.f };
	_float						m_fPhase2Ratio = { 0.5f };
	_float						m_fAttackGap = { 0.5f };
	_float						m_fAimTurnBoost = { 10.f };
	_float						m_fTurnRotBoost = { 0.7f };

	_int						m_iTurnDir = { 4 };
	_float						m_fTurnStartDeg = { 15.f };
	_float						m_fTurnStopDeg = { 5.f };

	_bool						m_bMoving = { false };
	_bool						m_bMustAttack = { false };
	_int						m_iMoveDir = { 0 };
	_float						m_fMoveTimer = { 0.f };
	_float						m_fMoveDuration = { 1.2f };
	_float						m_fMoveChance = { 0.35f };

	_float						m_fVerticalVel = { 0.f };
	_bool						m_bPerceived = { false };
	_bool						m_bGrounded = { true };
	_float						m_fChaseSwitchTimer = { 10.f };
	_bool						m_bAttackCommit = { false };
	_float						m_fCommitTimer = { 0.f };

	_int						m_iShowcaseIdx = { 0 };
	_bool						m_bShowcaseWasP2 = { false };
	_float						m_fAttackRangeTol = { 0.7f };
	_float						m_fApproachTimer = { 0.f };
	_float						m_fApproachTimeout = { 4.f };
	_float						m_fChaseSpeed = { 3.5f };
	_float						m_fStrafeSpeed = { 2.2f };

public:
	static CBoss_Festival* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END
