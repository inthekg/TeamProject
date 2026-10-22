#include "MonsterSpawnManager.h"
#include "GameInstance.h"
#include "Layer.h"
#include "Transform.h"
#include "AIObject.h"
#include "ActorSectorManager.h"
#include "NavigationManager.h"
#include "Monster_Include.h"
#include "GamePlayMapPreset.h"


CMonsterSpawnManager::CMonsterSpawnManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLevel, LEVEL eProtoLevel)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
    , m_eCurrentLevel{ eLevel }
    , m_eProtoLevel{ eProtoLevel }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CMonsterSpawnManager::Initialize(const CHAPTER_DESC& ChapterDesc, CActorSectorManager* pActorSectorManager, CNavigationManager* pNavigationManager)
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    m_pActorSectorManager = pActorSectorManager;
    Safe_AddRef(m_pActorSectorManager);

    m_pNavigationManager = pNavigationManager;
    Safe_AddRef(m_pNavigationManager);

    m_MonsterSpawnsBySector.clear();
    m_SpawnedSectorIDs.clear();

    for (const MONSTER_SPAWN_DATA& SpawnData : ChapterDesc.MonsterSpawns)
    {
        if (UINT_MAX == SpawnData.iSectorID)
            continue;

        if (ChapterDesc.Sectors.end() == ChapterDesc.Sectors.find(SpawnData.iSectorID))
            continue;

        m_MonsterSpawnsBySector[SpawnData.iSectorID].push_back(SpawnData);
    }

#ifdef _DEBUG
#if GAMEPLAY_MAP_PRESET == GAMEPLAY_MAP_TEST || GAMEPLAY_MAP_PRESET==GAMEPLAY_MAP_BOSSMAP
    {
        _uint iTestSpawnIndex = 0;
        _float3 vBasePosition = GAMEPLAY_TEST_MONSTER_START_POSITION;

#define ADD_GAMEPLAY_TEST_MONSTER(MONSTER_TYPE) \
    { \
        MONSTER_SPAWN_DATA SpawnData{}; \
        SpawnData.iSpawnID = iTestSpawnIndex; \
        SpawnData.iSectorID = GAMEPLAY_TEST_MONSTER_SECTOR_ID; \
        SpawnData.eMonsterType = MONSTER_TYPE; \
        SpawnData.eWeaponType = SPAWN_WEAPON_TYPE::AUTO; \
        SpawnData.iSpawnNumber = static_cast<_int>(iTestSpawnIndex); \
        SpawnData.vPosition = { vBasePosition.x + GAMEPLAY_TEST_MONSTER_INTERVAL_X * static_cast<_float>(iTestSpawnIndex), vBasePosition.y, vBasePosition.z + GAMEPLAY_TEST_MONSTER_INTERVAL_Z * static_cast<_float>(iTestSpawnIndex) }; \
        SpawnData.fYawDegree = GAMEPLAY_TEST_MONSTER_YAW; \
        if (ChapterDesc.Sectors.end() != ChapterDesc.Sectors.find(SpawnData.iSectorID)) \
            m_MonsterSpawnsBySector[SpawnData.iSectorID].push_back(SpawnData); \
        ++iTestSpawnIndex; \
    }

        GAMEPLAY_TEST_MONSTER_LIST(ADD_GAMEPLAY_TEST_MONSTER)

#undef ADD_GAMEPLAY_TEST_MONSTER
    }
