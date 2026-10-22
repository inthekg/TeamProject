#include "PoolManager.h"

CPool_Manager::CPool_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}	

CPool_Manager* CPool_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	return new CPool_Manager(pDevice, pContext);
}

void CPool_Manager::Free()
{
	Remove_AllPools();
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
	__super::Free();
}
