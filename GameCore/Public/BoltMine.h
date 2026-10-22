#pragma once
#include "Core_Defines.h"
#include "PoolableObject.h"


NS_BEGIN(Engine)
#ifdef _DEBUG
class CDebugCapsule;
#endif
NS_END


NS_BEGIN(GameCore)

class GAMECORE_DLL CBoltMine final : public CPoolableObject
{
public :
	typedef struct tagBoltMineArmDesc
	{
		CGameObject*	pOwner			= { nullptr };
		_float3			vPosition		= {};
		_float			fDelay			= { 2.f };
		_float			fLingerDelay	= { 1.f };
		_float			fRadius			= { 1.5f };
		_float			fDamage			= { 50.f };
		HIT_TYPE		eHitType		= { HIT_TYPE::KnockBack };
	} ARM_DESC;

private :
	CBoltMine(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoltMine(const CBoltMine& Prototype);
	virtual ~CBoltMine() = default;

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
	void					Detonate();

private:
	CGameObject* m_pOwner = { nullptr };

#ifdef _DEBUG
	CDebugCapsule* m_pDbg = { nullptr };
#endif

	_bool					m_bDetonated = { false };
	_float					m_fDelay = { 2.f };
	_float					m_fLingerDelay = { 1.f };
	_float					m_fRadius = { 2.f };
	_float					m_fDamage = { 20.f };
	HIT_TYPE				m_eHitType = { HIT_TYPE::Heavy };

public:
	static CBoltMine*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END