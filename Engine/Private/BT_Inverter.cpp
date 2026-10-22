#include "BT_Inverter.h"

CBT_Inverter::CBT_Inverter(const _wstring& strName, CBT_Node* pChild)
	: CBT_Decorator{ strName, pChild }
{
}

CBT_Node::Status CBT_Inverter::Tick()
{
	if (nullptr == m_pChild)
		return m_eStatus = Status::Failure;

	Status eChildStatus = m_pChild->Tick();

	switch (eChildStatus)
	{
	case Status::Success:
		return m_eStatus = Status::Failure;
	case Status::Failure:
		return m_eStatus = Status::Success;
	case Status::Running:
		return m_eStatus = Status::Running;
	}

	return m_eStatus = Status::Failure;
}


CBT_Inverter* CBT_Inverter::Create(const _wstring strName, CBT_Node* pChild)
{
	CBT_Inverter* pInstance = new CBT_Inverter(strName, pChild);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Inverter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Inverter::Free()
{
	__super::Free();
}
