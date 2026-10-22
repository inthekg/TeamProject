#include "Layer.h"
#include "GameObject.h"
#include "ContainerObject.h"

CLayer::CLayer()
{
}

CComponent* CLayer::Get_Component(const _wstring& strComponentTag, _uint iIndex)
{
	if (m_GameObjects.size() <= iIndex) return nullptr;

	auto	iter = m_GameObjects.begin();

	for (size_t i = 0; i < iIndex; i++)
		++iter;
	
	return (*iter)->Find_Component(strComponentTag);
}

CComponent* CLayer::Get_Component(const _wstring& strPartTag, const _wstring& strComponentTag, _uint iIndex)
{
	if (m_GameObjects.size() <= iIndex) return nullptr;

	auto	iter = m_GameObjects.begin();

	for (size_t i = 0; i < iIndex; i++)
		++iter;

	return dynamic_cast<CContainerObject*>((*iter))->Find_Component(strPartTag, strComponentTag);
}

HRESULT CLayer::Add_GameObject(CGameObject* pGameObject)
{
	if(nullptr == pGameObject)
		return E_FAIL;

	m_GameObjects.push_back(pGameObject);

	return S_OK;
}

void CLayer::Priority_Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if(nullptr != pGameObject)
			pGameObject->Priority_Update(fTimeDelta);
	}
		
}

void CLayer::Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Update(fTimeDelta);
	}
}

void CLayer::Physical_Update(_float fFixedTime)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Physical_Update(fFixedTime);
	}
}

void CLayer::Late_Update(_float fTimeDelta)
{
	//이전 프레임의 삭제대기 오브젝트들 삭제함. 이 오브젝트를 들고있는 곳이 있다면 1프레임 내에 제외함
	for (CGameObject* pPendingKillObject : m_PendingKillObjects)
	{
		if (nullptr != pPendingKillObject)
			Safe_Release(pPendingKillObject);
	}
	m_PendingKillObjects.clear();

	for (auto iter = m_GameObjects.begin(); iter != m_GameObjects.end();)
	{
		CGameObject* pObj = *iter;
		if (nullptr == pObj)
		{
			iter++;
			continue;
		}

		pObj->Late_Update(fTimeDelta);

		if (pObj->Is_InActive())
		{
			m_PendingKillObjects.push_back(pObj);
			iter = m_GameObjects.erase(iter);
		}
		else iter++;
	}
}

CLayer* CLayer::Create()
{
	return new CLayer();
}

void CLayer::Free()
{
	__super::Free();

	for (auto& pPendingKillObject : m_PendingKillObjects)
		Safe_Release(pPendingKillObject);

	m_PendingKillObjects.clear();

	for (auto& pGameObject : m_GameObjects)
		Safe_Release(pGameObject);

	m_GameObjects.clear();
}
