#include "GameObject.h"
#include "GameInstance.h"
#include "Transform.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CGameObject, CBase)
ADD_PROPERTY(CGameObject, m_strObjectName, WString)
END_REFLECTION(CGameObject)

#endif

CGameObject::CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{

    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

CGameObject::CGameObject(const CGameObject& Prototype)
    : m_pDevice{ Prototype.m_pDevice }
    , m_pContext{ Prototype.m_pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
    , m_iSortKey{Prototype.m_iSortKey}
    , m_strObjectName{Prototype.m_strObjectName }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CGameObject::Initialize_Prototype()
{
    m_eObjectState = OBJECT_STATE::ACTIVE;
    return S_OK;
}

HRESULT CGameObject::Initialize(void* pArg)
{
    if (nullptr != pArg)
    {
        auto        pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);
    }

    m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
    if (nullptr == m_pTransformCom)
        return E_FAIL;

    if (FAILED(m_pTransformCom->Initialize(pArg)))
        return E_FAIL;

    m_eObjectState = OBJECT_STATE::ACTIVE;

    m_Components.emplace(g_strTransformTag, m_pTransformCom);
    Safe_AddRef(m_pTransformCom);


    return S_OK;
}

void CGameObject::Priority_Update(_float fTimeDelta)
{

}

void CGameObject::Update(_float fTimeDelta)
{

}


void CGameObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CGameObject::Render()
{
    return S_OK;
}

unordered_map<_wstring, class CComponent*>* CGameObject::Get_ComponentMap()
{
    return &m_Components;
}

const _float4x4* CGameObject::Get_WorldTransform()
{
    if (m_pTransformCom == nullptr) return nullptr;
    return m_pTransformCom->Get_WorldMatrixPtr();
}

void CGameObject::Set_Name(_wstring strName)
{
    m_strObjectName = strName;
}

void CGameObject::Set_State(STATE eState, _fvector vState)
{
    if (m_pTransformCom == nullptr) return;
    m_pTransformCom->Set_State(eState, vState);
}

void CGameObject::Calculate_ViewDist()
{
    if (m_pTransformCom == nullptr) return;

    _vector vCameraPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());
    _vector vPosition = m_pTransformCom->Get_State(STATE::POSITION);

    m_fViewDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(vCameraPos, vPosition)));
}

HRESULT CGameObject::Add_Component(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, const _wstring& strComponentTag, CComponent** ppOut, void* pArg)
{
    if (nullptr != Find_Component(strComponentTag))
        return E_FAIL;

    CComponent* pComponent = m_pGameInstance->Clone_Prototype<CComponent>(iPrototypeLevelIndex, strPrototypeTag, pArg);
    if (nullptr == pComponent) return E_FAIL;

    m_Components.emplace(strComponentTag, pComponent);
    *ppOut = pComponent;

    Safe_AddRef(pComponent);

    return S_OK;
}

HRESULT CGameObject::Add_Component(CComponent* pComp, const _wstring& strComponentTag)
{
    if (Find_Component(strComponentTag) != nullptr)  return E_FAIL;

	if (pComp == nullptr) return E_FAIL;
    m_Components.emplace(strComponentTag, pComp);
    Safe_AddRef(pComp);
    return S_OK;
}

CComponent* CGameObject::Find_Component(const _wstring strComponentTag)
{
    auto    iter = m_Components.find(strComponentTag);
    if (iter == m_Components.end())
        return nullptr;

    return iter->second;
}

void CGameObject::Free()
{
    __super::Free();

    for (auto& Pair : m_Components)
        Safe_Release(Pair.second);
    m_Components.clear();

    Safe_Release(m_pTransformCom);
    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
