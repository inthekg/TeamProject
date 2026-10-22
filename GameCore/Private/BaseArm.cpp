#include "BaseArm.h"

CBaseArm::CBaseArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CLegionArm{ pDevice, pContext }
{
}

HRESULT CBaseArm::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CBaseArm : Arg is nullptr");
		return E_FAIL;
	}

	CLegionArm::LEGIONARM_DESC* pDesc = static_cast<CLegionArm::LEGIONARM_DESC*>(pArg);
	pDesc->strModelTag = L"Prototype_Component_Model_Player_ProtoArm";

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_eType = LegionArmType::ProtoArm;
	return S_OK;
}

void CBaseArm::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

CBaseArm* CBaseArm::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CBaseArm* pInstance = new CBaseArm(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CBaseArm");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBaseArm::Free()
{
	__super::Free();
}