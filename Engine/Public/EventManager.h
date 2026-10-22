#pragma once
#include "Base.h"
#include "IListener.h"

NS_BEGIN(Engine)
class ENGINE_DLL CEvent_Manager final : public CBase
{
    CEvent_Manager();
    CEvent_Manager(const CEvent_Manager& rhs) = delete;
    CEvent_Manager& operator=(const CEvent_Manager& rhs) = delete;
    virtual ~CEvent_Manager() = default;

//public:
//    void                                Subscribe(EVENT_TYPE _event, IListener* _listener);
//    void                                UnSubscribe(EVENT_TYPE _event, IListener* _listener);
//    void                                Broadcast(EVENT_TYPE _event, Event_Desc* _pData);
//    void                                Clear_All();

public:
    template<typename Event> 
    void Subscribe(Event event, IListener* _listener)
    {
        Subscribe_Key(To_EventKey(event), _listener);
	}

    template<typename Event>
    void UnSubscribe(Event event, IListener* _listener)
    {
        UnSubscribe_Key(To_EventKey(event), _listener);
    }
    
    template<typename Event>
    void Broadcast(Event event, Event_Desc* _pData)
    {
        list<IListener*>* pListeners = Find_Listeners(To_EventKey(event));
        if (nullptr == pListeners)
            return;

		//Broadcast중에 Subscribe/Unsubscribe해서 노드 무효화를 대비하여 안전하게 복사본을 만들어서 Broadcast함
		list<IListener*> Savelisteners = *pListeners;
        for (auto& listener : Savelisteners)
        {
            listener->OnEvent(event, _pData);
        }
	}

public:
    void                                UnSubscribe_AllEvent(IListener* _listener);
	void                                Clear_All();
public:
    

private:
	void                                Subscribe_Key(_uint iKey, IListener* plistener);
    void                                UnSubscribe_Key(_uint iKey, IListener* plistener);
	list<IListener*>*                   Find_Listeners(_uint iKey);

private:
    //unordered_map<EVENT_TYPE, list<IListener*>>   m_Listeners;
    unordered_map<_uint, list<IListener*>>   m_Listeners;
public:
    static CEvent_Manager*              Create();
    virtual void                        Free();

};

NS_END