#pragma once
#include "AIObject.h"
#include "BT_Node.h"

NS_BEGIN(GameCore)

class CWeapon_Monster;

class GAMECORE_DLL CMonster_Melee abstract : public CAIObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Melee)
#endif // _DEBUG

public:
	enum class EFFECT_KEY { FURY, TRAIL, END };

	typedef struct tagMeleeDesc : public CAIObject::AIOBJECT_DESC
	{
		_wstring	strModelProtoTag = {};
		_wstring	strAnimFolderTag = {};
		_wstring	strWeaponProtoTag = {};
		string		strWeaponSocketBone = { "BN_Weapon_R" };
		string		strWeaponTrailTopBone = { "FX_WP_Trail_R_1_1" };
		string		strWeaponTrailBotBone = { "FX_WP_Trail_R_1_5" };
		string		strWeaponTrailFx = { "Effect_Monster_Trail" };
		const char* pRootBoneName = { "Root" };

		_float		fShortAttackRange = { 3.f };
		_float		fAttackGap = { 0.5f };
		_float		fMoveDuration = { 1.2f };
		_float		fLoseDelay = { 2.f };
		_float		fSweepRadius = { 0.4f };
		_float		fSweepHalfHeight = { 0.6f };
		_float		fFatalFront = { 2.f };
		_float		fFatalBack = { 2.f };
	} MELEE_DESC;

protected:
	CMonster_Melee(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Melee(const CMonster_Melee& Prototype);
	virtual ~CMonster_Melee() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

	virtual void				On_Freeze(_bool bFreeze) override;
	virtual void				On_Groggy(_bool bGroggy) override;
	virtual void				On_SuperArmor(_bool bOn) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;

	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual void				On_SetDead() override;
	virtual _bool				Can_FatalFront() const override;
	virtual _bool				Can_FatalBack() const override;

	_float						Get_PatternLength(_int iID) const;

	void						Set_IntroPattern(_int iID) { m_iIntroPatternID = iID; }

protected:
	virtual void				On_Groggy_Triggered() override;

	HRESULT						Ready_Weapon(const MELEE_DESC& Desc);
	virtual HRESULT				Ready_NotifyKeys();
	virtual HRESULT				Ready_AttackData() { return S_OK; }
	void						Apply_MeleeDesc(const MELEE_DESC& Desc);

	void						Update_CombatParams(_float fTimeDelta);
	_int						Select_WeightedPattern(_float fDist);
	_int						Pick_CombatAction(_float fDist);
	void						Fire_Attack();
	void						Fire_Knockback();
	void						Mirror_ControllerParams();
	void						Drive_Steering(_float fTimeDelta);
	virtual void				Drive_MoveToTarget(_float fTimeDelta);
	_bool						Is_FacingTarget(_float fCosThreshold) const;
	_bool						Is_AnimIdle() const;
	void						Wake_And_FaceHit(const HitInfo& hitInfo);

	_bool						Try_Begin_Intro();

protected:
	virtual _float				Get_AttackRange() const { return m_fShortAttackRange; }
	virtual _bool				Wants_RunChase(_float fDist) const { return false; }
	virtual const _wstring& Get_KnockbackAction(_int iHitInfo) const;
	virtual void				On_Mirror_Extra() {}
	virtual void				On_Spawn_Post() {}

#ifdef _DEBUG
protected:
	virtual _int			Get_DebugMoveState() const override;
#endif


protected:
	CWeapon_Monster* m_pWeapon = { nullptr };
	unordered_map<_uint64, ATTACK_DATA>		m_AttackData;
	unordered_map<EFFECT_KEY, _uint64>		m_EffectKeys;
	const PATTERN_DEF* m_pPatterns = { nullptr };
	_int						m_iPatternDefCount = { 0 };

	_float						m_fDamage = { 5.f };
	_float						m_fSweepRadius = { 0.4f };
	_float						m_fSweepHalfHeight = { 0.6f };

	_bool						m_bDead = { false };
	string						m_strWeaponSocketBone = { "BN_Weapon_R" };
	string						m_strWeaponTrailTop = { "FX_WP_Trail_R_1_1" };
	string						m_strWeaponTrailBot = { "FX_WP_Trail_R_1_5" };

	_int						m_iCurPatternID = { 1 };
	_int						m_iLastPatternID = { 0 };
	_float						m_fIdleTimer = { 0.f };
	_float						m_fDistToTarget = { 9999.f };
	_float						m_fShortAttackRange = { 3.f };
	_float						m_fAttackGap = { 0.5f };

	_int						m_iIntroPatternID = { -1 };
	_bool						m_bIntroDone = { false };

	_bool						m_bMustAttack = { false };
	_bool						m_bLastMoveBack = { false };
	_bool						m_bMoving = { false };
	_bool						m_bCancelConsumed = { false };
	_float						m_fMoveTimer = { 0.f };
	_float						m_fMoveDuration = { 1.2f };
	_int						m_iMoveDir = { 0 };

public:
	virtual CGameObject* Clone(void* pArg) PURE;
	virtual void				Free() override;
};



NS_END
