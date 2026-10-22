#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "IListener.h"
#include "ChapterMapData.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(GameCore)
class CActorSectorManager;
class CNavigationManager;

// 챕터의 Monster 생성과 재생성만 담당한다.
class GAMECORE_DLL CMonsterSpawnManager final : public CBase
{
private:
    CMonsterSpawnManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLevel, LEVEL eProtoLevel = LEVEL::GAMEPLAY);
    virtual ~CMonsterSpawnManager() = default;

public:
    HRESULT Initialize(const CHAPTER_DESC& ChapterDesc, CActorSectorManager* pActorSectorManager, CNavigationManager* pNavigationManager);
    HRESULT Spawn_All();
    HRESULT Spawn_ForSector(_uint iSectorID);
	// 스폰된 적이 있는 섹터의 몬스터만 재스폰한다.
	HRESULT Respawn_Monsters();


private:
    HRESULT Spawn_Monster(const MONSTER_SPAWN_DATA& SpawnData);

    /* ADD */
	//Spawn~ 함수 수정. 각 몬스터의 고정된 level(gameplay)를 생성자에서 받은 eProtoLevel로 바꿔서 스폰하도록 수정.
    HRESULT Spawn_Buttler(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Range(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Elite(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Training(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Boss(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Raxasia_P1(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Raxasia_P2(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Balistar(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Butterfly(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_BigDog(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Zombie(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Priest(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Spawn_Donkey(const MONSTER_SPAWN_DATA& SpawnData);
    HRESULT Post_Spawn(const MONSTER_SPAWN_DATA& SpawnData);

private :
    SPAWN_WEAPON_TYPE Resolve_WeaponType(SPAWN_MONSTER_TYPE eMonster, SPAWN_WEAPON_TYPE eWeapon) const;
    _wstring Get_WeaponProtoTag(SPAWN_WEAPON_TYPE eWeapon) const;

private :
    template<typename TDesc>
    void Fill_CommonDesc(TDesc& Desc, const MONSTER_SPAWN_DATA& SpawnData)
    {
        Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
        Desc.fRotationPerSec = XMConvertToRadians(180.f);
        Desc.fGroggy = 0.f;
        Desc.fGroggyDecayDelay = 5.f;
        Desc.fGroggyDecayRate = 25.f;
        Desc.fFrontCos = 0.5f;
        Desc.iProtoLevel = ETOUI(m_eProtoLevel);
    }

private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    Engine::CGameInstance* m_pGameInstance = nullptr;

    unordered_map<_uint, vector<MONSTER_SPAWN_DATA>> m_MonsterSpawnsBySector = {};
    unordered_set<_uint> m_SpawnedSectorIDs = {};
    CActorSectorManager* m_pActorSectorManager = nullptr;
    CNavigationManager*  m_pNavigationManager = nullptr;

    LEVEL               m_eCurrentLevel = { LEVEL::GAMEPLAY };
	LEVEL               m_eProtoLevel = { LEVEL::GAMEPLAY };
public:
    static CMonsterSpawnManager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc, CActorSectorManager* pActorSectorManager, CNavigationManager* pNavigationManager, LEVEL eLevel = LEVEL::GAMEPLAY, LEVEL eProtoLevel = LEVEL::GAMEPLAY);
    virtual void Free() override;
};

NS_END