#endif
#else
#if GAMEPLAY_MAP_PRESET == GAMEPLAY_MAP_TEST || GAMEPLAY_MAP_PRESET==GAMEPLAY_MAP_BOSSMAP
    {
        _uint iTestSpawnIndex = 0;
        _float3 vBasePosition = GAMEPLAY_TEST_MONSTER_START_POSITION;

#define ADD_GAMEPLAY_TEST_MONSTER(MONSTER_TYPE) \
    { \
        MONSTER_SPAWN_DATA SpawnData{}; \
        SpawnData.iSpawnID = iTestSpawnIndex; \
        SpawnData.iSectorID = GAMEPLAY_TEST_MONSTER_SECTOR_ID; \
        SpawnData.eMonsterType = MONSTER_TYPE; \
        SpawnData.eWeaponType = SPAWN_WEAPON_TYPE::AUTO; \
        SpawnData.iSpawnNumber = static_cast<_int>(iTestSpawnIndex); \
        SpawnData.vPosition = { vBasePosition.x + GAMEPLAY_TEST_MONSTER_INTERVAL_X * static_cast<_float>(iTestSpawnIndex), vBasePosition.y, vBasePosition.z + GAMEPLAY_TEST_MONSTER_INTERVAL_Z * static_cast<_float>(iTestSpawnIndex) }; \
        SpawnData.fYawDegree = GAMEPLAY_TEST_MONSTER_YAW; \
        if (ChapterDesc.Sectors.end() != ChapterDesc.Sectors.find(SpawnData.iSectorID)) \
            m_MonsterSpawnsBySector[SpawnData.iSectorID].push_back(SpawnData); \
        ++iTestSpawnIndex; \
    }

        GAMEPLAY_TEST_MONSTER_LIST(ADD_GAMEPLAY_TEST_MONSTER)

#undef ADD_GAMEPLAY_TEST_MONSTER
    }
