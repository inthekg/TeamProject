#pragma once
#include "Core_Defines.h"
#include "PoolableObject.h"

NS_BEGIN(Engine)
#ifdef _DEBUG
class CDebugCapsule;
#endif
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CBoltSector final : public CPoolableObject
{
public:
	typedef struct tagBoltSectorArmDesc
	{
		CGameObject* pOwner = { nullptr };
		CGameObject* pTarget = { nullptr };
		_float3			vPosition = {};
		_bool			bUsePosition = { false };
		_float			fDelay = { 0.f };
		_float			fDuration = { 1.f };
		_float			fRadius = { 5.f };
		_float			fHalfAngleCos = { 0.866f };
		_float			fDamage = { 20.f };
		HIT_TYPE		eHitType = { HIT_TYPE::Heavy };
		_int			iType = { 0 };
	} ARM_DESC;


private:
	CBoltSector(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoltSector(const CBoltSector& Prototype);
	virtual ~CBoltSector() = default;

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
	CGameObject* m_pTarget = { nullptr };

#ifdef _DEBUG
	CDebugCapsule* m_pDbg = { nullptr };
#endif

	_bool					m_bHit = { false };
	_float					m_fDelay = { 0.f };
	_float					m_fTimer = { 0.f };
	_float					m_fDuration = { 1.f };
	_float					m_fRadius = { 5.f };
	_float					m_fHalfAngleCos = { 0.866f };
	_float					m_fDamage = { 20.f };
	HIT_TYPE				m_eHitType = { HIT_TYPE::Heavy };
	_int					m_iType = { 0 };

public:
	static CBoltSector*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END