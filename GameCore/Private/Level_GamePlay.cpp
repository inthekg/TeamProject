#include "Level_GamePlay.h"
#include "Camera_Free.h"
#include "GameInstance.h"
#include "Layer.h"
#include "Sprite.h"
#include "CustomStaticObject.h"
#include "Transform.h"
#include "Effect_Manager.h"
#include "Player.h"
#include "MapController.h"
#include "Map.h"
#include "Stargazer.h"
#include "ChapterMapManager.h"
#include "ChapterMapData.h"
#include "GamePlayMapPreset.h"
#include "EquipmentPreviewCharacter.h"
#include "EquipmentPreviewBackground.h"
#include "UILoadingController.h"
#include "UIFactory.h"
#include "CutScene_Manager.h"
#include "StargazerTravelData.h"

#include "FogVolume.h"
#include "CCT.h"
#include "Status.h"
#include "StaticRigidBody.h"
#include "Trigger.h"

#include "Loader.h"

#include "Monster_Include.h"

#ifdef _DEBUG
#include "NavigationDebugLineBuffer.h"
#include "NavigationDebugRenderer.h"
#endif

namespace
{
    const _float MAP_BGM_VOLUME = 0.35f;
    const _float MAP_BGM_FADE_SEC = 0.5f;
    const string CUTSCENE_RAXASIA_DEAD = "Raxasia_Dead";

    // g_ChapterFiles 와 같은 인덱스.
    // 0 Chapter01(LV_Krat_Outer) / 1 Hotel(LV_Hotel) / 2 Chapter02(LV_Monastery)
    // 3 TestFloor / 4 Test_object / 5 BossMap(LV_Krat_Outer)
    const char* MAP_BGM_KEYS[g_iMaxChapter] =
    {
        "AMB_SS_StreetStartPoint",
        "MU_MS_Hotel_Lobby_04",
        "MU_MS_MonasteryA_YK1_Fix",
        nullptr,
        nullptr,
        "AMB_SS_StreetStartPoint",
    };

    const char* Resolve_MapBGMKey(_uint iChapterIndex)
    {
        if (iChapterIndex >= g_iMaxChapter)
            return nullptr;

        return MAP_BGM_KEYS[iChapterIndex];
    }

    // 컷신이 끝나면 여기에 적힌 보스만 BOSS_START 를 받는다.
    typedef struct tagCutSceneBossStart
    {
        const char* pAssetName;
        SPAWN_MONSTER_TYPE  eBossType;
    } CUTSCENE_BOSS_START;

    const CUTSCENE_BOSS_START CUTSCENE_BOSS_STARTS[] =
    {
        { "Fguide",         SPAWN_MONSTER_TYPE::BOSS_FESTIVAL },
        { "Raxasia_Phase1", SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1 },
        { "Raxasia_Phase2", SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2 },
    };

    SPAWN_MONSTER_TYPE Resolve_CutSceneBossType(const string& strAssetName)
    {
        for (const CUTSCENE_BOSS_START& Data : CUTSCENE_BOSS_STARTS)
        {
            if (strAssetName == Data.pAssetName)
                return Data.eBossType;
        }

        return SPAWN_MONSTER_TYPE::END;
    }
}
CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
    : CLevel{ pDevice, pContext, hWnd }
{
}

HRESULT CLevel_GamePlay::Initialize()
{
    __super::Initialize();

    m_iCurrentChapter = GAMEPLAY_START_CHAPTER;
    if (FAILED(Ready_Lights()))
        return E_FAIL;

    if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
        return E_FAIL;

    if (FAILED(Ready_Effect()))
        return E_FAIL;

    if (FAILED(Rendy_Layer_Sky(TEXT("Layer_Sky"))))
        return E_FAIL;

    if (FAILED(Ready_Layer_Map(TEXT("Layer_Map"))))
        return E_FAIL;

    if (FAILED(Ready_Layer_Character(TEXT("Layer_Character"))))
        return E_FAIL;

    if (FAILED(Ready_Layer_EquipmentPreview(TEXT("Layer_EquipmentPreview"))))
        return E_FAIL;

    if (FAILED(Ready_UI()))
        return E_FAIL;

    if (FAILED(Ready_CutScene()))
        return E_FAIL;

    if (FAILED(Ready_Layer_Monster(TEXT("Layer_Monster"))))
        return E_FAIL;

    // 맵과 몬스터 배치가 끝난 뒤 컷신 오브젝트를 한 번만 복제해 숨겨 둔다.
    if (FAILED(Prepare_ChapterCutScenes(GAMEPLAY_START_CHAPTER)))
        return E_FAIL;

    if (FAILED(Ready_Layer_Fog(TEXT("Layer_Fog"))))
        return E_FAIL;

#ifdef _DEBUG

    if (FAILED(Ready_DebugRenderer()))
        return E_FAIL;

    if (FAILED(Ready_NavigationDebugRenderer()))
        return E_FAIL;

#endif //DEBUG

    //   m_pGameInstance->Play_BGM("AMB_SS_StreetStartPoint", 0.01f);

    //   m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_RESPAWN, this);
    //   m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_CHANGE_CHAPTER, this);
    //   m_pGameInstance->Subscribe(EVENT_TYPE::UPDATE_RESPAWN_POINT, this);
       //m_pGameInstance->Subscribe(EVENT_TYPE::TELEPORT_START, this);
    //   m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE, this);

    //Play_MapBGM(0.f);

    m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_RESPAWN, this);
    m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_CHANGE_CHAPTER, this);
    m_pGameInstance->Subscribe(EVENT_TYPE::UPDATE_RESPAWN_POINT, this);
    m_pGameInstance->Subscribe(EVENT_TYPE::TELEPORT_START, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE, this);
    m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, this);

    //Trigger 콜백은 MapTriggerManager로 이동
    //Contact 콜백 바인딩 
    m_pGameInstance->Bind_ContactCallback([this](const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 count)
        {
            m_ContactCallbackFunc(pairHeader, pairs, count);
        });

    //커서 중앙 
    m_pGameInstance->Set_CursorState(CURSOR_STATE::CENTER);

    return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
    if (m_pGameInstance->Get_CursorState() != CURSOR_STATE::FREE)
    {
        _bool bUIing = m_pGameInstance->Has_UIInputOwner();
        m_pGameInstance->Set_CursorState(bUIing ? CURSOR_STATE::CLIP : CURSOR_STATE::CENTER);
    }

    Update_ChapterState(fTimeDelta);

    if (nullptr != m_pCutSceneManager)
        m_pCutSceneManager->Update(fTimeDelta);

    //다음 챕터로 전환
    if (m_pGameInstance->Key_Down(DIK_6))
    {
        //      Respawn_Point Enter{};
              //Enter.iChapterIndex = (m_iCurrentChapter + 1) % g_iMaxChapter;
              //Enter.vPosition = g_ChapterPlayer_StartPos[Enter.iChapterIndex];
        //      Request_ChangeChapter(Enter.iChapterIndex, Enter);

        ChangeChapter_EventDesc Desc{};
        Desc.iChapterIndex = (m_iCurrentChapter + 1) % g_iMaxChapter;
        Desc.SpawnPoint.iChapterIndex = Desc.iChapterIndex;
        Desc.SpawnPoint.vPosition = g_ChapterPlayer_StartPos[Desc.iChapterIndex];

        m_pGameInstance->Broadcast(EVENT_TYPE::TELEPORT_BEGIN, &Desc);
    }
    //현재 맵에서 체크포인트로 리스폰 
    if (m_pGameInstance->Key_Down(DIK_7))
        Request_Respawn();

    __super::Update(fTimeDelta);


    static const _float4 Chapter02DebugSpawnPositions[6] =
    { { 116.961f,  145.460f, -165.77f, 1.f }, { 118.890f, 187.185f, -99.071f, 1.f },
    { 140.300f, 194.044f, -77.971f,  1.f }, { 155.420f, 200.855f, -92.294f, 1.f },
    { 145.601f, 215.237f, -87.175f,  1.f }, { -10.722f, 401.186f, -76.903f, 1.f } };

    // Chapter02에서 F1~F6 키로 디버그 이동 위치를 선택한다.
    if (2 == m_iCurrentChapter)
    {
        _int iDebugSpawnIndex = -1;

        if (m_pGameInstance->Key_Down(DIK_F6))
            iDebugSpawnIndex = 0;
        else if (m_pGameInstance->Key_Down(DIK_F7))
            iDebugSpawnIndex = 1;
        else if (m_pGameInstance->Key_Down(DIK_F8))
            iDebugSpawnIndex = 2;
        else if (m_pGameInstance->Key_Down(DIK_F9))
            iDebugSpawnIndex = 3;
        else if (m_pGameInstance->Key_Down(DIK_F10))
            iDebugSpawnIndex = 4;
        else if (m_pGameInstance->Key_Down(DIK_F11))
            iDebugSpawnIndex = 5;

        if (0 <= iDebugSpawnIndex)
        {
            CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

            if (nullptr != pPlayer && nullptr != m_pChapterMapManager)
            {
                Respawn_Point DebugSpawnPoint{};
                DebugSpawnPoint.iChapterIndex = m_iCurrentChapter;
                DebugSpawnPoint.vPosition = Chapter02DebugSpawnPositions[iDebugSpawnIndex];
                DebugSpawnPoint.fYaw = 0.f;

                // 플레이어의 Transform와 CCT 위치를 선택한 위치로 함께 변경한다.
                pPlayer->Teleport(DebugSpawnPoint);

                // 이동한 좌표를 기준으로 현재 섹터 상태를 갱신한다.
                m_pChapterMapManager->Refresh_CurrentSector(XMLoadFloat4(&DebugSpawnPoint.vPosition));
            }
        }
    }

