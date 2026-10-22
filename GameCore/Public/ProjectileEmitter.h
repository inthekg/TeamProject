#pragma once
#include "Core_Defines.h"
#include "PoolableObject.h"
#include "Projectile.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CProjectileEmitter final : public CPoolableObject
{
public:
	typedef struct tagEmitterArmDesc
	{
		CGameObject* pOwner = { nullptr };
		CGameObject* pTarget = { nullptr };
		_float3				vOrigin = {};
		_float3				vAimDir = { 0.f, 0.f, 1.f };
		CProjectile::MODE	eMode = { CProjectile::MODE::FAN };
		_int				iCount = { 7 };
		_float				fStepDeg = { 15.f };
		_bool				bFullCircle = { false };
		_float				fDelay = { 0.6f };
		const char* pTelegraphFX = { nullptr };
		_float				fSpeed = { 20.f };
		_float				fAccel = { 0.f };
		_float				fRange = { 30.f };
		_float				fRadius = { 0.4f };
		_float				fDamage = { 10.f };
		HIT_TYPE			eHitType = { HIT_TYPE::Heavy };
	} ARM_DESC;

private:
	CProjectileEmitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CProjectileEmitter(const CProjectileEmitter& Prototype);
	virtual ~CProjectileEmitter() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual void			Activate(void* pArg = nullptr) override;
	virtual void			InActivate() override;

private:
	void					Fire_Spread();

private:
	CGameObject* m_pOwner = { nullptr };
	CGameObject* m_pTarget = { nullptr };
	_float3					m_vOrigin = {};
	_float3					m_vAimDir = { 0.f, 0.f, 1.f };
	const char* m_pTelegraphFX = { nullptr };
	CProjectile::MODE		m_eMode = { CProjectile::MODE::FAN };
	_int					m_iCount = { 7 };
	_float					m_fStepDeg = { 15.f };
	_bool					m_bFullCircle = { false };
	_float					m_fDelay = { 0.f };
	_float					m_fSpeed = { 20.f };
	_float					m_fAccel = { 0.f };
	_float					m_fRange = { 30.f };
	_float					m_fRadius = { 0.4f };
	_float					m_fDamage = { 10.f };
	HIT_TYPE				m_eHitType = { HIT_TYPE::Heavy };
	_int					m_iCurrentLevel = { -1 };

public:
	static CProjectileEmitter* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
