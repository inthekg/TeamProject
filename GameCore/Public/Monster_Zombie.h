#pragma once
#include "Monster_Melee.h"
#include "BT_Node.h"
#include "HitBox_Helper.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_Zombie final : public CMonster_Melee, public IListener
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Zombie)
#endif

public:
	typedef struct tagZombieDesc : public CMonster_Melee::MELEE_DESC
	{
		_float		fFarAttackRange = { 7.f };
		_float		fRunChaseRange = { 6.f };
		_float		fFatalFront = { 1.2f };
		_float		fFatalBack = { 1.2f };
		_int		iSpawnNumber = { 1 };
		_uint		iSpawnID = { UINT_MAX };
	} MONSTER_DESC;

private:
	enum class SPAWN_PHASE { WAIT, FALL, LAND, DONE };

private:
	CMonster_Zombie(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Zombie(const CMonster_Zombie& Prototype);
	virtual ~CMonster_Zombie() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Late_Update(_float fTimeDelta) override;

	virtual void				Apply_SpawnDesc(void* pArg) override;

	virtual void				On_CheckDead() override;
	virtual void				Set_FatalMode(_bool bMode) override;

	virtual void				OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void				OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				On_Hit_Post(const HitInfo& hitInfo) override;
	virtual _bool				Blocks_Damage_While_Groggy() const override;

	virtual _float				Get_AttackRange() const override;
	virtual _bool				Wants_RunChase(_float fDist) const override;
	virtual void				Drive_MoveToTarget(_float fTimeDelta) override;
	virtual const _wstring& Get_KnockbackAction(_int iHitInfo) const override;

	virtual void				On_Spawn_Post() override;
	virtual void				On_Mirror_Extra() override;

	HRESULT						Ready_HitBoxes();
	void						Update_PatternSelect(_float fTimeDelta);
	_int						Select_LongPattern();

	void						Drive_SpawnSequence(_float fTimeDelta);
	void						Enter_LandSpawn();
	void						Capture_HomeCell();

private:
	_float						m_fFarAttackRange = { 7.f };
	_float						m_fRunChaseRange = { 6.f };

	_int						m_iSpawnNumber = { 1 };
	_uint						m_iSpawnID = { UINT_MAX };
	SPAWN_PHASE					m_eSpawnPhase = { SPAWN_PHASE::DONE };
	_bool						m_bBossStarted = { false };
	_bool						m_bAirborneOnce = { false };
	_float						m_fPhaseTimer = { 0.f };
	_int						m_iHomeCellIndex = { -1 };
	_bool						m_bEventSubscribed = { false };

	_float						m_fFallLatchVelY = { -5.f };
	_float						m_fLandTimeout = { 5.f };

	CHitBox_Helper				m_HitBoxes;

public:
	static CMonster_Zombie* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