#ifdef _DEBUG
    if (0 == m_iCurrentChapter && m_pGameInstance->Key_Down(DIK_F11))
    {
        CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

        if (nullptr != pPlayer && nullptr != m_pChapterMapManager)
        {
            Respawn_Point BossTriggerPoint{};
            BossTriggerPoint.iChapterIndex = m_iCurrentChapter;
            BossTriggerPoint.vPosition = { 154.9f, 59.1f, -291.4f, 1.f };
            BossTriggerPoint.fYaw = 0.f;

            // Transform와 CCT 위치를 함께 변경한다.
            pPlayer->Teleport(BossTriggerPoint);

            // 보스 섹터를 현재 섹터로 갱신하고 보스 시뮬레이션을 활성화한다.
            m_pChapterMapManager->Refresh_CurrentSector(XMLoadFloat4(&BossTriggerPoint.vPosition));
        }
    }

    if (0 == m_iCurrentChapter && m_pGameInstance->Key_Down(DIK_F10))
    {
        CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

        if (nullptr != pPlayer && nullptr != m_pChapterMapManager)
        {
            Respawn_Point BossTriggerPoint{};
            BossTriggerPoint.iChapterIndex = m_iCurrentChapter;
            BossTriggerPoint.vPosition = { 203.853f, 66.277f, -168.1f, 1.f };
            BossTriggerPoint.fYaw = 0.f;

            // Transform와 CCT 위치를 함께 변경한다.
            pPlayer->Teleport(BossTriggerPoint);

            // 호텔 문으로  현재 섹터로 갱신하고 보스 시뮬레이션을 활성화한다.
            m_pChapterMapManager->Refresh_CurrentSector(XMLoadFloat4(&BossTriggerPoint.vPosition));
        }
    }

   



    if (m_pGameInstance->Key_Down(DIK_8) && nullptr != m_pNavigationDebugRenderer)
        m_pNavigationDebugRenderer->Toggle_Enable();


    if (m_pGameInstance->Key_Down(DIK_COMMA))
        CAIObject::Toggle_TraceAnim();

    if (m_pGameInstance->Key_Down(DIK_PERIOD))
    {
        m_bDebugPaused = !m_bDebugPaused;

        if (true == m_bDebugPaused)
        {
            m_pGameInstance->Stop_MainTimer();
            OutputDebugStringW(L"[Debug] PAUSE\n");
        }
        else
        {
            m_pGameInstance->Start_MainTimer();
            OutputDebugStringW(L"[Debug] RESUME\n");
        }

        m_bDebugStep = false;
    }

    if (true == m_bDebugStep)
    {
        m_pGameInstance->Stop_MainTimer();
        m_bDebugStep = false;
    }
    else if (true == m_bDebugPaused && m_pGameInstance->Key_Down(DIK_SLASH))
    {
        m_pGameInstance->Start_MainTimer();
        m_bDebugStep = true;
    }

#endif

}

void CLevel_GamePlay::Late_Update(_float fTimeDelta)
{
    //Active시에만 MaterialBuffer 업데이트
    if (m_eChapterState == CHAPTER_STATE::ACTIVE)
        __super::Late_Update(fTimeDelta);

    if (nullptr == m_pChapterMapManager || false == m_pChapterMapManager->Is_ChapterLoaded())
        return;

    // MapController가 Instance 렌더 캐시를 만들기 전에 컷신이 대체할 원본만 잠시 제외한다.
    if (nullptr != m_pCutSceneManager)
        m_pCutSceneManager->Pre_MapUpdate();

    CCharacter* pPlayer = m_pGameInstance->Get_Player();
    const _float4* pCameraPosition = m_pGameInstance->Get_CamPosition();

    if (nullptr != pPlayer && nullptr != pPlayer->Get_Transform() && nullptr != pCameraPosition)
    {
        _vector vTrackingPosition = pPlayer->Get_Transform()->Get_State(STATE::POSITION);
        _vector vCameraPosition = XMLoadFloat4(pCameraPosition);
#ifdef _DEBUG
        const _float4* pCameraPosition = m_pGameInstance->Get_CamPosition();
        if (CAMERA_MODE::FREE == m_pGameInstance->Get_CameraMode() && nullptr != pCameraPosition)
            vTrackingPosition = XMLoadFloat4(pCameraPosition);
#endif // _DEBUG
        m_pChapterMapManager->Update(vTrackingPosition, vCameraPosition);
    }


    m_pChapterMapManager->Late_Update(fTimeDelta);

    if (nullptr != m_pCutSceneManager)
        m_pCutSceneManager->Post_MapUpdate();

#ifdef _DEBUG
    if (nullptr != m_pNavigationDebugRenderer)
    {
        m_pNavigationDebugRenderer->Update_DebugLines(m_pChapterMapManager);
        m_pNavigationDebugRenderer->Late_Update(fTimeDelta);
    }
#endif
}

