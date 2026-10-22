#pragma once
#include "AIObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_Butterfly final : public CAIObject
{
public:
	enum class BFLY_STATE { GHOST, SPAWN, PATROL, ATTACK, HIT, FLEE, DEAD, DESPAWN };
	enum class EFFECT_KEY { MOVE, END };

protected:
	CMonster_Butterfly(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Butterfly(const CMonster_Butterfly& Prototype);
	virtual ~CMonster_Butterfly() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual _bool				Take_Damage(const HitInfo& hitInfo) override;
	virtual void				Spawn() override;
	virtual void				On_SetDead() override;
	virtual _bool				Can_FatalFront() const override;
	virtual _bool				Can_FatalBack() const override;
	virtual _bool				Is_Lockable() const override;

private:
	HRESULT						Ready_NotifyKeys();
	HRESULT						Ready_ButterflyLight();
	void						Tick_Butterfly(_float fTimeDelta);
	void						Do_SphereAttack();

	void						Collect_PatrolCells();
	void						Drive_Move(_float fTimeDelta, _bool bFlee);
	void						Drive_MoveEffect();
	_int						Pick_PatrolCell() const;
	_int						Pick_FleeCell(const _float3& vFrom) const;

	_bool						Get_DistToTarget(_float& fOut) const;

	void						Spawn_DropItem();
	void						Change_State(BFLY_STATE eNextState);
	void						Set_ButterflyLightEnable(_bool bEnable);
	void						Update_ButterflyLight();
	virtual void				Despawn() override;

private:
	static constexpr _float		BUTTERFLY_LIGHT_INTENSITY = { 1.2f };
	static constexpr _float		BUTTERFLY_LIGHT_RANGE = { 4.5f };
	static constexpr _float		BUTTERFLY_LIGHT_FALLBACK_Y = { 0.6f };

	BFLY_STATE					m_eState = { BFLY_STATE::GHOST };
	unordered_set<CGameObject*>	m_HitTargets;

	// 풀 객체마다 라이트 하나를 보유하고 상태에 따라 활성화만 전환한다.
	_uint						m_iButterflyLightIndex = { UINT_MAX };
	LIGHT_DESC					m_ButterflyLightDesc = {};
	_bool						m_bButterflyLightEnable = { false };
	const _float4x4* m_pButterflyLightBoneMatrix = { nullptr };

	vector<_uint>				m_PatrolCells;
	_uint						m_iPatrolDepth = { 2 };
	_bool						m_bPatrolCollected = { false };
	_float3						m_vMoveTarget = {};
	_bool						m_bHasMoveTarget = { false };
	_float3						m_vFleeFrom = {};
	_bool						m_bHitEntered = { false };
	_float						m_fHitKnockDist = { 1.5f };
	_float						m_fHitKnockTime = { 0.2f };

	_float						m_fLifeTimer = { 0.f };
	_float						m_fAttackCool = { 0.f };

	_float						m_fAttackRadius = { 3.f };
	_float						m_fAttackDamage = { 200.f };
	_float						m_fAttackCooldown = { 1.f };
	_float						m_fLifeMax = { 15000000.f };

	_float						m_fAlertRange = { 20.f };
	_bool						m_bAlerted = { false };
	string						m_strAlertSound = {};

	unordered_map<EFFECT_KEY, _uint64>	m_EffectKeys;
	_bool						m_bMoveFxOn = { false };

	_bool						m_bDeadPending = { false };
	_float						m_fDeadTimer = { 0.f };
	_float						m_fDeadDelay = { 1.f };

public:
	static CMonster_Butterfly* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
