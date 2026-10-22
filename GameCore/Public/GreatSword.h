#pragma once
#include "SkeletalWeapon.h"

NS_BEGIN(GameCore)
class GAMECORE_DLL CGreatSword final :public CSkeletalWeapon
{
protected:
	CGreatSword(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CGreatSword(const CGreatSword& Prototype);
	virtual ~CGreatSword() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;

protected:
	virtual void			Execute_FableArts(_uint iStep) override;
	virtual HRESULT			Ready_Components() override;
	virtual HRESULT 		Ready_WeaponData() override;

public:
	static CGreatSword*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg);
	virtual void			Free() override;
};

NS_END