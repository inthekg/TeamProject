#pragma once
#include "Monster_Buttler.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CButtler_Basic final : public CMonster_Buttler
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CButtler_Basic)
#endif

private:
	CButtler_Basic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CButtler_Basic(const CButtler_Basic& Prototype);
	virtual ~CButtler_Basic() = default;

public:
	static CButtler_Basic* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void		 Free() override;
};

NS_END
