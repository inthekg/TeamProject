#include "PartObject.h"
#include "RenderStruct.h"

CPartObject::CPartObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject { pDevice, pContext }
{
}

CPartObject::CPartObject(const CPartObject& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CPartObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CPartObject::Initialize(void* pArg)
{	
	auto		pDesc = static_cast<PARTOBJECT_DESC*>(pArg);

	m_pParentMatrix = pDesc->pParentMatrix;
	
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CPartObject::Priority_Update(_float fFixedTime)
{
}

void CPartObject::Update(_float fTimeDelta)
{
}

void CPartObject::Physical_Update(_float fFixedTime)
{
}

void CPartObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CPartObject::Render()
{
	return S_OK;
}

void CPartObject::Compute_CombinedWorldMatrix(_fmatrix ChildMatrix)
{
	XMStoreFloat4x4(&m_CombinedWorldMatrix, ChildMatrix* XMLoadFloat4x4(m_pParentMatrix));
}

void CPartObject::Free()
{
	__super::Free();


}
