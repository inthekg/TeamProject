#pragma once
#include "Monster_Melee.h"
#include "BT_Node.h"

NS_BEGIN(Engine)
class CEffect;
NS_END

NS_BEGIN(GameCore)

class CMonster_Attach;

class GAMECORE_DLL CMonster_Priest final : public CMonster_Melee
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Priest)
#endif

public:
	typedef struct tagPriestDesc : public CMonster_Melee::MELEE_DESC
	{
		_wstring	strCanisterProtoTag = { TEXT("Prototype_Component_Model_Canister") };
		_wstring	strCapProtoTag = { TEXT("Prototype_Component_Model_MagicianHat") };
		string		strCanisterBone = { "BN_Canister" };
		string		strCapBone = { "Bone001_Cap" };

		_float		fFarAttackRange = { 8.f };
		_float		fRunChaseRange = { 6.f };
		_float		fFatalFront = { 1.5f };
		_float		fFatalBack = { 1.5f };
		_float		fRunSpeed = { 2.2f };
		_float		fSprintSpeed = { 2.2f };
	} MONSTER_DESC;

private:
	CMonster_Priest(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Priest(const CMonster_Priest& Prototype);
	virtual ~CMonster_Priest() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

	virtual void				Spawn() override;
	virtual void				Despawn() override;

	virtual void				On_CheckDead() override;
	virtual void				On_Parried() override;
	virtual void				Set_FatalMode(_bool bMode) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_Hit_Post(const HitInfo& hitInfo) override;
	virtual _bool				Blocks_Damage_While_Groggy() const override;

	virtual HRESULT				Ready_NotifyKeys() override;
	virtual HRESULT				Ready_AttackData() override;
	virtual _float				Get_AttackRange() const override;
	virtual _bool				Wants_RunChase(_float fDist) const override;
	virtual const _wstring& Get_KnockbackAction(_int iHitInfo) const override;

	HRESULT						Ready_Attachments(const MONSTER_DESC& Desc);
	void						Update_PatternSelect(_float fTimeDelta);
	_int						Pick_Action_Priest(_float fDist, _bool bAllowMove);
	void						Update_BallAim(_float fTimeDelta);
	void						Fire_ElectricBall();

	void						Update_ChaseHysteresis(_float fTimeDelta);
	void						Drive_Steering_Priest(_float fTimeDelta);
	void						Drive_Facing_Priest(_fvector vTargetPos, _float fTimeDelta);

	void						Update_ComboLink();
	void						Fire_ComboLink(_int iPatternID);

#ifdef _DEBUG
private:
	void						Debug_MeasureAttack(_float fTimeDelta);
	void						Debug_ResetToOrigin();
#endif

private:
	CMonster_Attach* m_pCanister = { nullptr };
	CMonster_Attach* m_pCap = { nullptr };

	_float						m_fFarAttackRange = { 8.f };
	_float						m_fRunChaseRange = { 6.f };

	_bool						m_bChasing = { true };
	_float						m_fChaseSwitchTimer = { 0.f };
	_float						m_fChaseMargin = { 0.7f };
	_float						m_fChaseDwell = { 0.35f };

	_bool						m_bTurning = { false };
	_int						m_iTurnDir = { 4 };
	_float						m_fTurnStartDeg = { 15.f };
	_float						m_fTurnStopDeg = { 5.f };

	_float						m_fMoveWeight = { 0.6f };
	_float						m_fRepeatPenalty = { 0.2f };
	_float						m_fOffBandScale = { 0.25f };

	_bool						m_bParryReact = { false };

	_bool						m_bBallAiming = { false };
	_bool						m_bBallFired = { false };
	_float						m_fBallPrevRotSpeed = { 0.f };
	_float						m_fBallTrackSpeed = { 360.f };

	string						m_strBallBone = { "BN_Handle" };
	_float						m_fBallForward = { 1.2f };
	_float						m_fBallHeight = { 1.3f };
	_float						m_fBallAimHeight = { 1.0f };
	_float						m_fBallScale = { 2.f };
	_float						m_fBallSpeed = { 80.f };
	_float						m_fBallRange = { 40.f };
	_float						m_fBallRadius = { 0.4f };
	_float						m_fBallDamage = { 800.f };

	_bool						m_bLinkFired = { false };
	_int						m_iLinkOwnerID = { -1 };
	_float						m_fLinkFrontRange = { 3.5f };
	_float						m_fLinkFrontDeg = { 45.f };

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
	CEffect*					m_pWeaponFx = { nullptr };

public:
	static CMonster_Priest* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
