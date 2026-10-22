#pragma once
#include "Monster_Melee.h"
#include "BT_Node.h"
#include "HitBox_Helper.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_BigDog final : public CMonster_Melee
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_BigDog)
#endif

public:
	enum class REAR_MODE { FREE, CONTEXTUAL };

	typedef struct tagBigDogDesc : public CMonster_Melee::MELEE_DESC
	{
		_float		fFarAttackRange			= { 8.f };
		_float		fRunChaseRange			= { 6.f };
		_float		fApproachRange			= { 4.f };
		_float		fWalkSpeed				= { 2.f };
		_float		fChaseSpeed				= { 3.f };
		_float		fSkillMoveSpeed			= { 20.f };
		REAR_MODE	eRearMode				= { REAR_MODE::FREE };
		_float		fRearTailRange			= { 3.f };
		_float		fFatalFront				= { 2.5f };
		_float		fFatalBack				= { 2.5f };
	} MONSTER_DESC;

private:
	CMonster_BigDog(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_BigDog(const CMonster_BigDog& Prototype);
	virtual ~CMonster_BigDog() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;
	virtual void				Spawn() override;

	virtual void				On_CheckDead() override;
	virtual void				Set_FatalMode(_bool bMode) override;
	virtual CHitBox_Helper*		Get_HitBoxes() override { return &m_HitBoxes; }

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				On_Hit_Post(const HitInfo& hitInfo) override;
	virtual _bool				Blocks_Damage_While_Groggy() const override;
	virtual _bool				Uses_Full_Hit_Reaction() const override;

	virtual _float				Get_AttackRange() const override;
	virtual _bool				Wants_RunChase(_float fDist) const override;

	HRESULT						Ready_HitBoxes();

	void						Update_PatternSelect(_float fTimeDelta);
	void						Update_AttackLoop(_float fTimeDelta);
	void						Drive_Steering_BigDog(_float fTimeDelta);

	_bool						Is_RearPattern(_int iPatternID) const;
	_bool						Is_BackAligned();
	_bool						Is_TargetInFront();
	_bool						Is_TargetInRear();
	_int						Select_Pattern_Angle(_float fDist);
	_bool						Face_Target(_fvector vTargetPos, _float fTimeDelta, _bool bAway);
	_bool						Drive_Facing(_fvector vTargetPos, _float fTimeDelta, _bool bAway);

private:
	_float						m_fFarAttackRange		= { 8.f };
	_float						m_fRunChaseRange		= { 6.f };
	_float						m_fApproachRange		= { 4.f };
	_float						m_fSkillMoveSpeed		= { 80.f };

	_bool						m_bCounting				= { false };
	_float						m_fCheckTimer			= { 0.f };

	_bool						m_bRearSnapshot			= { false };
	_bool						m_bWasIdle				= { false };
	_bool						m_bAggroLossActive		= { false };

	REAR_MODE					m_eRearMode				= { REAR_MODE::FREE };
	_float						m_fRearTailRange		= { 3.f };
	_float						m_fAttackStartDeg		= { 45.f };
	_float						m_fRearConeDeg			= { 30.f };
	_bool						m_bTurning				= { false };
	_float						m_fTurnStartDeg			= { 15.f };
	_float						m_fTurnStopDeg			= { 5.f };
	_int						m_iTurnDir				= { 4 };

	_float						m_fMoveChance			= { 0.35f };

	CHitBox_Helper				m_HitBoxes;

public:
	static CMonster_BigDog*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
