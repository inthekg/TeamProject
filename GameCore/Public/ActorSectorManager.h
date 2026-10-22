#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(GameCore)

class CAIObject;

class GAMECORE_DLL CActorSectorManager final : public CBase, public Engine::IListener
{
private:
	CActorSectorManager();
	virtual ~CActorSectorManager() = default;

public:
	HRESULT Initialize();
	void Register_Monster(_uint iSectorID, CAIObject* pMonster);
	void Update_MonsterSleep(const unordered_set<_uint>* pActiveSectorIDs);
	virtual void OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;

	//추가 : 몬스터가 죽거나 Despawn될 때, ActorSectorManager에서 제거용
	void UnRegister_Monster(CGameObject* pMonster);
	
private:
	Engine::CGameInstance* m_pGameInstance = nullptr;
	unordered_map<_uint, vector<CAIObject*>> m_MonstersBySector = {};
	unordered_set<_uint> m_ActiveSectorIDs = {};

public:
	static CActorSectorManager* Create();
	virtual void Free() override;
};

NS_END