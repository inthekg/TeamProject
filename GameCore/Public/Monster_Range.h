#pragma once
#include "AIObject.h"
#include "BT_Node.h"

NS_BEGIN(GameCore)

class CWeapon_Range;

class GAMECORE_DLL CMonster_Range final : public CAIObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Range)
#endif

public:	
	typedef struct tagRangeDesc : public CAIObject::AIOBJECT_DESC
	{
		_wstring	strModelProtoTag = {};
		_wstring	strAnimFolderTag = {};
		_wstring	strWeaponProtoTag = {};
		string		strWeaponSocketBone = { "Hand_Weapon_R_01" };
		string		strFireBoneName = { "Weapon_Fire_01" };
		const char* pRootBoneName = { "Root" };

		_float		fChaseRange = { 13.f };
		_float		fAttackGap = { 1.f };
		_float		fLoseDelay = { 2.f };

		_float		fBulletDamage = { 10.f };
		_float		fBulletSpeed = { 80.f };
		_float		fBulletRange = { 30.f };
		_float		fBulletRadius = { 0.2f };

		_float		fFatalFront = { 1.2f };
		_float		fFatalBack = { 1.2f };
	} MONSTER_DESC;

private:
	CMonster_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Range(const CMonster_Range& Prototype);
	virtual ~CMonster_Range() = default;

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
	virtual void				Set_FatalMode(_bool bMode) override;


	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual void				On_CheckDead() override;
	virtual void				On_SetDead() override;
	virtual _bool				Can_FatalFront() const override;
	virtual _bool				Can_FatalBack() const override;
	virtual void				On_Groggy(_bool bGroggy) override;


private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_Hit_Post(const HitInfo& hitInfo) override;
	virtual _bool				Blocks_Damage_While_Groggy() const override;
	virtual void				Apply_ChargeHit(_float fDamage, _float fGroggy) override;

	HRESULT						Ready_Weapon(const MONSTER_DESC& Desc);
	HRESULT						Ready_NotifyKeys();
	void						Update_CombatParams(_float fTimeDelta);
	void						Update_ShootSelect(_float fTimeDelta);
	void						Fire_Shot();
	void						Fire_Knockback();
	void						Mirror_ControllerParams();
	void						Drive_Steering(_float fTimeDelta);
	void						Drive_Aim(_float fTimeDelta);
	_bool						Get_FireOrigin(_vector& vOut);
	_bool						Is_FacingTarget(_float fCosThreshold) const;
	_bool						Is_AnimIdle() const;

private:
	CWeapon_Range*				m_pWeapon = { nullptr };
	_bool						m_bDead = { false };	

	string						m_strWeaponSocketBone = { "BN_Weapon_R" };
	string						m_strFireBoneName = { "Weapon_Fire_01" };

	_float						m_fChaseRange = { 13.f };
	_float						m_fAttackGap = { 1.f };
	_float						m_fIdleTimer = { 0.f };
	_float						m_fDistToTarget = { 9999.f };

	_float						m_fBulletDamage = { 10.f };
	_float						m_fBulletSpeed = { 40.f };
	_float						m_fBulletRange = { 30.f };
	_float						m_fBulletRadius = { 0.2f };

	_bool						m_bHasLOS = { false };
	_float						m_fVerticalVel = { 0.f };

public:
	static CMonster_Range*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
