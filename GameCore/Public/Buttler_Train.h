#pragma once
#include "Monster_Buttler.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CButtler_Train final : public CMonster_Buttler
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CButtler_Train)
#endif

private:
	CButtler_Train(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CButtler_Train(const CButtler_Train& Prototype);
	virtual ~CButtler_Train() = default;

public:
	static CButtler_Train* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void		 Free() override;
};

NS_END
