#include "BT_RandomSelector.h"
#include "Engine_Utility.h"

CBT_RandomSelector::CBT_RandomSelector(const _wstring& strName)
	: CBT_Composite{strName}
{
}

CBT_Node::Status CBT_RandomSelector::Tick()
{
	if (false == m_bShuffled)
	{
		Build_Order();
		m_iCurrentIndex = 0;
	}

	for (; m_iCurrentIndex < ETOUI(m_Orders.size()); ++m_iCurrentIndex)
	{
		_uint iChild = m_Orders[m_iCurrentIndex];
		Status eResult = m_Childrens[iChild]->Tick();

		if (Status::Success == eResult)
		{
			m_bShuffled = false;
			m_iCurrentIndex = 0;
			return m_eStatus = Status::Success;
		}

		if (Status::Running == eResult)
			return m_eStatus = Status::Running;
	}

	m_bShuffled = false;
	m_iCurrentIndex = 0;
	return m_eStatus = Status::Failure;
}

void CBT_RandomSelector::Reset()
{
	m_bShuffled = false;
	m_Orders.clear();
	__super::Reset();
}

void CBT_RandomSelector::Abort()
{
	m_bShuffled = false;
	m_Orders.clear();
	__super::Abort();
}

void CBT_RandomSelector::Build_Order()
{
	const _uint iCount = ETOUI(m_Childrens.size());

	m_Orders.clear();
	m_Orders.reserve(iCount);

	_bool bUseWeight = (m_Weights.size() == iCount && 0 != iCount);

	if (bUseWeight)
	{
		vector<_uint>	Remains;
		vector<_float>	Weights;
		Remains.reserve(iCount);
		Weights.reserve(iCount);

		for (_uint i = 0; i < iCount; i++)
		{
			Remains.push_back(i);

			_float fW = (m_Weights[i] > 0.f) ? m_Weights[i] : 0.f;
			if (ETOI(i) == m_iLastPicked)
				fW *= m_fRepeatPenalty;
			Weights.push_back(fW);
		}

		const _uint iRemain = ETOUI(Remains.size());

		while (false == Remains.empty())
		{			
			_float fTotal = 0.f;
			for (auto fW : Weights)
				fTotal += fW;

			_uint iPick = 0;
			
			if (fTotal <= 0.f)
			{
				iPick = ETOUI(Random(0, ETOI(iRemain) - 1));
			}
			else
			{
				_float fRand = Random0To1() * fTotal;
				_float fAccum = 0.f;

				iPick = iRemain - 1;

				for (_uint i = 0; i < iRemain; i++)
				{
					fAccum += Weights[i];
					if (fRand < fAccum)
					{
						iPick = i;
						break;
					}
				}
			}

			m_Orders.push_back(Remains[iPick]);

			Remains.erase(Remains.begin() + iPick);
			Weights.erase(Weights.begin() + iPick);
		}
	}
	else
	{
		for (_uint i = 0; i < iCount; i++)
			m_Orders.push_back(i);

		for (_int i = ETOI(iCount) - 1; i > 0; --i)
		{
			_int j = Random(0, i);

			swap(m_Orders[i], m_Orders[j]);			
		}
		if(iCount > 1 && m_iLastPicked >= 0 && m_Orders[0] == ETOUI(m_iLastPicked))
			swap(m_Orders[0], m_Orders[1]);
	}
	m_bShuffled = true;
}

CBT_RandomSelector* CBT_RandomSelector::Create(const _wstring& strName)
{
	CBT_RandomSelector* pInstance = new CBT_RandomSelector(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_RandomSelector");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_RandomSelector::Free()
{
	__super::Free();
}
