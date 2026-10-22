#pragma once
#include "Weapon_Monster.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CWeapon_Range final : public CWeapon_Monster
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CWeapon_Range)
#endif

public:
	enum class FIRE_MODE { HITSCAN, PROJECTILE };

	typedef struct tagRangeDesc : public CWeapon_Monster::WEAPON_DESC
	{
		_float		fRange = { 30.f };
		_float		fBulletSpeed = { 40.f };
		_float		fBulletAccel = { 20.f };
		_float		fBulletRadius = { 0.2f };
		FIRE_MODE	eFireMode = { FIRE_MODE::PROJECTILE };
	}RANGE_DESC;

private:
	CWeapon_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon_Range(const CWeapon_Range& Prototype);
	virtual ~CWeapon_Range() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual void			Fire(_fvector vOrigin, _fvector vDir, _float fDamage = -1.f, HIT_TYPE eType = HIT_TYPE::Normal) override;
	virtual void			Update_Attack(_float fTimeDelta) override;

private:
	_int					m_iCurrentLevel = { -1 };
	_float					m_fRange = { 80.f };
	_float					m_fBulletSpeed = { 20.f };
	_float					m_fBulletAccel = { 20.f };
	_float					m_fBulletRadius = { 0.2f };
	FIRE_MODE				m_eFireMode = { FIRE_MODE::PROJECTILE };

public:
	static CWeapon_Range* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
