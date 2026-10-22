#include "CustomStaticObject.h"
#include "GameInstance.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CCustomStaticObject, CStaticObject)
//ADD_PROPERTY_SETTER(CCustomStaticObject, m_strShaderName, WString, Set_Shader, _wstring)
ADD_PROPERTY_SETTER(CCustomStaticObject, m_strModelName, WString, Set_ModelByName, _wstring)
END_REFLECTION(CCustomStaticObject)
#endif

CCustomStaticObject::CCustomStaticObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContexts)
	: CStaticObject{ pDevice, pContexts}
{
}

CCustomStaticObject::CCustomStaticObject(const CCustomStaticObject& Prototype)
	:CStaticObject{ Prototype }
{
}

HRESULT CCustomStaticObject::Initialize_Prototype()
{
	__super::Initialize_Prototype();
	return S_OK;
}

HRESULT CCustomStaticObject::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg))) return E_FAIL;
	return S_OK;
}

void CCustomStaticObject::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CCustomStaticObject::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CCustomStaticObject::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CCustomStaticObject::Render()
{
	if (FAILED(__super::Render())) return E_FAIL;
	return S_OK;
}

HRESULT CCustomStaticObject::Set_Model(CModel* pModelCom)
{
	if (m_pModelCom) Safe_Release(m_pModelCom);
	m_pModelCom = pModelCom;
	return S_OK;
}


#ifdef _DEBUG
HRESULT CCustomStaticObject::Set_ModelByName(_wstring strModelName)
{
	if (m_strModelName == strModelName)
		return S_FALSE;

	CModel* pModel = m_pGameInstance->Clone_Prototype<CModel>(3, strModelName);		
	if (pModel == nullptr) return E_FAIL;
	
	Set_Model(pModel);
	m_strModelName = strModelName;
	return S_OK;
}
#endif // _DEBUG

CCustomStaticObject* CCustomStaticObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCustomStaticObject* pInstance = new CCustomStaticObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Created : CCustomStaticObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCustomStaticObject::Clone(void* pArg)
{
	CCustomStaticObject* pInstance = new CCustomStaticObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CCustomStaticObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCustomStaticObject::Free()
{
	__super::Free();
}