HRESULT CLevel_GamePlay::Render()
{
    wchar_t szTitle[128];


    if (CHAPTER_STATE::ACTIVE == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d"), m_iFPS);
    }
    else if (CHAPTER_STATE::FADE_OUT == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | FADE_OUT"), m_iFPS);
    }
    else if (CHAPTER_STATE::UNLOAD == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | UNLOAD"), m_iFPS);
    }
    else if (CHAPTER_STATE::LOAD == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | LOAD"), m_iFPS);
    }
    else if (CHAPTER_STATE::LOAD_WAIT == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | LOAD_WAIT"), m_iFPS);
    }
    else if (CHAPTER_STATE::CREATE == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | CREATE"), m_iFPS);
    }
    else if (CHAPTER_STATE::RESETTING == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | RESETTING"), m_iFPS);
    }
    else if (CHAPTER_STATE::FADE_IN == m_eChapterState)
    {
        wsprintf(szTitle, TEXT("[ LevelGame ] FPS : %d | 전환 중 | FADE_IN"), m_iFPS);
    }

    SetWindowText(m_hWnd, szTitle);

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Lights()
{
    LIGHT_DESC      LightDesc{};

    LightDesc.eType = LIGHT::DIRECTIONAL;
    LightDesc.vDiffuse = _float4(0.1f, 0.1f, 0.1f, 0.1f);
    LightDesc.vAmbient = _float4(0.4f, 0.4f, 0.4f, 0.4f);
    LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);
    LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);

    if (FAILED(m_pGameInstance->Add_Light(LIGHT::DIRECTIONAL, LightDesc)))
        return E_FAIL;

    return S_OK;
}


HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _wstring& strLayerTag)
{

    //CCamera_Free::CAMERA_FREE_DESC      CameraDesc{};

    //CameraDesc.vEye = _float3(0.f, 3.f, -6.f);
    //CameraDesc.vAt = _float3(0.f, 0.f, 0.f);
    //CameraDesc.fFovy = XMConvertToRadians(60.f);
    //CameraDesc.fNear = 0.1f;
    //CameraDesc.fFar = 500.f;
    //CameraDesc.fSpeedPerSec = 20.f;
    //CameraDesc.fRotationPerSec = XMConvertToRadians(180.f);
    //CameraDesc.fMouseSensor = 0.02f;


    //if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_CameraFree"),
    //    ETOUI(LEVEL::GAMEPLAY), strLayerTag, &CameraDesc)))
    //    return E_FAIL;

    return S_OK;
}

