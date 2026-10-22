#include "EventManager.h"
#include "GameInstance.h"


CEvent_Manager::CEvent_Manager()
{
}

void CEvent_Manager::UnSubscribe_AllEvent(IListener* _listener)
{
    for(auto& pair : m_Listeners)
    {
        pair.second.remove(_listener);
	}
}

void CEvent_Manager::Clear_All()
{
    m_Listeners.clear();
}


void CEvent_Manager::Subscribe_Key(_uint iKey, IListener* plistener)
{
    m_Listeners[iKey].push_back(plistener);
}

void CEvent_Manager::UnSubscribe_Key(_uint iKey, IListener* plistener)
{
    if (m_Listeners.find(iKey) == m_Listeners.end()) return;
    m_Listeners[iKey].remove(plistener);
}

list<IListener*>* CEvent_Manager::Find_Listeners(_uint iKey)
{
	auto iter = m_Listeners.find(iKey);

    if (iter == m_Listeners.end()) return nullptr;
    return &iter->second;
}

CEvent_Manager* CEvent_Manager::Create()
{
    CEvent_Manager* pInstance = new CEvent_Manager();
    if (pInstance == nullptr)
    {
        MSG_BOX("CEvent_Manager Create Failed!");
        return nullptr;
    }
    return pInstance;
}

void CEvent_Manager::Free()
{
    __super::Free();
    Clear_All();
}