#endif
#endif // _DEBUG



    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_All()
{
    for (const auto& Pair : m_MonsterSpawnsBySector)
    {
        if (FAILED(Spawn_ForSector(Pair.first)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_ForSector(_uint iSectorID)
{
    if (UINT_MAX == iSectorID)
        return S_OK;

    if (m_SpawnedSectorIDs.end() != m_SpawnedSectorIDs.find(iSectorID))
        return S_OK;

    auto MonsterIter = m_MonsterSpawnsBySector.find(iSectorID);
    m_SpawnedSectorIDs.emplace(iSectorID);

    if (m_MonsterSpawnsBySector.end() == MonsterIter)
        return S_OK;

    for (const MONSTER_SPAWN_DATA& SpawnData : MonsterIter->second)
    {
        if (FAILED(Spawn_Monster(SpawnData)))
            return E_FAIL;
    }

    return S_OK;
}

//추가 
HRESULT CMonsterSpawnManager::Respawn_Monsters()
{
    for (_uint iSectorID : m_SpawnedSectorIDs)
    {
        auto iter = m_MonsterSpawnsBySector.find(iSectorID);
        if (m_MonsterSpawnsBySector.end() == iter) continue;

        for (const MONSTER_SPAWN_DATA& SpawnData : iter->second)
        {
            if (FAILED(Spawn_Monster(SpawnData)))
                return E_FAIL;
        }
    }
    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Monster(const MONSTER_SPAWN_DATA& SpawnData)
{
    HRESULT hrSpawn = E_FAIL;

    switch (SpawnData.eMonsterType)
    {
    case SPAWN_MONSTER_TYPE::BUTTLER_BASE:
    case SPAWN_MONSTER_TYPE::BUTTLER_TRAIN:
    case SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD:
        hrSpawn = Spawn_Buttler(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::BUTTLER_RANGE:
        hrSpawn = Spawn_Range(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::MONSTER_ELITE:
        hrSpawn = Spawn_Elite(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::TRAINING:
        hrSpawn = Spawn_Training(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::BOSS_FESTIVAL:
        hrSpawn = Spawn_Boss(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1:
        hrSpawn = Spawn_Raxasia_P1(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2:
        hrSpawn = Spawn_Raxasia_P2(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::BALISTAR:
        hrSpawn = Spawn_Balistar(SpawnData);
        break;

    case SPAWN_MONSTER_TYPE::BUTTERFLY:
        hrSpawn = Spawn_Butterfly(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::BIG_DOG:
        hrSpawn = Spawn_BigDog(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::ZOMBIE:
        hrSpawn = Spawn_Zombie(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::PRIEST:
        hrSpawn = Spawn_Priest(SpawnData);
        break;
    case SPAWN_MONSTER_TYPE::BOSS_DONKEY:
        hrSpawn = Spawn_Donkey(SpawnData);
        break;

    default:
    {
        MSG_BOX("SpawnManager :: Not Set MonsterType ");
        return S_OK;
    }
    }

    if (FAILED(hrSpawn))
        return E_FAIL;

    if (FAILED(Post_Spawn(SpawnData)))
    {
        MSG_BOX("FAILED POST SPAWN BY MONSTER :: SPAWNMANAGER");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Buttler(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Buttler::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 2.f;
    Desc.fHP = 3000.f;
    Desc.fMaxHP = 3000.f;
    Desc.fGroggy = 0.f;
    Desc.fMaxGroggy = 70.f;
    Desc.fDetectRange = 10.f;
    Desc.fLeashRange = 10.f;
    Desc.fAttackRange = 10.f;

    switch (SpawnData.eMonsterType)
    {
    case SPAWN_MONSTER_TYPE::BUTTLER_TRAIN:
        Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Buttler_Train");
        Desc.strAnimFolderTag = TEXT("Buttler_Train");
        break;

    case SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD:
        Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Buttler_NoHead");
        Desc.strAnimFolderTag = TEXT("Buttler_NoHead");
        break;

    default:
        Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Buttler_Basic");
        Desc.strAnimFolderTag = TEXT("Buttler_Basic");
        break;
    }

    Desc.strWeaponProtoTag = Get_WeaponProtoTag(Resolve_WeaponType(SpawnData.eMonsterType, SpawnData.eWeaponType));
    Desc.strWeaponTrailFx = "Effect_Monster_Trail";
    Desc.fShortAttackRange = 3.f;
    Desc.fAttackGap = 0.1f;
    Desc.fMoveDuration = 2.f;
    Desc.fLoseDelay = 1.f;
    Desc.fSweepRadius = 0.4f;
    Desc.fSweepHalfHeight = 0.6f;
    Desc.iPatternCount = 3;
    Desc.iSpawnNumber = SpawnData.iSpawnNumber;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Buttler* pMonster = nullptr;

    switch (SpawnData.eMonsterType)
    {
    case SPAWN_MONSTER_TYPE::BUTTLER_TRAIN:
        pMonster = m_pGameInstance->Get_PoolObject<CButtler_Train>(&Desc);
        break;

    case SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD:
        pMonster = m_pGameInstance->Get_PoolObject<CButtler_NoHead>(&Desc);
        break;

    default:
        pMonster = m_pGameInstance->Get_PoolObject<CButtler_Basic>(&Desc);
        break;
    }

    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Range(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Range::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fHP = 3000.f;
    Desc.fMaxHP = 3000.f;
    Desc.fMaxGroggy = 70.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Monster_Range");
    Desc.strAnimFolderTag = TEXT("Monster_Range");
    Desc.strWeaponProtoTag = Get_WeaponProtoTag(SPAWN_WEAPON_TYPE::RANGE);
    Desc.strWeaponSocketBone = "Hand_Weapon_R_01";
    Desc.strFireBoneName = "Weapon_Fire_01";

    Desc.fAttackRange = 80.f;
    Desc.fDetectRange = 80.f;
    Desc.fLeashRange = 10.f;
    Desc.fAttackGap = 0.1f;
    Desc.fLoseDelay = 1.f;

    Desc.fBulletDamage = 500.f;
    Desc.fBulletSpeed = 10.f;
    Desc.fBulletRange = 80.f;
    Desc.fBulletRadius = 0.2f;

    Desc.fFatalFront = 2.f;
    Desc.fFatalBack = 2.f;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Range* pMonster = m_pGameInstance->Get_PoolObject<CMonster_Range>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Elite(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Elite::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 1.f;
    Desc.fHP = 10000.f;
    Desc.fMaxHP = 10000.f;
    Desc.fMaxGroggy = 140.f;
    Desc.fGroggyDecayDelay = 5.f;
    Desc.fDetectRange = 10.f;
    Desc.fLeashRange = 10.f;
    Desc.fAttackRange = 10.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Monster_Elite");
    Desc.strAnimFolderTag = TEXT("Monster_Elite");
    Desc.strWeaponProtoTag = Get_WeaponProtoTag(Resolve_WeaponType(SpawnData.eMonsterType, SpawnData.eWeaponType));
    Desc.strWeaponTrailFx = "Effect_Monster_Trail";
    Desc.fShortAttackRange = 5.f;
    Desc.fFarAttackRange = 10.f;
    Desc.fRunChaseRange = 15.f;
    Desc.fAttackGap = 0.1f;
    Desc.fMoveDuration = 2.f;
    Desc.fLoseDelay = 1.f;
    Desc.fSweepRadius = 0.4f;
    Desc.fSweepHalfHeight = 0.6f;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Elite* pMonster = m_pGameInstance->Get_PoolObject<CMonster_Elite>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Training(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Training::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 1.5f;
    Desc.fHP = 5000.f;
    Desc.fMaxHP = 5000.f;
    Desc.fMaxGroggy = 70.f;
    Desc.fGroggyDecayDelay = 2.f;
    Desc.fDetectRange = 10.f;
    Desc.fAttackRange = 2.f;
    Desc.eMonsterType = SpawnData.eMonsterType;

    CMonster_Training* pMonster = m_pGameInstance->Get_PoolObject<CMonster_Training>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Boss(const MONSTER_SPAWN_DATA& SpawnData)
{
    CBoss_Festival::MONSTER_DESC Desc{};
    Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
    Desc.fRotationPerSec = XMConvertToRadians(180.f);
    Desc.fSpeedPerSec = 5.f;
    Desc.fHP = 30000.f;
    Desc.fMaxHP = 30000.f;
    Desc.fGroggy = 0.f;
    Desc.fMaxGroggy = 240.f;
    Desc.fGroggyDecayDelay = 10.f;
    Desc.fGroggyDecayRate = 25.f;
    Desc.fDetectRange = 15.f;
    Desc.fAttackRange = 8.f;
    Desc.fFrontCos = 0.5f;
    Desc.fShortAttackRange = 5.f;
    Desc.fFarAttackRange = 15.f;
    Desc.fPhase2Ratio = 0.5f;
    Desc.fAttackGap = 0.35f;
    Desc.bWeaponTrailUse = false;
    Desc.strWeaponTrailFx = "Effect_Player_Normal_Trail";
    Desc.iProtoLevel = ETOUI(m_eProtoLevel);
    Desc.eMonsterType = SpawnData.eMonsterType;

    CBoss_Festival* pMonster = m_pGameInstance->Get_PoolObject<CBoss_Festival>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Raxasia_P1(const MONSTER_SPAWN_DATA& SpawnData)
{
    CBoss_Raxasia_P1::MONSTER_DESC Desc{};
    Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
    Desc.fRotationPerSec = XMConvertToRadians(360.f);
    Desc.fSpeedPerSec = 1.f;
    Desc.fHP = 40000.f;
    Desc.fMaxHP = 40000.f;
    Desc.fGroggy = 0.f;
    Desc.fMaxGroggy = 350.f;
    Desc.fGroggyDecayDelay = 10.f;
    Desc.fGroggyDecayRate = 25.f;
    Desc.fDetectRange = 15.f;
    Desc.fAttackRange = 8.f;
    Desc.fFrontCos = 0.5f;
    Desc.fShortAttackRange = 5.f;
    Desc.fFarAttackRange = 15.f;
    Desc.fAttackGap = 0.35f;
    Desc.iProtoLevel = ETOUI(m_eProtoLevel);
    Desc.eMonsterType = SpawnData.eMonsterType;

    CBoss_Raxasia_P1* pMonster = m_pGameInstance->Get_PoolObject<CBoss_Raxasia_P1>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    // 테스트 맵에는 1페이즈 컷신 종료 이벤트가 없으므로 스폰 직후 전투 상태로 전환한다.
    if (LEVEL::CHAP_TEST == m_eProtoLevel)
        pMonster->Begin_Battle();

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Raxasia_P2(const MONSTER_SPAWN_DATA& SpawnData)
{
    CBoss_Raxasia_P2::MONSTER_DESC Desc{};
    Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
    Desc.fRotationPerSec = XMConvertToRadians(360.f);
    Desc.fSpeedPerSec = 5.f;
    Desc.fHP = 46000.f;
    Desc.fMaxHP = 46000.f;
    Desc.fGroggy = 0.f;
    Desc.fMaxGroggy = 350.f;
    Desc.fGroggyDecayDelay = 10.f;
    Desc.fGroggyDecayRate = 25.f;
    Desc.fDetectRange = 15.f;
    Desc.fAttackRange = 8.f;
    Desc.fFrontCos = 0.5f;
    Desc.fAttackGap = 0.35f;
    Desc.iProtoLevel = ETOUI(m_eProtoLevel);
    Desc.eMonsterType = SpawnData.eMonsterType;

    CBoss_Raxasia_P2* pMonster = m_pGameInstance->Get_PoolObject<CBoss_Raxasia_P2>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Balistar(const MONSTER_SPAWN_DATA& SpawnData)
{
    CBalistar::BALISTAR_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);

    Desc.fHP = 100.f;
    Desc.fMaxHP = 100.f;
    Desc.fMaxGroggy = 100.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Balistar");
    Desc.strAnimFolderTag = TEXT("Balistar");
    Desc.strRootBoneName = "Base";
    Desc.strFireBone = "MissileTurret_Muzzle";
    Desc.strGazeBaseBone = "BN_Top_Shooter_F";

    Desc.fDetectRange = 80.f;
    Desc.fAttackRange = 80.f;
    Desc.fLeashRange = 60.f;
    Desc.fAttackGap = 2.f;
    Desc.fLoseGrace = 0.5f;

    Desc.fBulletDamage = 800.f;
    Desc.fBulletSpeed = 80.f;
    Desc.fBulletRange = 80.f;
    Desc.fBulletRadius = 0.5f;
    Desc.iMaxHit = 1;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CBalistar* pTurret = m_pGameInstance->Get_PoolObject<CBalistar>(&Desc);
    if (nullptr == pTurret)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pTurret)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonsterSpawnManager::Spawn_Butterfly(const MONSTER_SPAWN_DATA& SpawnData)
{
    CAIObject::AIOBJECT_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);

    Desc.fHP = 2000.f;
    Desc.fMaxHP = 2000.f;
    Desc.fMaxGroggy = 1000000.f;
    Desc.fDetectRange = 10.f;
    Desc.fAttackRange = 3.f;
    Desc.fLeashRange = 20.f;
    Desc.fSpeedPerSec = 1.f;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Butterfly* pButterfly = m_pGameInstance->Get_PoolObject<CMonster_Butterfly>(&Desc);
    if (nullptr == pButterfly)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pButterfly)))
        return E_FAIL;

    return S_OK;

}

HRESULT CMonsterSpawnManager::Spawn_BigDog(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_BigDog::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fHP = 10000.f;
    Desc.fMaxHP = 10000.f;
    Desc.fMaxGroggy = 200.f;
    Desc.fRotationPerSec = XMConvertToRadians(75.f);
    Desc.fGroggyDecayDelay = 5.f;
    Desc.fDetectRange = 15.f;
    Desc.fLeashRange = 80.f;
    Desc.fAttackRange = 10.f;
    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_BigDog");
    Desc.strAnimFolderTag = TEXT("BigDog");
    Desc.fShortAttackRange = 6.f;
    Desc.fFarAttackRange = 10.f;
    Desc.fRunChaseRange = 8.f;
    Desc.fApproachRange = 6.f;
    Desc.fFrontCos = 0.5f;
    Desc.fAttackGap = 0.8f;
    Desc.eRearMode = CMonster_BigDog::REAR_MODE::CONTEXTUAL;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);
    CMonster_BigDog* pMonster = m_pGameInstance->Get_PoolObject<CMonster_BigDog>(&Desc);
    if (nullptr == pMonster) return E_FAIL;
    return m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster);
}

HRESULT CMonsterSpawnManager::Spawn_Zombie(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Zombie::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 1.f;
    Desc.fHP = 3000.f;
    Desc.fMaxHP = 3000.f;
    Desc.fMaxGroggy = 70.f;
    Desc.fGroggyDecayDelay = 5.f;
    Desc.fDetectRange = 30.f;
    Desc.fLeashRange = 60.f;
    Desc.fAttackRange = 10.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Zombie");
    Desc.strAnimFolderTag = TEXT("Monster_Zombie");

    Desc.fShortAttackRange = 3.f;
    Desc.fFarAttackRange = 7.f;
    Desc.fRunChaseRange = 6.f;
    Desc.fAttackGap = 0.2f;
    Desc.fMoveDuration = 1.2f;
    Desc.fLoseDelay = 1.f;
    Desc.fFatalFront = 1.2f;
    Desc.fFatalBack = 1.2f;

    Desc.iSpawnNumber = (SpawnData.iSpawnNumber > 0) ? SpawnData.iSpawnNumber : 1;
    Desc.iSpawnID = SpawnData.iSpawnID;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Zombie* pMonster = m_pGameInstance->Get_PoolObject<CMonster_Zombie>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    return m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster);
}

HRESULT CMonsterSpawnManager::Spawn_Priest(const MONSTER_SPAWN_DATA& SpawnData)
{
    CMonster_Priest::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 1.f;
    Desc.fHP = 8000.f;
    Desc.fMaxHP = 8000.f;
    Desc.fMaxGroggy = 100.f;
    Desc.fGroggyDecayDelay = 5.f;
    Desc.fDetectRange = 30.f;
    Desc.fLeashRange = 60.f;
    Desc.fAttackRange = 10.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_Priest");
    Desc.strAnimFolderTag = TEXT("Priest");
    Desc.strWeaponProtoTag = TEXT("Prototype_Component_Model_Weapon_Bapist");
    Desc.strWeaponTrailFx = "Effect_Player_Normal_Trail";
    Desc.fShortAttackRange = 3.f;
    Desc.fFarAttackRange = 7.f;
    Desc.fRunChaseRange = 6.f;
    Desc.fAttackGap = 0.2f;
    Desc.fMoveDuration = 1.2f;
    Desc.fLoseDelay = 1.f;
    Desc.fFatalFront = 1.6f;
    Desc.fFatalBack = 1.6f;
    Desc.fRunSpeed = 2.2f;
    Desc.fSprintSpeed = 2.2f;
    Desc.eMonsterType = SpawnData.eMonsterType;

    if (nullptr != m_pNavigationManager)
        Desc.pNavigation = m_pNavigationManager->Find_Navigation(SpawnData.iSectorID);

    CMonster_Priest* pMonster = m_pGameInstance->Get_PoolObject<CMonster_Priest>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    return m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster);
}

HRESULT CMonsterSpawnManager::Spawn_Donkey(const MONSTER_SPAWN_DATA& SpawnData)
{
    static const _int    DONKEY_SPAWNNUMBER_BOSS = 1;
    static const _int    DONKEY_SPAWNNUMBER_GATEKEEPER = 2;
    static const _uint   DONKEY_GATE_DOOR_SPAWNID = 41;
    static const _float  DONKEY_GATEKEEPER_WALKSPEED = 1.5f;
    static const _float3 DONKEY_GATE_EXIT_POS = { 97.772f, 141.186f,-112.573f };

    const _bool bGatekeeper = (DONKEY_SPAWNNUMBER_GATEKEEPER == SpawnData.iSpawnNumber);

    CBoss_MadDonkey::MONSTER_DESC Desc{};
    Fill_CommonDesc(Desc, SpawnData);
    Desc.fSpeedPerSec = 1.f;
    Desc.fRotationPerSec = XMConvertToRadians(180.f);
    Desc.fHP = 20000.f;
    Desc.fMaxHP = 20000.f;
    Desc.fMaxGroggy = 200.f;
    Desc.fGroggyDecayDelay = 10.f;
    Desc.fGroggyDecayRate = 25.f;
    Desc.fDetectRange = 20.f;
    Desc.fLeashRange = 80.f;
    Desc.fAttackRange = 8.f;

    Desc.strModelProtoTag = TEXT("Prototype_Component_Model_MadDonKey");
    Desc.strAnimFolderTag = TEXT("MadDonKey");
    Desc.strWeaponProtoTag = TEXT("Prototype_Component_Model_Weapon_GreatSaw");
    Desc.strWeaponTrailTopBone = "FX_WP_Trail_R_1_1";
    Desc.strWeaponTrailBotBone = "FX_WP_Trail_R_1_3";
    Desc.strWeaponTrailFx = "Effect_Player_Normal_Trail";
    Desc.pRootBoneName = "Root";
    Desc.iSpawnID = SpawnData.iSpawnID;
    Desc.iSpawnNumber = SpawnData.iSpawnNumber;
    Desc.eMonsterType = SpawnData.eMonsterType;

    Desc.fShortAttackRange = 4.f;
    Desc.fFarAttackRange = 8.f;
    Desc.fRunChaseRange = 6.f;
    Desc.fAttackGap = 0.4f;
    Desc.fMoveDuration = 1.2f;
    Desc.fFatalFront = 2.f;
    Desc.fFatalBack = 2.f;
    Desc.fRunSpeed = 3.f;
    Desc.fSprintSpeed = 5.0f;

    Desc.fSweepRadius = 0.5f;
    Desc.fSweepHalfHeight = 1.2f;
    Desc.fWeaponDamage = 1100.f;

    Desc.fCCTRadius = 0.6f;
    Desc.fCCTHeight = 1.6f;
    Desc.fAttackGap = 0.01f;

    if (true == bGatekeeper)
    {
        Desc.iTargetDoorSpawnID = DONKEY_GATE_DOOR_SPAWNID;
        Desc.fRunSpeed = DONKEY_GATEKEEPER_WALKSPEED;
        Desc.vExitGoalPos = DONKEY_GATE_EXIT_POS;
    }
    else
    {
        Desc.iTargetDoorSpawnID = UINT_MAX;
    }

    Desc.pNavigation = nullptr;

    CBoss_MadDonkey* pMonster = m_pGameInstance->Get_PoolObject<CBoss_MadDonkey>(&Desc);
    if (nullptr == pMonster)
        return E_FAIL;

    return m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"), pMonster);
}




HRESULT CMonsterSpawnManager::Post_Spawn(const MONSTER_SPAWN_DATA& SpawnData)
{
    CLayer* pMonsterLayer = m_pGameInstance->Get_Layer(ETOUI(m_eCurrentLevel), TEXT("Layer_Monster"));

    if (nullptr == pMonsterLayer || pMonsterLayer->Get_Objects().empty())
        return E_FAIL;

    CGameObject* pMonsterObject = pMonsterLayer->Get_Objects().back();

    if (nullptr == pMonsterObject)
        return E_FAIL;

    CTransform* pMonsterTransform = pMonsterObject->Get_Transform();

    if (nullptr != pMonsterTransform)
        pMonsterTransform->Set_Quat(XMQuaternionRotationRollPitchYaw(0.f, XMConvertToRadians(SpawnData.fYawDegree), 0.f));

    CLayer* pPlayerLayer = m_pGameInstance->Get_Layer(ETOUI(m_eCurrentLevel), TEXT("Layer_Character"));
    CGameObject* pPlayer = (nullptr != pPlayerLayer && false == pPlayerLayer->Get_Objects().empty()) ? pPlayerLayer->Get_Objects().back() : nullptr;

    if (SPAWN_MONSTER_TYPE::BALISTAR == SpawnData.eMonsterType)
    {
        static_cast<CBalistar*>(pMonsterObject)->Set_Target(pPlayer);
        return S_OK;
    }

    CAIObject* pMonsterAI = static_cast<CAIObject*>(pMonsterObject);

    if (nullptr != pPlayer)
        pMonsterAI->Set_Target(pPlayer);

    if (nullptr != m_pActorSectorManager)
        m_pActorSectorManager->Register_Monster(SpawnData.iSectorID, pMonsterAI);

    return S_OK;
}

SPAWN_WEAPON_TYPE CMonsterSpawnManager::Resolve_WeaponType(SPAWN_MONSTER_TYPE eMonster, SPAWN_WEAPON_TYPE eWeapon) const
{
    if (SPAWN_WEAPON_TYPE::AUTO != eWeapon)
        return eWeapon;

    switch (eMonster)
    {
    case SPAWN_MONSTER_TYPE::BUTTLER_BASE:      return SPAWN_WEAPON_TYPE::CANDLE;
    case SPAWN_MONSTER_TYPE::BUTTLER_TRAIN:     return SPAWN_WEAPON_TYPE::SIGN;
    case SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD:    return SPAWN_WEAPON_TYPE::KNIFE;
    case SPAWN_MONSTER_TYPE::BUTTLER_RANGE:     return SPAWN_WEAPON_TYPE::RANGE;
    case SPAWN_MONSTER_TYPE::MONSTER_ELITE:     return SPAWN_WEAPON_TYPE::ELITE;
    case SPAWN_MONSTER_TYPE::BOSS_FESTIVAL:     return SPAWN_WEAPON_TYPE::BOSS;
    default:                                    return SPAWN_WEAPON_TYPE::END;
    }
}

_wstring CMonsterSpawnManager::Get_WeaponProtoTag(SPAWN_WEAPON_TYPE eWeapon) const
{
    switch (eWeapon)
    {
    case SPAWN_WEAPON_TYPE::CANDLE: return TEXT("Prototype_Component_Model_Weapon_Candle");
    case SPAWN_WEAPON_TYPE::SIGN:   return TEXT("Prototype_Component_Model_Weapon_Train");
    case SPAWN_WEAPON_TYPE::KNIFE:  return TEXT("Prototype_Component_Model_Weapon_Knife");
    case SPAWN_WEAPON_TYPE::RANGE:  return TEXT("Prototype_Component_Model_Weapon_ShotGun");
    case SPAWN_WEAPON_TYPE::ELITE:  return TEXT("Prototype_Component_Model_Weapon_Elite");
    default:                        return TEXT("");
    }
}

CMonsterSpawnManager* CMonsterSpawnManager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc, CActorSectorManager* pActorSectorManager, CNavigationManager* pNavigationManager, LEVEL eLevel, LEVEL eProtoLevel)
{
    CMonsterSpawnManager* pInstance = new CMonsterSpawnManager(pDevice, pContext, eLevel, eProtoLevel);

    if (FAILED(pInstance->Initialize(ChapterDesc, pActorSectorManager, pNavigationManager)))
    {
        MSG_BOX("Failed to Created : CMonsterSpawnManager");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMonsterSpawnManager::Free()
{
    m_SpawnedSectorIDs.clear();
    m_MonsterSpawnsBySector.clear();

    Safe_Release(m_pNavigationManager);
    Safe_Release(m_pActorSectorManager);
    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);

    __super::Free();
}