HRESULT CLevel_GamePlay::Rendy_Layer_Sky(const _wstring& strLayerTag)
{
    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Sky"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bake_IBL()))
        return E_FAIL;


    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Character(const _wstring& strLayerTag)
{
    CTransform::TRANSFORM_DESC desc;

    desc.vTranslate = g_ChapterPlayer_StartPos[GAMEPLAY_START_CHAPTER];
    desc.fRotationPerSec = XMConvertToRadians(720.f);
    desc.fSpeedPerSec = 10.f;

    m_RespawnPoint.iChapterIndex = GAMEPLAY_START_CHAPTER;
    m_RespawnPoint.vPosition = g_ChapterPlayer_StartPos[GAMEPLAY_START_CHAPTER];

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
        ETOUI(LEVEL::GAMEPLAY), strLayerTag, &desc)))
        return E_FAIL;

    CCharacter* pPlayer = static_cast<CCharacter*>(m_pGameInstance->Get_Layer(ETOUI(LEVEL::GAMEPLAY), strLayerTag)->Get_Objects().front());
    m_pGameInstance->Set_Player(pPlayer);

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_EquipmentPreview(const _wstring& strLayerTag)
{
    CTransform::TRANSFORM_DESC BackgroundDesc{};
    BackgroundDesc.vScale = _float3(10.f, 10.f, 1.f);
    BackgroundDesc.vTranslate = _float4(0.f, -999.f, 1.f, 1.f);
    BackgroundDesc.vRotationDegree = _float3(0.f, 0.f, 0.f);
    BackgroundDesc.fRotationPerSec = 0.f;
    BackgroundDesc.fSpeedPerSec = 0.f;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_EquipmentPreviewBackground"), ETOUI(LEVEL::GAMEPLAY), strLayerTag, &BackgroundDesc)))
        return E_FAIL;

    CTransform::TRANSFORM_DESC CharacterDesc{};
    CharacterDesc.vTranslate = _float4(0.f, -1000.f, 0.f, 1.f);
    CharacterDesc.vRotationDegree = _float3(0.f, 180.f, 0.f);
    CharacterDesc.fRotationPerSec = XMConvertToRadians(90.f);
    CharacterDesc.fSpeedPerSec = 0.f;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_EquipmentPreviewCharacter"), ETOUI(LEVEL::GAMEPLAY), strLayerTag, &CharacterDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Monster(const _wstring& strLayerTag)
{
    if (nullptr != m_pChapterMapManager)
    {
        if (FAILED(m_pChapterMapManager->Spawn_All()))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Map(const _wstring& strLayerTag)
{
    CHAPTER_DESC ChapterDesc{};
    _wstring strError{};

    m_pChapterMapManager = CChapterMapManager::Create(m_pDevice, m_pContext);

    if (nullptr == m_pChapterMapManager)
        return E_FAIL;

    //const _wstring strChapterPath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\" + GAMEPLAY_CHAPTER_FILE; 

    _wstring strChapterPath = {};
    if (FAILED(CChapterMapManager::Get_ChapterPath(GAMEPLAY_START_CHAPTER, strChapterPath))) return E_FAIL;

    if (FAILED(m_pChapterMapManager->Load_Chapter(strChapterPath, LEVEL::GAMEPLAY, Chapter_To_Level(GAMEPLAY_START_CHAPTER))))
        return E_FAIL; 

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Fog(const _wstring& strLayerTag)
{
    /* {
         CFogVolume::FOGVOLUME_DESC Desc{};
         Desc.vTranslate = { 97.f, 40.5f, -557.f, 1.f };
         if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY),
             TEXT("Prototype_GameObject_FogVolume"),
             ETOUI(LEVEL::GAMEPLAY), strLayerTag, &Desc)))
             return E_FAIL;
     }

     {
         CFogVolume::FOGVOLUME_DESC Desc{};
         Desc.vTranslate = { 162.2f, 56.5f, -272.7f, 1.f };
         if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY),
             TEXT("Prototype_GameObject_FogVolume"),
             ETOUI(LEVEL::GAMEPLAY), strLayerTag, &Desc)))
             return E_FAIL;
     }*/




    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Effect()
{
    const string strRoot = string(RESOURCE_DIR) + "\\Effects\\";

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Perfect_Guard", strRoot + "Player_Perfect_Guard.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Repair", strRoot + "Player_Repair.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Heal", strRoot + "Player_Heal.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Heal_Body", strRoot + "Player_Heal_Body.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Heal_Hand", strRoot + "Player_Heal_Hand.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Normal_Guard", strRoot + "Player_Normal_Guard.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Attack_Swing", strRoot + "Player_Hit.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Attack_Thrust", strRoot + "Player_Hit_2.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Normal_Trail", strRoot + "Player_Normal_Trail.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_TwoHand_Pable_Trail_1", strRoot + "Player_Pable_Trail.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_TwoHand_Pable_Trail_2", strRoot + "Player_Pable_Trail_2.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Pable_Trail_3", strRoot + "Player_Pable_Trail_3.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Take_Damage", strRoot + "Player_Take_Damage.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Dead", strRoot + "Player_Dead.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Groggy_Attack", strRoot + "Player_Groggy_Attack.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Fatal_Attack", strRoot + "Player_Fatal_Attack.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Turn_On_Lamp", strRoot + "Player_Turn_On_Lamp.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Respawn", strRoot + "Player_Respawn.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Test_Bomb_Effect", strRoot + "Test_Bomb_Effect.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Player_ChargeGun_BombFlare", strRoot + "Player_ChargeGun_Bomb.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Player_ChargeGun_ArmSmoke", strRoot + "Player_ChargeGun_ArmSmoke.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Player_ChargeGun_ArmFlare", strRoot + "Player_ChargeGun_ArmFlare.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Player_Flare_Spark", strRoot + "Player_Flare_Spark.json")))
        return E_FAIL;
    
    if (FAILED(m_pGameInstance->Load_Effect_File("Player_GrappleArm_Hit", strRoot + "Player_GrappleArm_Hit.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Player_GrappleArn_Wire_Hit", strRoot + "Player_GrappleArn_Wire_Hit.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Scissor_Buff", strRoot + "Player_Scissor_Buff.json")))
        return E_FAIL;
    
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Electric", strRoot + "Test_Pino_Lightning.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Player_Scissor_Spark", strRoot + "Player_Scissor_Spark.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Common_Touch_Fire", strRoot + "Common_Touch_Fire.json")))
        return E_FAIL;


#pragma region [COMMON EFFECT]

    // 챕터 맵에 배치할 공용 화염 이펙트 원본을 등록한다.
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Common_Fire", strRoot + "Common_Fire.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Common_Spark", strRoot + "Common_Spark.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Fatal_Symbol", strRoot + "Common_Fatal_Symbol.json")))
        return E_FAIL;

#pragma endregion

    /* Monster */
#pragma region [ MONSTER EFFECT ]
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Shotgun_Muzzle", strRoot + "Monster_Shotgun_Muzzle.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bullet_Trail", strRoot + "Monster_Bullet_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Monster_Trail", strRoot + "Monster_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Fury_Attack_Model", strRoot + "Common_Fury_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Punch", strRoot + "Boss_Attack_Punch.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Attack_Smoke", strRoot + "Boss_Attack_Smoke.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Dash", strRoot + "Boss_Dash.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Fire", strRoot + "Boss_Head_Fire.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Smoke", strRoot + "Boss_Smoke.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Furry", strRoot + "Boss_Fury_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Stone", strRoot + "Boss_Attack_Stone.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Slide", strRoot + "Boss_Attack_Slide2.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Boss_Slide2", strRoot + "Boss_Attack_Slide.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Monster_Fury", strRoot + "Monster_Fury_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Mine", strRoot + "Raxasia_Thunder_Mine.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Sector", strRoot + "Raxasia_Explosion_Sphere.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Range", strRoot + "Raxasia_Explosion_Sphere.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Weapon_Spark", strRoot + "Raxasia_Sword_Spark.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Missile_Trail", strRoot + "Test_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_ButterFly_Attack", strRoot + "Test_Butterfly.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_ButterFly_Move", strRoot + "Test_Butterfly_Move.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_ButterFly_Dead", strRoot + "Test_Butterfly_Dead.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Priest_Weapon_Thunder", strRoot + "Priest_Weapon_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Charge_Thunder", strRoot + "Raxasia_Charge_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Charge", strRoot + "Raxasia_Charge.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Groggy_Attack", strRoot + "Raxasia_Groggy_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Sword_Thunder", strRoot + "Raxasia_Sword_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Thunder_Trail", strRoot + "Raxasia_Thunder_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Normal_Trail", strRoot + "Raxasia_Normal_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Common_Fury_Attack", strRoot + "Common_Fury_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Ground_Attack", strRoot + "Raxasia_Ground_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Attack", strRoot + "Raxasia_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Sting_Thunder", strRoot + "Raxasia_Sting_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_P2_Charge", strRoot + "Raxasia_P2_Charge.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Body_Thunder", strRoot + "Raxasia_P2_Body_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Shield_Attack", strRoot + "Raxasia_Shield_Attack.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Run_Trail", strRoot + "Raxasia_Run_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Charge_Spark", strRoot + "Raxasia_Charge_Spark.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Glow", strRoot + "Raxasia_Glow.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Start_1", strRoot + "Raxasia_P2_Start_1.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Start_2", strRoot + "Raxasia_P2_Start_2.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Dive_Strike", strRoot + "Raxasia_P2_Dive_Strike.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Sky_Smoke", strRoot + "Raxasia_P2_Sky_Smoke.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Circle_Thunder", strRoot + "Raxasia_P2_Circle_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_ElectricBall", strRoot + "Raxasia_ElectricBall.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_ElectricBall_Reflect", strRoot + "Raxasia_ElectricBall_Reflect.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Projectile_Reflect_Trail", strRoot + "Raxasia_Projectile_Reflect_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Projectile_Trail", strRoot + "Raxasia_Projectile_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_ElectricBall_Reflect_Success", strRoot + "Raxasia_P2_Projectile_Guard.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Projectile_Shoot", strRoot + "Raxasia_P2_Projectile_Shoot.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_Sky_Thunder", strRoot + "Raxasia_P2_Sky_Thunder.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_GroundBall", strRoot + "Raxasia_GroundBall.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Raxasia_GroundBall_Trail", strRoot + "Raxasia_GroundBall_Trail.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Dog_Stone", strRoot + "Dog_Stone.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Balista_Fire", strRoot + "Balista_Explosion.json")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Balista_Muzzle", strRoot + "Balista_Muzzle.json")))
        return E_FAIL;
#pragma endregion

#pragma region [ ENVIRONMENT OBJECT ]
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Stargazer_Wait", strRoot + "Stargazer_Wait.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Stargazer_Activate", strRoot + "Stargazer_Activate.json")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Stargazer_Idle", strRoot + "Stargazer_Idle.json")))
        return E_FAIL;

