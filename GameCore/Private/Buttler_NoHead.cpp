#include "Buttler_NoHead.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CButtler_NoHead, CMonster_Buttler)
END_REFLECTION(CButtler_NoHead)
#endif

CButtler_NoHead::CButtler_NoHead(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Buttler{ pDevice, pContext }
{
}

CButtler_NoHead::CButtler_NoHead(const CButtler_NoHead& Prototype)
	: CMonster_Buttler{ Prototype }
{
}

CButtler_NoHead* CButtler_NoHead::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CButtler_NoHead* pInstance = new CButtler_NoHead(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CButtler_NoHead");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CButtler_NoHead::Clone(void* pArg)
{
	CButtler_NoHead* pInstance = new CButtler_NoHead(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButtler_NoHead");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CButtler_NoHead::Free()
{
	__super::Free();
}
