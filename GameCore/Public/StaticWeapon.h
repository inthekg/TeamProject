#pragma once
#include "Weapon.h"

//TODO : 사용 여부가 확정되면 삭제 판단. 현재까지는 쓰지않음 
NS_BEGIN(GameCore)
class GAMECORE_DLL CStaticWeapon abstract : public CWeapon
{
protected:
	CStaticWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStaticWeapon(const CStaticWeapon& Prototype);
	virtual ~CStaticWeapon() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual CGameObject*	Clone(void* pArg) PURE;
	virtual void			Free() override;
};

NS_END