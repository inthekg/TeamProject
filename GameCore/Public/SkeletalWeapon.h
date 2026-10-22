#pragma once
#include "Weapon.h"

NS_BEGIN(GameCore)
class GAMECORE_DLL CSkeletalWeapon abstract : public CWeapon
{
protected:
	CSkeletalWeapon(ID3D11Device * pDevice, ID3D11DeviceContext * pContext);
	CSkeletalWeapon(const CSkeletalWeapon& Prototype);
	virtual ~CSkeletalWeapon() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;

public:
	virtual CGameObject*	Clone(void* pArg) PURE;
	virtual void			Free() override;
};

NS_END
