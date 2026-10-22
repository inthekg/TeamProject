#include "Camera_Main.h"

CCamera_Main::CCamera_Main(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CCamera{pDevice, pContext }
{
}

HRESULT CCamera_Main::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	return S_OK;
}

CCamera_Main* CCamera_Main::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CCamera_Main* pInstance = new CCamera_Main(pDevice, pContext);
	if(FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CCamera_Main");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CCamera_Main::Free()
{
	__super::Free();
}