#pragma region [ TEST EFFECT ]
    if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Test", strRoot + "Raxasia_Thunder_Mine.json")))
        return E_FAIL;
#pragma endregion

    return S_OK;
}

HRESULT CLevel_GamePlay::Ready_UI()
{
    return CUIFactory::Ready_GameplayHUDs();
}

HRESULT CLevel_GamePlay::Ready_CutScene()
{
    // Level은 맵 전환 상태와 Player 입력 제어만 제공하고 컷신 이벤트와 재생은 매니저가 소유한다.
    m_pCutSceneManager = CCutScene_Manager::Create(m_pDevice, m_pContext, [this]() -> _bool { return false == Is_Transitioning(); }, [this](_bool bPlaying) { Lock_PlayerInput(bPlaying); Freeze_Monsters(bPlaying); }, [this](const string& strAssetName) -> _bool { return Handle_CutSceneFinished(strAssetName); });

    if (nullptr == m_pCutSceneManager)
        return E_FAIL;

    // 현재 Chapter 데이터에 선언된 컷신을 플레이 진입 전에 읽어 Trigger 프레임의 동기 파싱을 없앤다.
    return Preload_ChapterCutScenes(GAMEPLAY_START_CHAPTER);
}

HRESULT CLevel_GamePlay::Preload_ChapterCutScenes(_uint iChapterIndex)
{
    if (nullptr == m_pCutSceneManager || nullptr == m_pChapterMapManager || iChapterIndex >= g_iMaxChapter)
        return E_FAIL;

    m_pCutSceneManager->Clear_Cache();

    vector<string> AssetNames = {};
    // Chapter Trigger가 실제로 요청하는 컷신만 진입 시점에 미리 읽는다.
    m_pChapterMapManager->Get_CutSceneAssetNames(&AssetNames);

    for (const string& strAssetName : AssetNames)
    {
        if (FAILED(m_pCutSceneManager->Preload(strAssetName)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CLevel_GamePlay::Prepare_ChapterCutScenes(_uint iChapterIndex)
{
    if (nullptr == m_pCutSceneManager || iChapterIndex >= g_iMaxChapter)
        return E_FAIL;

    return m_pCutSceneManager->Prepare_Chapter(ETOUI(LEVEL::GAMEPLAY), ETOUI(Chapter_To_Level(iChapterIndex)));
}

_bool CLevel_GamePlay::Handle_CutSceneFinished(const string& strAssetName)
{
    // 컷신 모델이 정리된 직후 미리 배치된 실제 보스를 깨워 바로 전투를 시작한다.
    // Chapter01의 BOSS_FESTIVAL Spawn 위치는 Fguide 마지막 보스 위치와 맞춰져 있다.
    const SPAWN_MONSTER_TYPE eBossType = Resolve_CutSceneBossType(strAssetName);

    if (SPAWN_MONSTER_TYPE::END != eBossType)
    {
        if (SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2 == eBossType)
        {
            CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

            if (nullptr != pPlayer)
            {
                Respawn_Point Phase2StartPoint = {};
                Phase2StartPoint.iChapterIndex = m_iCurrentChapter;
                Phase2StartPoint.vPosition = { 3.682f, 401.500f, -54.868f, 1.f };
                Phase2StartPoint.fYaw = XMConvertToRadians(36.217f);

                // P2 전투 시작 전에 Player Transform과 CCT 발 위치를 지정 좌표로 함께 이동한다.
                pPlayer->Teleport(Phase2StartPoint);

                if (nullptr != m_pChapterMapManager)
                    m_pChapterMapManager->Refresh_CurrentSector(XMLoadFloat4(&Phase2StartPoint.vPosition));
            }
        }

        BossClear_EventDesc StartDesc{};
        StartDesc.eMonsterType = eBossType;

        m_pGameInstance->Broadcast(EVENT_TYPE::BOSS_START, &StartDesc);
        return false;
    }

    if (CUTSCENE_RAXASIA_DEAD == strAssetName)
    {
        // P2가 풀 반환을 마친 뒤 직접 최종 BOSS_CLEAR를 발생시키도록 완료 시점만 전달한다.
        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::END_CUTSCENE_RAXASIA_DEAD, nullptr);
        return false;
    }

    // 호텔 진입 컷신이 끝나면 게임 카메라로 복귀하지 않고 마지막 컷신 카메라를 유지한 채 Hotel Chapter로 전환한다.
    if ("Hotel_BI" == strAssetName)
    {
        constexpr _uint iHotelChapterIndex = { 1 };
        Respawn_Point HotelSpawnPoint = {};
        HotelSpawnPoint.iChapterIndex = iHotelChapterIndex;
        HotelSpawnPoint.vPosition = g_ChapterPlayer_StartPos[iHotelChapterIndex];
        Request_ChangeChapter(iHotelChapterIndex, HotelSpawnPoint);
        return Is_Transitioning();
    }

    // 컷신 BGM이 EndTime에서 정지된 채로 끝나므로 보스도 Chapter 전환도 없으면 맵 BGM으로 되돌린다.
    //Play_MapBGM(MAP_BGM_FADE_SEC);

    return false;
}

//Test용 
//특정 오브젝트(별바라기..) 등에서 Trigger 생성하면서 아래처럼 함수 바인딩 하기 
//HRESULT CLevel_GamePlay::Ready_Trigger()
//{
//    CTrigger::TRIGGER_DESC Desc{};
//    Desc.vTranslate = { 0.f,1.f, 5.f, 1.f};             // 위치 
//	//Desc.vRotationDegree = { 0.f, 0.f, 0.f };         // 회전
//    Desc.vHalfExtents = { 1.f, 1.f, 1.f };              // Shape Box 크기 
//	Desc.bOnceTrigger = false;                          // true : beginOverlap 한 번만 발동 그 뒤에는 충돌 X, false : 계속 발동
//	Desc.iTargetFilter = ETOUI(FILTER_GROUP::PLAYER );   // 충돌 대상 필터링
//    Desc.OnBeginOverlap = [this](CGameObject* pOther)
//    {
//        if (nullptr == pOther)
//            return;
//        //기능 정의 
//
//	};
//
//    Desc.OnEndOverlap = [this](CGameObject* pOther)
//    {
//        if (nullptr == pOther) 
//            return;
//        //기능 정의 
//
//    };
//
//    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Trigger"),
//        ETOUI(LEVEL::GAMEPLAY), L"Layer_Trigger", &Desc)))
//        return E_FAIL;
//
//    return S_OK;
//}

void CLevel_GamePlay::m_TriggerCallbackFunc(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; i++)
    {

        if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            continue;

        PxActor* pTriggerActor = pairs[i].triggerActor;
        PxActor* pOtherActor = pairs[i].otherActor;

        if (!pTriggerActor || !pOtherActor) continue;

        PxFilterData triggerFilter = pairs[i].triggerShape->getSimulationFilterData();
        PxFilterData otherFilter = pairs[i].otherShape->getSimulationFilterData();

        CGameObject* pTriggerOwner = static_cast<CGameObject*>(pTriggerActor->userData);
        CGameObject* pOtherOwner = static_cast<CGameObject*>(pOtherActor->userData);

        if (!pTriggerOwner || !pOtherOwner) continue;

        //Begin Overlap 
        if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            pTriggerOwner->Begin_Overlap(pOtherOwner);
        }
        //End Overlap
        else if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            pTriggerOwner->End_Overlap(pOtherOwner);
        }

    }
}

