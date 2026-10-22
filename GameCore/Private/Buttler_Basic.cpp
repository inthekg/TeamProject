#include "Buttler_Basic.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CButtler_Basic, CMonster_Buttler)
END_REFLECTION(CButtler_Basic)
#endif

CButtler_Basic::CButtler_Basic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Buttler{ pDevice, pContext }
{
}

CButtler_Basic::CButtler_Basic(const CButtler_Basic& Prototype)
	: CMonster_Buttler{ Prototype }
{
}

CButtler_Basic* CButtler_Basic::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CButtler_Basic* pInstance = new CButtler_Basic(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CButtler_Basic");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CButtler_Basic::Clone(void* pArg)
{
	CButtler_Basic* pInstance = new CButtler_Basic(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButtler_Basic");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CButtler_Basic::Free()
{
	__super::Free();
}
