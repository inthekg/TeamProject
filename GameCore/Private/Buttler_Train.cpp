#include "Buttler_Train.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CButtler_Train, CMonster_Buttler)
END_REFLECTION(CButtler_Train)
#endif

CButtler_Train::CButtler_Train(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMonster_Buttler{ pDevice, pContext }
{
}

CButtler_Train::CButtler_Train(const CButtler_Train& Prototype)
    : CMonster_Buttler{ Prototype }
{
}

CButtler_Train* CButtler_Train::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CButtler_Train* pInstance = new CButtler_Train(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CButtler_Train");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CButtler_Train::Clone(void* pArg)
{
	CButtler_Train* pInstance = new CButtler_Train(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButtler_Train");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CButtler_Train::Free()
{
	__super::Free();
}