void CLevel_GamePlay::m_ContactCallbackFunc(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 iCount)
{
    if (nullptr == pairs)
        return;

    //이미 삭제된 Actor에 대한 Contact는 무시함
    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1)) return;

    PxActor* pActor0 = pairHeader.actors[0];
    PxActor* pActor1 = pairHeader.actors[1];
    if (!pActor0 || !pActor1)
        return;

    CGameObject* pObj0 = static_cast<CGameObject*>(pActor0->userData);
    CGameObject* pObj1 = static_cast<CGameObject*>(pActor1->userData);

    for (PxU32 i = 0; i < iCount; i++)
    {
        const PxContactPair& Pair = pairs[i];
        //충돌 시작만 처리함 
        if (Pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            _float3 vNormal = { 0.f, 1.f, 0.f };
            PxContactPairPoint ContactPoint;

            //충돌 Point 정보 추출
            if (Pair.extractContacts(&ContactPoint, 1) > 0)
            {
                vNormal = { ContactPoint.normal.x, ContactPoint.normal.y, ContactPoint.normal.z };
            }

            if (pObj0) pObj0->Begin_Contact(pObj1, XMLoadFloat3(&vNormal));
            if (pObj1) pObj1->Begin_Contact(pObj0, XMVectorSet(-vNormal.x, -vNormal.y, -vNormal.z, 0.f));

        }

    }
}

void CLevel_GamePlay::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
    switch (eType)
    {
    case EVENT_TYPE::REQUEST_RESPAWN:
        Request_Respawn();
        break;
    case EVENT_TYPE::REQUEST_CHANGE_CHAPTER:
        if (pData != nullptr)
        {
            ChangeChapter_EventDesc* pChapterDesc = static_cast<ChangeChapter_EventDesc*>(pData);
            Request_ChangeChapter(pChapterDesc->iChapterIndex, pChapterDesc->SpawnPoint);
        }
        break;
    case EVENT_TYPE::UPDATE_RESPAWN_POINT:
        if (pData != nullptr)
        {
            RespawnPoint_EventDesc* pRespawnDesc = static_cast<RespawnPoint_EventDesc*>(pData);
            Respawn_Point Point = pRespawnDesc->SpawnPoint;
            Point.iChapterIndex = m_iCurrentChapter; // 현재 챕터에 맞게 iChapterIndex를 설정
            Set_RespawnPoint(Point);
        }
        break;
    case EVENT_TYPE::TELEPORT_START:
        if (pData != nullptr)
        {
            ChangeChapter_EventDesc* pChapterDesc = static_cast<ChangeChapter_EventDesc*>(pData);

            // 같은 Chapter는 이미 로드된 현재 Sector 안에서 Player만 직접 이동한다.
            if (pChapterDesc->iChapterIndex == m_iCurrentChapter)
            {
                CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

                if (nullptr != pPlayer)
                {
                    pPlayer->Teleport(pChapterDesc->SpawnPoint);

                    // Teleport_Start 이후 기본 상태로 복귀시킨다.
                    m_pGameInstance->Broadcast(EVENT_TYPE::START_CHAPTER, nullptr);
                }

                Lock_PlayerInput(false);
                break;
            }

            Request_ChangeChapter(pChapterDesc->iChapterIndex, pChapterDesc->SpawnPoint);

            // 요청 실패 시 입력 잠금을 해제한다.
            if (false == Is_Transitioning())
                Lock_PlayerInput(false);
        }
        break;
    }

}

void CLevel_GamePlay::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR == eType)
    {
        // 보스는 자기 전투곡만 틀고, 복귀는 레벨이 챕터 테이블로 처리한다.
        //Play_MapBGM(MAP_BGM_FADE_SEC);
        return;
    }

    if (EVENT_TYPE_MAP_OBJECT::STARGAZER_TRAVEL_EXECUTE != eType || nullptr == pData)
        return;

    if (false == Can_RequestTransition() || nullptr == m_pChapterMapManager)
        return;

    STARGAZER_TRAVEL_EXECUTE_DESC* pTravelDesc = static_cast<STARGAZER_TRAVEL_EXECUTE_DESC*>(pData);
    Respawn_Point TargetPoint = {};
    const _uint iCurrentSectorID = m_pChapterMapManager->Get_CurrentSectorID();

    // UI 목록을 신뢰하지 않고 실제 이동 직전에 Runtime 상태를 다시 검증한다.
    if (FAILED(m_pChapterMapManager->Resolve_StargazerTravel(pTravelDesc->iTargetStargazerID, m_iCurrentChapter, iCurrentSectorID, &TargetPoint)))
        return;

    ChangeChapter_EventDesc ChangeDesc = {};
    ChangeDesc.iChapterIndex = TargetPoint.iChapterIndex;
    ChangeDesc.SpawnPoint = TargetPoint;

    // 기존 Player Teleport 연출과 Chapter 전환 흐름을 그대로 사용한다.
    // (이하 라이브 원본 그대로)

    //Request_ChangeChapter(ChangeDesc.iChapterIndex, ChangeDesc.SpawnPoint);
    m_pGameInstance->Broadcast(EVENT_TYPE::TELEPORT_BEGIN, &ChangeDesc);

}

void CLevel_GamePlay::Level_Exit()
{
    // Release_Chapter가 재생 중인 컷신까지 정리해 카메라와 화면 상태를 남기지 않는다.
    if (nullptr != m_pCutSceneManager)
        m_pCutSceneManager->Release_Chapter();

    Close_LoadingUI();

    if (nullptr != m_pChapterLoader)
    {
        Safe_Release(m_pChapterLoader);
        m_pChapterLoader = nullptr;
    }

    if (m_pChapterMapManager != nullptr)
        m_pChapterMapManager->Unload_Chapter();

    //모든 챕터 리소스 해제 
    for (_uint i = 0; i < g_iMaxChapter; ++i)
        m_pGameInstance->Clear_Resources(ETOI(LEVEL::CHAP_01) + i);

    CLoader::Clear_CachedChapters();
}

