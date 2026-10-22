#include "PoolableObject.h"
#include "ObjectPool.h"

CPoolableObject::CPoolableObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CGameObject{ pDevice, pContext }
{
}

_uint CPoolableObject::Release()
{
	//풀에서 Release한 경우
	if (0 == m_iRefCnt)
	{
		Free();

		delete this;

		return 0;
	}
	else
	{
		m_iRefCnt--;

		//감소시킨 레퍼런스가 0이면 외부에서 다 Release 했다는 뜻
		if (m_iRefCnt == 0)
		{
			if (Has_OwnerPool())
			{
				//풀에 반납 
				return Return_ToPool();
			}
			else
			{
				//삭제할 관리자Pool가 없다면(풀이 삭제됨) 스스로 삭제함 
				Free();
				delete this; 
				return 0;
			}
		}
	}
	return m_iRefCnt;
}

void CPoolableObject::Free()
{
	__super::Free();
}
