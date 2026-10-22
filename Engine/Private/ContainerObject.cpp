#include "ContainerObject.h"
#include "GameInstance.h"
#include "PartObject.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CContainerObject, CGameObject)
END_REFLECTION(CContainerObject)
#endif

CContainerObject::CContainerObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CContainerObject::CContainerObject(const CContainerObject& Prototype)
    : CGameObject { Prototype }
{
}


HRESULT CContainerObject::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CContainerObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

void CContainerObject::Priority_Update(_float fTimeDelta)
{
}

void CContainerObject::Update(_float fTimeDelta)
{
}

void CContainerObject::Physical_Update(_float fFixedTime)
{
}

void CContainerObject::Late_Update(_float fTimeDelta)
{
    
}

HRESULT CContainerObject::Render()
{
    return S_OK;
}

CComponent* CContainerObject::Find_Component(const _wstring& strPartTag, const _wstring& strComponentTag)
{
    auto    iter = m_PartObjects.find(strPartTag);

    if (iter == m_PartObjects.end())
        return nullptr;

    return iter->second->Find_Component(strComponentTag);
}

CGameObject* CContainerObject::Get_PartObject(const _wstring& strPartTag)
{
    auto iter = m_PartObjects.find(strPartTag);
    if (iter != m_PartObjects.end()) return iter->second;

    return nullptr;
}

HRESULT CContainerObject::Add_PartObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, const _wstring& strPartTag, void* pArg)
{
    if (m_PartObjects.find(strPartTag) != m_PartObjects.end())  return E_FAIL;

    auto pPartObject = m_pGameInstance->Clone_Prototype<CPartObject>(iPrototypeLevelIndex, strPrototypeTag, pArg);

    if (nullptr == pPartObject)
        return E_FAIL;

    m_PartObjects.emplace(strPartTag, pPartObject);   

    return S_OK;
}

void CContainerObject::Free()
{
    __super::Free();

    for (auto& Pair : m_PartObjects)
        Safe_Release(Pair.second);

    m_PartObjects.clear();

}