void CLevel_GamePlay::Request_ChangeChapter(_uint iChapterIndex, const Respawn_Point& SpawnPoint)
{
    //전환 중 요청은 무시 
    if (Can_RequestTransition() == false) return;

    if (iChapterIndex >= g_iMaxChapter || iChapterIndex == m_iCurrentChapter) return;

    Begin_FadeOut(TRANSITION_KIND::CHANGE_CHAPTER);

    m_iPendingChapter = iChapterIndex;
    m_PendingSpawnPoint = SpawnPoint;

}

void CLevel_GamePlay::Request_Respawn()
{
    //전환 중 요청은 무시 
    if (Can_RequestTransition() == false) return;

    // 추가
    // 같은 챕터 리스폰도 로딩 화면을 먼저 보여주도록
    if (FAILED(Open_LoadingUI()))
        OutputDebugStringW(L"[Respawn] Failed to open loading UI.\n");

    Begin_FadeOut(TRANSITION_KIND::RESPAWN);
}

void CLevel_GamePlay::Update_ChapterState(_float fTimeDelta)
{
    switch (m_eChapterState)
    {
    case CHAPTER_STATE::ACTIVE:		return;
    case CHAPTER_STATE::FADE_OUT:	State_FadeOut(fTimeDelta);	break;
    case CHAPTER_STATE::UNLOAD:		State_Unload();				break;
    case CHAPTER_STATE::LOAD:		State_Load();				break;
    case CHAPTER_STATE::LOAD_WAIT:	State_LoadWait();			break;
    case CHAPTER_STATE::CREATE:		State_Create();				break;
    case CHAPTER_STATE::RESETTING:	State_Resetting();			break;
    case CHAPTER_STATE::FADE_IN:	State_FadeIn(fTimeDelta);	break;
    }
}


void CLevel_GamePlay::State_FadeOut(_float fTimeDelta)
{
    m_fFadeTimer -= fTimeDelta;
    if (m_fFadeTimer > 0.f) return;

    //월드 정지 
    m_pGameInstance->Stop_MainTimer();

    //리스폰 
    if (TRANSITION_KIND::RESPAWN == m_eTransitionKind)
    {
        //m_RespawnPoint 가 있고, 현재 챕터와 동일한 챕터라면 로딩 필요없고 바로 리셋
        if (m_RespawnPoint.iChapterIndex == m_iCurrentChapter)
        {
            m_eChapterState = CHAPTER_STATE::RESETTING;
        }
        else
        {
            m_iPendingChapter = m_RespawnPoint.iChapterIndex;
            m_PendingSpawnPoint = m_RespawnPoint;
            m_eChapterState = CHAPTER_STATE::UNLOAD;
        }
    }
    //다른 맵 전환 
    else
    {
        m_eChapterState = CHAPTER_STATE::UNLOAD;
    }
}

void CLevel_GamePlay::State_Unload()
{
    // 컷신 객체가 참조하는 챕터 모델과 맵 Instance를 맵보다 먼저 해제한다.
    if (nullptr != m_pCutSceneManager)
        m_pCutSceneManager->Release_Chapter();

    m_pChapterMapManager->Unload_Chapter();
    m_eChapterState = CHAPTER_STATE::LOAD;
    m_pGameInstance->Invalidate_AllControllerCache();
}

void CLevel_GamePlay::State_Load()
{
    if (FAILED(Open_LoadingUI()))
        OutputDebugStringW(L"[Chapter Loading] Failed to open loading UI.\n");

    if (CLoader::Is_ChapterCached(m_iPendingChapter))
    {
        m_eChapterState = CHAPTER_STATE::CREATE; // 재방문이면 로딩 생략
    }
    else
    {
        m_pChapterLoader = CLoader::Create(m_pDevice, m_pContext, m_iPendingChapter, m_hWnd);
        if (m_pChapterLoader == nullptr)
        {
            MSG_BOX("Level Gameplay : Chapter Loading Failed");
            m_iPendingChapter = m_iCurrentChapter;
            m_eChapterState = CHAPTER_STATE::CREATE; //실패시 기존 레벨로 복귀
            return;
        }

        m_eChapterState = CHAPTER_STATE::LOAD_WAIT;
    }
}

void CLevel_GamePlay::State_LoadWait()
{
    if (m_pChapterLoader == nullptr) return;

    // 진행률
    CUILoadingController* pLoadingController = static_cast<CUILoadingController*>(m_pGameInstance->Find_UIController(L"LoadingController"));
    if (pLoadingController != nullptr)
        pLoadingController->Set_LoadingProgress(m_pChapterLoader->Get_LoadingProgress());

    if (m_pChapterLoader->isFinished() == false) return;

    if (pLoadingController != nullptr)
        pLoadingController->Set_LoadingProgress(1.f);

    _bool bFailed = m_pChapterLoader->isFailed();

    Safe_Release(m_pChapterLoader);
    m_pChapterLoader = nullptr;

    if (bFailed)
    {
        MSG_BOX("Level Gameplay : Chapter Loading Failed");
        m_iPendingChapter = m_iCurrentChapter;
    }

    m_eChapterState = CHAPTER_STATE::CREATE;

}

void CLevel_GamePlay::State_Create()
{
    _wstring strChapterPath;
    if (FAILED(CChapterMapManager::Get_ChapterPath(m_iPendingChapter, strChapterPath)))
    {
        MSG_BOX("Level Gameplay : Chapter Path Not Found");
        Close_LoadingUI();
        Begin_FadeIn();
        return;
    }

    if (FAILED(m_pChapterMapManager->Load_Chapter(strChapterPath, LEVEL::GAMEPLAY, Chapter_To_Level(m_iPendingChapter))))
    {
        MSG_BOX("Level Gameplay : Chapter Loading Failed");

        if (m_iPendingChapter != m_iCurrentChapter)
        {
            m_iPendingChapter = m_iCurrentChapter;
            return;
        }
        Begin_FadeIn();
        return;
    }

    if (FAILED(Preload_ChapterCutScenes(m_iPendingChapter)))
    {
        MSG_BOX("Level Gameplay : CutScene Preload Failed");
        Begin_FadeIn();
        return;
    }

    if (FAILED(m_pChapterMapManager->Spawn_All()))
    {
        MSG_BOX("Level Gameplay : Chapter Object Spawn Failed");
        Begin_FadeIn();
        return;
    }

    if (FAILED(Prepare_ChapterCutScenes(m_iPendingChapter)))
    {
        MSG_BOX("Level Gameplay : CutScene Object Prepare Failed");
        Begin_FadeIn();
        return;
    }

    m_pGameInstance->Update_MaterialBuffer();

    //챕터 전환용 초기화
    ResetPlayer_ForChangeMap(m_PendingSpawnPoint);
    //Clear가 lock을 풀어서 재잠금
    Lock_PlayerInput(true);

    //체크포인트를 챕터 변경 시점에 갱신
    m_RespawnPoint = m_PendingSpawnPoint;
    m_iCurrentChapter = m_iPendingChapter;
    m_iPendingChapter = UINT_MAX;

    Close_LoadingUI();

    Begin_FadeIn();

}

