#include "Map.h"
#include "GameInstance.h"
#include "MapObject.h"
#include "MapInstanceObject.h"

CMap::CMap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice(pDevice), m_pContext(pContext), m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

}

HRESULT CMap::Initialize()
{
    //m_iNumObject = (_uint)objects.size();
    //m_objects = objects;
    return S_OK;
}

void CMap::Priority_Update(_float fTimeDelta)
{

}

void CMap::Update(_float fTimeDelta)
{

}

void CMap::Late_Update(_float fTimeDelta)
{

}

_fvector CMap::Get_MapObjectPos(_uint iIndex)
{
    if (iIndex >= m_iNumObjects) return XMVectorZero();
    _fvector vPosition = m_Objects[iIndex]->Get_Transform()->Get_State(STATE::POSITION);
    return vPosition;
}

void CMap::Add_GameObjects_ToMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    for(auto* object: m_Objects)
    {
        m_pGameInstance->Add_GameObject(iLayerLevelIndex, strLayerTag, object);
        //레퍼런스 증가시키고 보냄 
        object->AddRef();
    }
}

//void CMap::Remove_GameObjects_FromMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag)
//{
//    for (auto* object : m_Objects)
//    {
//        if (object) object->Set_InActive();
//    }
//    //해제되어 Ref감소되고 Mgr에서 빠지지만 Map이 소유중이라 삭제되지는 않음
//}
//
void CMap::Push_MapObject(CMapObject* pMapObject)
{
    if (pMapObject == nullptr) return;

    CMapInstanceObject* pInstanceObject = dynamic_cast<CMapInstanceObject*>(pMapObject);

    if (nullptr != pInstanceObject)
        pInstanceObject->Set_OwnerMap(this);

    m_Objects.push_back(pMapObject);
    m_iNumObjects++;
    Set_SpatialDirty(true);
}

void CMap::ReMove_MapObject(CMapObject* pMapObject)
{
    if (nullptr == pMapObject)
        return;

    auto iter = find(m_Objects.begin(), m_Objects.end(), pMapObject);
    if (iter == m_Objects.end())
        return;

    CMapInstanceObject* pInstanceObject = dynamic_cast<CMapInstanceObject*>(*iter);

    if (nullptr != pInstanceObject)
        pInstanceObject->Set_OwnerMap(nullptr);

    Safe_Release(*iter);
    m_Objects.erase(iter);
    m_iNumObjects = static_cast<_uint>(m_Objects.size());
    Set_SpatialDirty(true);
}


CMap* CMap::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMap* pInstance = new CMap(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMap");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMap::Free()
{
	__super::Free();

    for (auto obj : m_Objects)
    {
        Safe_Release(obj);
    }
    m_Objects.clear();


    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);


}
