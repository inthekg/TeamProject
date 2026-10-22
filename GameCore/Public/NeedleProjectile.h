#pragma once

#include "Core_Defines.h"
#include "PoolableObject.h"

NS_BEGIN(Engine)

class CModel;

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CNeedleProjectile final : public CPoolableObject
{
public:
	// 화살 한 발을 발사할 때 전달하는 설정이다.
	typedef struct tagNeedleFireDesc
	{
		_float3 vOrigin = {};
		_float3 vDirection = { 0.f, 0.f, 1.f };

		_float fSpeed = { 30.f };
		_float fRange = { 20.f };
		_float fRadius = { 0.08f };
		_float fDamage = { 15.f };
	} FIRE_DESC;

private:
	CNeedleProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CNeedleProjectile() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

	virtual void Activate(void* pArg = nullptr) override;
	virtual void InActivate() override;

private:
	CModel* m_pModelCom = { nullptr };

	_float3 m_vDirection = { 0.f, 0.f, 1.f };

	_float m_fSpeed = { 30.f };
	_float m_fRemainRange = { 0.f };
	_float m_fRadius = { 0.08f };
	_float m_fDamage = { 15.f };

	unordered_set<CGameObject*> m_HitTargets = {};

public:
	static CNeedleProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