void CLevel_GamePlay::State_Resetting()
{
    Respawn_Player(m_RespawnPoint);
    Lock_PlayerInput(true);

    m_pChapterMapManager->Reset_Chapter();
    m_pGameInstance->Invalidate_AllControllerCache();

    // 추가
    // 같은 챕터 리스폰 처리가 완료됐으므로 로딩 UI를 닫는다.
    Close_LoadingUI();

    Begin_FadeIn();
}

void CLevel_GamePlay::State_FadeIn(_float fTimeDelta)
{
    m_fFadeTimer -= fTimeDelta;
    if (m_fFadeTimer > 0.f) return;

    Lock_PlayerInput(false);

    m_eTransitionKind = TRANSITION_KIND::NONE;
    m_eChapterState = CHAPTER_STATE::ACTIVE;

}

void CLevel_GamePlay::Begin_FadeOut(TRANSITION_KIND eKind)
{
    // 챕터 전환 페이드와 컷신 페이드가 겹치지 않도록 컷신 생명주기를 먼저 종료한다.
    if (nullptr != m_pCutSceneManager && m_pCutSceneManager->Is_Playing())
        m_pCutSceneManager->Stop();

    // 리스폰과 챕터 전환 모두 전투곡을 여기서 끊고 FadeIn에서 그 Chapter의 맵 BGM을 다시 시작한다.
    m_pGameInstance->Stop_BGM();

    m_eTransitionKind = eKind;
    m_eChapterState = CHAPTER_STATE::FADE_OUT;
    m_fFadeTimer = FADE_DURATION;

    Lock_PlayerInput(true);
}

void CLevel_GamePlay::Begin_FadeIn()
{
    m_pGameInstance->Start_MainTimer();
    m_eChapterState = CHAPTER_STATE::FADE_IN;
    m_fFadeTimer = FADE_DURATION;

    // m_iCurrentChapter가 갱신된 뒤라 전환 후 Chapter의 BGM이 잡힌다.
    //Play_MapBGM(MAP_BGM_FADE_SEC);

    m_pGameInstance->Broadcast(EVENT_TYPE::START_CHAPTER, nullptr);
}

void CLevel_GamePlay::Play_MapBGM(_float fFadeSec)
{
    const char* pMapBGMKey = Resolve_MapBGMKey(m_iCurrentChapter);

    if (nullptr == pMapBGMKey)
    {
        m_pGameInstance->Stop_BGM();
        return;
    }

    m_pGameInstance->Play_BGM(pMapBGMKey, MAP_BGM_VOLUME, fFadeSec);
}

void CLevel_GamePlay::Respawn_Player(const Respawn_Point& Point)
{
    CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
    pPlayer->Respawn(Point);

    _vector vSpawnPosition = XMLoadFloat4(&Point.vPosition);

    if (m_pChapterMapManager)
        m_pChapterMapManager->Refresh_CurrentSector(vSpawnPosition);

}

void CLevel_GamePlay::ResetPlayer_ForChangeMap(const Respawn_Point& Point)
{
    CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
    pPlayer->Reset_ForChangeMap(Point);

    _vector vSpawnPosition = XMLoadFloat4(&Point.vPosition);

    if (m_pChapterMapManager)
        m_pChapterMapManager->Refresh_CurrentSector(vSpawnPosition);
}

void CLevel_GamePlay::Lock_PlayerInput(_bool bLock)
{
    CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
    if (nullptr == pPlayer)
        return;

    pPlayer->Get_StatusComponent()->Set_InputLock(bLock);
    pPlayer->Get_StatusComponent()->Set_CamMoveLock(bLock);
    pPlayer->Get_StatusComponent()->Set_RotationLock(bLock);
}

void CLevel_GamePlay::Freeze_Monsters(_bool bFreeze)
{
    // 컷신 재생 중에는 BT 판단을 멈춰 추격과 공격을 막는다. Animator는 계속 돌아 현재 포즈를 유지한다.
    CLayer* pLayer = m_pGameInstance->Get_Layer(ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Monster"));

    if (nullptr == pLayer)
        return;

    for (CGameObject* pObject : pLayer->Get_Objects())
    {
        CAIObject* pMonster = dynamic_cast<CAIObject*>(pObject);

        if (nullptr == pMonster || true == pMonster->Is_InActive())
            continue;

        pMonster->Set_Freeze(bFreeze);
    }
}

HRESULT CLevel_GamePlay::Open_LoadingUI()
{
    if (m_hLoadingUI.Is_Valid() && m_pGameInstance->Is_OpenUIObject(m_hLoadingUI))
        return S_OK;

    // 최초 로딩에서 만든 External 싱글톤을 다시 열고 핸들을 받는다.
    if (FAILED(CUIFactory::Ready_LoadingUI(&m_hLoadingUI)))
        return E_FAIL;

    CUILoadingController* pLoadingController = static_cast<CUILoadingController*>(m_pGameInstance->Find_UIController(L"LoadingController"));

    if (pLoadingController != nullptr)
        pLoadingController->Begin_LoadingSession();

    return S_OK;
}

void CLevel_GamePlay::Close_LoadingUI()
{
    if (!m_hLoadingUI.Is_Valid())
        return;

    if (m_pGameInstance->Is_OpenUIObject(m_hLoadingUI))
        m_pGameInstance->Close_UIObject(m_hLoadingUI);

    // UI 인스턴스는 유지하고 Level_GamePlay가 보관한 핸들 값만 비운다.
    m_hLoadingUI.Invalidate();
}

#ifdef _DEBUG

HRESULT CLevel_GamePlay::Ready_DebugRenderer()
{
    if (FAILED(m_pGameInstance->Create_VisualDebugger(m_pDevice, m_pContext)))
        return E_FAIL;
    return S_OK;
}

void CLevel_GamePlay::Render_DebugWindow()
{

}

HRESULT CLevel_GamePlay::Ready_NavigationDebugRenderer()
{
    m_pNavigationDebugRenderer = CNavigationDebugRenderer::Create(m_pDevice, m_pContext);

    if (nullptr == m_pNavigationDebugRenderer)
        return E_FAIL;

    return S_OK;
}

#endif // DEBUG



CLevel_GamePlay* CLevel_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
{
    CLevel_GamePlay* pInstance = new CLevel_GamePlay(pDevice, pContext, hWnd);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CLevel_GamePlay");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLevel_GamePlay::Free()
{
    m_pGameInstance->UnSubscribe_AllEvent(this);

    m_pGameInstance->Bind_TriggerCallback(nullptr);
    m_pGameInstance->Bind_ContactCallback(nullptr);
#ifdef _DEBUG
    Safe_Release(m_pNavigationDebugRenderer);
#endif

    Safe_Release(m_pChapterLoader);
    Safe_Release(m_pCutSceneManager);
    Safe_Release(m_pChapterMapManager);
    __super::Free();
}

