#include "Resource.h"

CResource::CResource(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CComponent(pDevice, pContext)
{
}

//TODO : Resource 는 복사생성자 제거하기 
CResource::CResource(const CResource& Prototype)
	: CComponent{ Prototype }
	, m_strResourceName{Prototype.m_strResourceName}
    , m_iID{Prototype.m_iID}
{
}

void CResource::Set_Name(_wstring strResourceName)
{
	m_strResourceName = strResourceName;
}


void CResource::Free()
{
	__super::Free();
}
