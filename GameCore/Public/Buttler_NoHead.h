#pragma once
#include "Monster_Buttler.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CButtler_NoHead final : public CMonster_Buttler
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CButtler_NoHead)
#endif

private:
	CButtler_NoHead(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CButtler_NoHead(const CButtler_NoHead& Prototype);
	virtual ~CButtler_NoHead() = default;

public:
	static CButtler_NoHead* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void		 Free() override;
};

NS_END
