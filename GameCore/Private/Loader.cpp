#include "Loader.h"
#include "GameInstance.h"
#include "Camera_Free.h"
#include "TestObject.h"
#include "Sprite.h"
#include "StaticObject.h"
#include "CustomStaticObject.h"
#include "CCT.h"
#include "BodyPart.h"
#include "PartsModel.h"
#include "Player.h"
#include "SpringArm.h"
#include "Movement.h"
#include "Sky.h"
#include "ChapterMapData.h"
#include "NavigationManager.h"

#include "Stargazer.h"
#include "HotelDoor.h"
#include "BossArenaBarrier.h"
#include "Cage.h"
#include "Ladder.h"
#include "NPC.h"
#include "Elevator.h"
#include "MapActivator.h"
#include "MapTrap.h"
#include "RollingBallTrap.h"
#include "PressurePlate.h"
#include "NeedleLauncher.h"
#include "NeedleProjectile.h"
#include "SteppingStone.h"
#include "MonasteryBridge.h"
#include "Item.h"
#include "TreasureChest.h"

#include "EquipmentPreviewCharacter.h"
#include "EquipmentPreviewBackground.h"
#include "UILoadingController.h"

#include "Trigger.h"
#include "GreatSword.h"
#include "Scissor.h"
#include "TargetComponent.h"
#include "GamePlayMapPreset.h" 
#include "Monster_Include.h"
#include "ChapterMapManager.h"
#include "ThreadPool.h"

#include "FogVolume.h"
#include "CutScene_Actor.h"

#include "ChargeBomb.h"
#include "ItemTable.h"

#ifdef _DEBUG
#include "PXDebugRenderer.h"
#endif // _DEBUG

unordered_set<_uint> CLoader::g_LoadedChapters = {};


CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
	, m_hWnd{ hWnd }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
    CLoader* pLoader = static_cast<CLoader*>(pArg);

    if (FAILED(pLoader->Loading()))
        return -1;

    return 0;

}


HRESULT CLoader::Begin_Thread()
{
    InitializeCriticalSection(&m_CriticalSection);

    m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr));
    if (0 == m_hThread) return E_FAIL;

    return S_OK;
}

HRESULT CLoader::Initialize(LEVEL eNextLevelID)
{
    m_eNextLevelID = eNextLevelID;

    return Begin_Thread();
}

HRESULT CLoader::Initialize(_uint iChapterIndex)
{
    if (iChapterIndex >= g_iMaxChapter) return E_FAIL;
    
    m_iChapterIndex = iChapterIndex;
    return Begin_Thread();
}

HRESULT CLoader::Loading()
{
    EnterCriticalSection(&m_CriticalSection);

    CoInitializeEx(nullptr, 0);

    HRESULT         hr = {};

    //예약된 챕터가 있다면 
    if (m_iChapterIndex != UINT_MAX)
    {
        hr = Ready_Resources_For_Chapter(m_iChapterIndex);
    }
    else
    {
        switch (m_eNextLevelID)
        {
        case LEVEL::GAMEPLAY:
            hr = Ready_Resources_For_GamePlay();
            if (SUCCEEDED(hr))
                hr = Ready_Resources_For_Chapter(GAMEPLAY_START_CHAPTER); // 시작 챕터 
            break;

        case LEVEL::EDIT:
            hr = Ready_Resources_For_GamePlay();
            if (SUCCEEDED(hr))
                hr = Ready_Resources_For_Chapter(GAMEPLAY_START_CHAPTER);
            break;
        case LEVEL::SHADER:
            hr = Ready_Resources_For_GamePlay();
            if (SUCCEEDED(hr))
                hr = Ready_Resources_For_Chapter(GAMEPLAY_START_CHAPTER);
            break;
        }


    }

    m_ModelBatch.Wait();
	m_AnimBatch.Wait();

    if (SUCCEEDED(hr) && m_AnimBatch.IsFailed())
        hr = E_FAIL;

    if(SUCCEEDED(hr))
		hr = Join_And_Register_Models();
    else
        Clear_ModelJobs();

    CoUninitialize();

    LeaveCriticalSection(&m_CriticalSection);

    if (FAILED(hr))  
        m_isFailed.store(true, memory_order_relaxed);

    // 메모리 재배치로 인한 순서 꼬임을 방지함 
    // 반드시 가장 마지막에 release. 이 위의 모든 쓰기가 이 기준선 위에 오도록 함 
    // aquire로 true를 읽은 메인스레드에 모든 쓰기가 끝났음을 보장함 
    m_isFinished.store(true, memory_order_release);

    if (FAILED(hr))
    {
        lstrcpy(m_szLoadingText, TEXT("리소스 로딩 실패"));

        if (false == m_pGameInstance->Is_LoadingCancled())
            MSG_BOX("Loader Thread : Failed to Load Resources");
        return E_FAIL;
    }

    lstrcpy(m_szLoadingText, TEXT("리소스 로딩 완료"));


    return S_OK;
}

_bool CLoader::Is_ChapterCached(_uint iChapterIndex)
{
    CUILoadingController* pLoadingController = static_cast<CUILoadingController*>(CGameInstance::Get_Instance()->Find_UIController(L"LoadingController"));

    if (pLoadingController != nullptr)
        pLoadingController->Set_LoadingProgress(1.f);

    return g_LoadedChapters.count(iChapterIndex) > 0;
}

void CLoader::Clear_CachedChapters()
{
	g_LoadedChapters.clear();
}

//#ifdef _DEBUG

_float CLoader::Get_LoadingProgress() const
{
    if (m_isFinished.load(memory_order_acquire))
        return 1.f;

    const _uint iTotalTaskCount = m_ModelBatch.Get_TotalTaskCount() + m_AnimBatch.Get_TotalTaskCount();
    const _uint iCompletedTaskCount = m_ModelBatch.Get_CompletedTaskCount() + m_AnimBatch.Get_CompletedTaskCount();

    if (iTotalTaskCount == 0)
        return m_fHighestLoadingProgress.load(memory_order_relaxed);

    // 병렬 작업 완료 비율은 95%까지만 사용하고, 최종 등록 완료 시 100%로 만든다.
    const _float fMeasuredProgress = min(0.95f, static_cast<_float>(iCompletedTaskCount) / static_cast<_float>(iTotalTaskCount) * 0.95f);
    _float fHighestProgress = m_fHighestLoadingProgress.load(memory_order_relaxed);

    while (fMeasuredProgress > fHighestProgress && !m_fHighestLoadingProgress.compare_exchange_weak(fHighestProgress, fMeasuredProgress, memory_order_relaxed))
    {
    }

    return max(fHighestProgress, fMeasuredProgress);
}

void CLoader::Show()
{
    if(m_hWnd)
        SetWindowText(m_hWnd, m_szLoadingText);
}

//#endif


HRESULT CLoader::Ready_Resources_For_Logo()
{
    lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));
    //m_isFinished = true;

    return S_OK;
}

//게임플레이 전역적으로 필요한 리소스 (플레이어, 컴포넌트, 장비, 맵 오브젝트 등 )
HRESULT CLoader::Ready_Resources_For_GamePlay()
{
    lstrcpy(m_szLoadingText, TEXT("GamePlay : 텍스쳐 로딩 중"));

    _wstring strSkyTextureName = TEXT("SkyBox");
    _wstring strSkyTexturePath = m_pGameInstance->Get_ResourcePath() + TEXT("\\Map\\SkyBox\\SkyBox.dds");

    CTexture* pSkyTexture = { nullptr };

    // Logo에서 먼저 등록한 SkyBox가 있으면 재사용하고, Gameplay 직행 시에만 새로 생성한다.
    if (FAILED(m_pGameInstance->Get_Texture(strSkyTextureName, &pSkyTexture)))
    {
        pSkyTexture = CTexture::Create(m_pDevice, m_pContext, &strSkyTextureName, &strSkyTexturePath);
        if (nullptr == pSkyTexture)
        {
            MSG_BOX("SkyTexture not Created");
            return E_FAIL;
        }

        if (FAILED(m_pGameInstance->Register_Texture(pSkyTexture)))
        {
            Safe_Release(pSkyTexture);
            return E_FAIL;
        }
    }
    else
        Safe_Release(pSkyTexture);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), CSky::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    //Item Table Load 
    CItemTable::Load_ItemList();

    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 컴포넌트 로딩 중"));

#pragma region [ PLAYER COM ]
    /*Prototype_Component_Movement*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement"), CMovement::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /*Prototype_Component_SpringArm*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_SpringArm"), CSpringArm::Create())))
        return E_FAIL;

    /*Prototype_Component_Target*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Target"), CTargetComponent::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /*Prototype_Component_CCT*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"), CCT::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /*Prototype_Component_Movement_AI*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement_AI"), CMovement_AI::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    
#pragma endregion

// Start Model Load --------------------------------------------------------------------------------------------------------------
// Model 생성 함수를 Job에 등록하고, 후에 ThreadPool에서 병렬로 생성 + Add_Prototype() 호출

    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 모델 로딩 중"));
	
#pragma region [ PLAYER MODEL ]

    /* Pino Animation */
    //플레이어 애니메이션 파일이 제일 오래걸려서 제일 먼저 워커에 보냄 
    Enqueue_AnimationJob(L"Pino", L"Pino_AllParts", true, ANIMATION_TIME_UNIT::TICKS);
    // 보물상자는 Controller 없이 Close/Open Clip을 직접 재생한다.
    Enqueue_AnimationJob(L"TreasureChest", L"TreasureChest", true);

    /* Prototype_Component_Model_GreatSword*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_GreatSword"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"GreatSword", Pre);
        } });

    /* Prototype_Component_Model_Scissor_All*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Scissor_Merge"), [this]() -> CBase*
        {
            //_matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            _matrix Pre = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f) * XMMatrixRotationX(XMConvertToRadians(90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Scissor_Merge", Pre);
        } });

    /* Prototype_Component_Model_Scissor_Left*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Scissor_Left"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f) * XMMatrixRotationX(XMConvertToRadians(90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Scissor_Left", Pre);
        } });

    /* Prototype_Component_Model_Scissor_Right*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Scissor_Right"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f) * XMMatrixRotationX(XMConvertToRadians(90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Scissor_Right", Pre);
        } });

    /*Prototype_Component_Model_Player_Acc_Eyepatch*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Acc_Eyepatch"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Acc_EyePatch", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Acc_Rudolf_Nose*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Acc_Rudolf_Nose"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Acc_RudolfNose", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Helmet_MadDonkey*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Helmet_MadDonkey"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_MadDonkey", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Helmet_Santa*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Helmet_Santa"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_RedHat", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Helmet_Alchemist*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Helmet_Alchemist"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_Alchemist", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_PlayerBody*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseBody", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_PlayerBody2*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody2"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Body2", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Facial*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Facial"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Face", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_ProtoArm*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_ProtoArm"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_ProtoArm", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_GrappleGun*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_GrappleGun"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_GrappleGun", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_ChargeGun*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_ChargeGun"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_SniperCannon", XMMatrixIdentity()); } });

    /*Prototype_Component_Model_Player_Hair*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Hair"), [this]() -> CBase*
        { return CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseHair", XMMatrixIdentity()); } });
    
    /*Prototype_Component_Model_Player_Lamp*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Pino_Lamp"), [this]() -> CBase*
        {
            //_matrix Pre = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f) * XMMatrixRotationX(XMConvertToRadians(90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Lamp", XMMatrixIdentity());
        } });

#pragma endregion
#pragma region [ Monster_Weapon ]
    /* Prototype_Component_Model_FestivalHead*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_FestivalHead"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Fesitval", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Candle*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Candle"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Candle", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Knife*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Knife"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Knife", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Elite*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Elite"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Elite", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Train*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Train"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Train", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Raxasia*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Raxasia"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_Raxasia", Pre);
        } });

    /* Prototype_Component_Model_Shield_Raxasia*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Shield_Raxasia"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Shield_Raxasia", Pre);
        } });

    /* Prototype_Component_Model_Weapon_ShotGun*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_ShotGun"), [this]() -> CBase*
        {
            _matrix Pre =
                XMMatrixScaling(0.01f, 0.01f, 0.01f) *
                XMMatrixRotationX(XMConvertToRadians(-90)) *
                XMMatrixRotationX(XMConvertToRadians(180)) *
                XMMatrixRotationY(XMConvertToRadians(45));
            return CModel::Create(m_pDevice, m_pContext, L"Weapon_ShotGun", Pre);
        } });

    /* Prototype_Component_Model_Weapon_Bapist*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Bapist"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) *
                XMMatrixRotationZ(XMConvertToRadians(90.f)) *
                XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Bapist", Pre);
        } });

    /* Prototype_Component_Model_Weapon_GreatSaw*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_GreatSaw"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"GreatSaw", Pre);
        } });

#pragma endregion 
#pragma region [ Monster Equipment]
    Enqueue_AnimationJob(L"Shooter", L"Shooter", false);

    /* Shooter */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Shooter"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        //_matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
        return CModel::Create(m_pDevice, m_pContext, L"Shooter", Pre);
    } });

    /* Prototype_Component_Model_Canister*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Canister"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"Canister", Pre);
        } });

    /* Prototype_Component_Model_MagicianHat*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_MagicianHat"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
            return CModel::Create(m_pDevice, m_pContext, L"MagicianHat", Pre);
        } });

#pragma endregion

#pragma region [ MAP OBJECT MODEL ]

	/* Prototype_Component_Model_NPC_Sophia */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Sophia"), [this]() -> CBase*
		{
			// 다른 인간형 캐릭터와 동일하게 원본 센티미터 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Sophia", PreTransform);
		} });

	/* Prototype_Component_Model_NPC_Antonia */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Antonia"), [this]() -> CBase*
		{
			// NPC 공통 원본 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Antonia", PreTransform);
		} });

	/* Prototype_Component_Model_NPC_Polendina */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Polendina"), [this]() -> CBase*
		{
			// NPC 공통 원본 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Polendina", PreTransform);
		} });

	/* Prototype_Component_Model_NPC_Vendor */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Vendor"), [this]() -> CBase*
		{
			// NPC 공통 원본 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Vendor", PreTransform);
		} });

	/* Prototype_Component_Model_NPC_Riddleking */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Riddleking"), [this]() -> CBase*
		{
			// NPC 공통 원본 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Riddleking", PreTransform);
		} });

	/* Prototype_Component_Model_NPC_Eugenie */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NPC_Eugenie"), [this]() -> CBase*
		{
			// NPC 공통 원본 단위와 정면 축을 보정한다.
			const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
			return CModel::Create(m_pDevice, m_pContext, L"Eugenie", PreTransform);
		} });

    /*Prototype_Component_Model_Stargazer_Broken*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer_Broken"), [this]() -> CBase*
        { return CModel::Create(m_pDevice, m_pContext, L"Stargazer_Broken", XMMatrixScaling(0.0001f, 0.0001f, 0.0001f)); } });

    /*Prototype_Component_Model_Stargazer*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer"), [this]() -> CBase*
        { return CModel::Create(m_pDevice, m_pContext, L"Stargazer", XMMatrixScaling(0.0001f, 0.0001f, 0.0001f)); } });

    /* Prototype_Component_Model_Stargazer_Hotel */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer_Hotel"), [this]() -> CBase*
        {
            // 호텔 대형 시계 원본의 cm 단위를 게임 월드의 m 단위로 변환한다.
            return CModel::Create(m_pDevice, m_pContext, L"Stargazer_Hotel", XMMatrixScaling(0.01f, 0.01f, 0.01f));
        } });

    /*Prototype_Component_Model_HotelDoor*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_HotelDoor"), [this]() -> CBase*
        {
            // 문 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Hotel_Door", Pre);
        } });

    /*Prototype_Component_Model_Hotel_InDoor*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Hotel_InDoor"), [this]() -> CBase*
        {
            // 문 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
            _matrix Pre = XMMatrixScaling(0.001f, 0.001f, 0.001f);
            return CModel::Create(m_pDevice, m_pContext, L"Hotel_InDoor", Pre);
        } });

	/* Prototype_Component_Model_Monastery_Door */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monastery_Door"), [this]() -> CBase*
		{
			// 수도원 문 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
			const _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
			return CModel::Create(m_pDevice, m_pContext, L"Monastery_Door", Pre);
		} });

	/* Prototype_Component_Model_Train_Door */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Train_Door"), [this]() -> CBase*
		{
			// 열차 문 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
			const _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
			return CModel::Create(m_pDevice, m_pContext, L"Train_Door", Pre);
		} });

	/* Prototype_Component_Model_Boss_Door */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Boss_Door"), [this]() -> CBase*
		{
			// 보스 문 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
			const _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
			return CModel::Create(m_pDevice, m_pContext, L"Boss_Door", Pre);
		} });

	/* Prototype_Component_Model_BossArenaBarrier */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_BossArenaBarrier"), [this]() -> CBase*
		{
			// Boss_Barrier 데이터가 이미 월드 단위로 제작됐으므로 별도 PreTransform을 적용하지 않는다.
			return CModel::Create(m_pDevice, m_pContext, L"Boss_Barrier");
		} });

	/* Prototype_Component_Model_Cage */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Cage"), [this]() -> CBase*
		{
			// Cage 애니메이션의 최종 포즈가 이미 Y-Up이므로 단위 스케일만 보정한다.
			const _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
			return CModel::Create(m_pDevice, m_pContext, L"Cage", Pre);
		} });

    /* Prototype_Component_Model_Elevator */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Elevator"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Elevator", Pre);
       } });

    /*Prototype_Component_Model_Elevator_Controller00*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Elevator_Controller00"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
        return CModel::Create(m_pDevice, m_pContext, L"Elevator_Controller00",Pre);
    } });

    /*Prototype_Component_Model_Ladder_MetalWood*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_MetalWood"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_MetalWood", Pre);
        } });

    /*Prototype_Component_Model_Ladder_MetalWood6M*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_MetalWood6M"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_MetalWood6M", Pre);
        } });

    /*Prototype_Component_Model_Ladder_Static_3_5m*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_Static_3_5m"), [this]() -> CBase*
        {
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_3_5m");
        } });

    /*Prototype_Component_Model_Ladder_Static_5m*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_Static_5m"), [this]() -> CBase*
        {
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_5m");
        } });

    /*Prototype_Component_Model_Ladder_Static_7m*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_Static_7m"), [this]() -> CBase*
        {
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_7m");
        } });

    /*Prototype_Component_Model_Ladder_Static_7_5m*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Ladder_Static_7_5m"), [this]() -> CBase*
        {
            return CModel::Create(m_pDevice, m_pContext, L"Ladder_7_5m");
        } });

    /*Prototype_Component_Model_LadderActivater*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_LadderActivater"), [this]() -> CBase*
        {
            /*_matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);*/
            return CModel::Create(m_pDevice, m_pContext, L"LadderActivater");
        } });

    /*Prototype_Component_Model_Monastery_WallDeco01*/
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monastery_WallDeco01"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Monastery_WallDeco01", Pre);
        } });

	/* Prototype_Component_Model_RollingBall */
	m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_RollingBall"), [this]() -> CBase*
		{
			return CModel::Create(m_pDevice, m_pContext, L"RollingBall");
		} });


    /* Prototype_Component_Model_Trapfloor */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Trapfloor"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"Trapfloor", PreTransform);
        } });

    /* Prototype_Component_Model_NeedleHole */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_NeedleHole"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"NeedleHole", PreTransform);
        } });

    /* Prototype_Component_Model_SteppingStone */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_SteppingStone"), [this]() -> CBase*
        {
            // SteppingStone 원본 모델은 센티미터 단위이므로 0.01을 적용한다.
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f);
            return CModel::Create(m_pDevice, m_pContext, L"SteppingStone", Pre);
        } });

    /* Prototype_Component_Model_Monastery_Bridge */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monastery_Bridge"), [this]() -> CBase*
        {
            // Bridge 모델은 이미 현재 월드 단위에 맞아 별도 PreTransform을 적용하지 않는다.
            return CModel::Create(m_pDevice, m_pContext, L"Monastery_Bridge");
        } });

    /* Prototype_Component_Model_Item */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Item"), [this]() -> CBase*
        {
            return CModel::Create(m_pDevice, m_pContext, L"Item");
        } });

    /* Prototype_Component_Model_TreasureChest */
    m_GamePlay_ModelJobs.push_back({ ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_TreasureChest"), [this]() -> CBase*
        {
            // Unreal 원본 TreasureChest는 센티미터 단위와 Z-Up 좌표계를 사용한다.
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) ;
            return CModel::Create(m_pDevice, m_pContext, L"TreasureChest", Pre);
        } });


#pragma endregion

    // !! 주의 !! Bullet은 Job에 넣지않음!! 
    /* Prototype_Component_Model_Bullet */

    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f);
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Bullet"),
            CModel::Create(m_pDevice, m_pContext, L"TestProjectile", PreWeaponTransformMatrix))))
            return E_FAIL;
        PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Missile"),
            CModel::Create(m_pDevice, m_pContext, L"Missile", PreWeaponTransformMatrix))))
            return E_FAIL;

        HRESULT hr = m_pGameInstance->Create_Pool<CProjectileEmitter, 8>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CMissile, 8>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CElectricBall, 16>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CGroundBall, 8>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CSectorBall, 64>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CBullet, 32>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CBoltMine, 16>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CBoltSector, 8>();
        if (FAILED(hr))
            return E_FAIL;

        hr = m_pGameInstance->Create_Pool<CChargeBomb, 5>();
        if (FAILED(hr))
            return E_FAIL;

    }
    
    /* Prototype_Component_Model_Needle */ // 트랩용 화살? 바늘?
    const _matrix NeedlePreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Needle"), 
        CModel::Create(m_pDevice, m_pContext, L"Needle", NeedlePreTransform))))
        return E_FAIL;

    HRESULT hr = m_pGameInstance->Create_Pool<CNeedleProjectile, 32>();
    if (FAILED(hr))
        return E_FAIL;


    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 애니메이션 로딩 중"));


#pragma endregion

//    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 애니메이션 로딩 중"));
//
//#pragma region [ ANIMATION ]
//    if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Pino", L"Pino_AllParts", true, ANIMATION_TIME_UNIT::TICKS)))
//        return E_FAIL;
//#pragma endregion

	CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if(pPool == nullptr)
    {
        MSG_BOX("Thread Pool is nullptr");
        return E_FAIL;
	}

    for(ModelJob& job : m_GamePlay_ModelJobs)
    {
		ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() ->HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
				return pJob->pResult != nullptr ? S_OK : E_FAIL;
            });
	}

    // 로딩 스레디 Wait과 AddPrototype등은 모든 Job을 Enequeue한 뒤인 Loading 후반에서 수행함 
    
// End Model Load  -----------------------------------------------------------------------------------------------------

    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 객체원형 로딩 중"));
 
#pragma region [ PLAYER OBJECT ]

    /* Prototype_Weapon_GreatSword */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Weapon_GreatSword"),
        CGreatSword::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_Weapon_Scissor */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Weapon_Scissor"),
        CScissor::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_BodyPart */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_BodyPart"),
        CBodyPart::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_EquipmentPreviewBackground */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_EquipmentPreviewBackground"), CEquipmentPreviewBackground::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_EquipmentPreviewCharacter */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_EquipmentPreviewCharacter"), CEquipmentPreviewCharacter::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Camera */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_CameraFree"),
        CCamera_Free::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    // 컷신 캐릭터는 챕터별 모델 원형을 교체해 사용하므로 게임플레이 공용 객체 원형만 한 번 등록한다.
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_CutSceneActor"), CCutScene_Actor::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    /* Prototype_GameObject_Body_Player */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
       CPlayer::Create(m_pDevice, m_pContext))))
        return E_FAIL;

#pragma endregion
#pragma region [ Monster Weapon ]

    /* Prototype_GameObject_Weapon_Melee */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Melee"),
        CWeapon_Melee::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Weapon_Range */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Range"),
        CWeapon_Range::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Weapon_Raxasia */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Raxasia"),
        CWeapon_Raxasia::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Shield_Raxasia */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Shield_Raxasia"),
        CShield_Raxasia::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Shooter */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Shooter"),
        CShooter::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Attach */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Attach"),
        CMonster_Attach::Create(m_pDevice, m_pContext))))
        return E_FAIL;


#pragma endregion
#pragma region [ MAP OBJECT ]
    /* Prototype_GameObject_FogVolume */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_FogVolume"),
        CFogVolume::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    /* Prototype_GameObject_Stargazer */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Stargazer"),
        CStargazer::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_NPC */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_NPC"), CNPC::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_HotelDoor */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_HotelDoor"),
        CHotelDoor::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_BossArenaBarrier */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_BossArenaBarrier"),
        CBossArenaBarrier::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Cage */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Cage"),
        CCage::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Ladder */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Ladder"), CLadder::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Elevator"), CElevator::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Elevator_Controller00 */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Elevator_Controller00"),  
        CMapActivator::Create(m_pDevice, m_pContext, CMapActivator::ACTIVATOR_TYPE::ELEVATOR_CONTROLLER))))
        return E_FAIL;

    /* Prototype_GameObject_LadderActivater */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_LadderActivater"),
        CMapActivator::Create(m_pDevice, m_pContext, CMapActivator::ACTIVATOR_TYPE::LADDER_ACTIVATER))))
        return E_FAIL;

    /* Prototype_GameObject_MapTrap */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapTrap"),
        CMapTrap::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_RollingBallTrap */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_RollingBallTrap"),
        CRollingBallTrap::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_PressurePlate */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_PressurePlate"), 
        CPressurePlate::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_NeedleLauncher */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_NeedleLauncher"), 
        CNeedleLauncher::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_SteppingStone */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_SteppingStone"),
        CSteppingStone::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_MonasteryBridge */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MonasteryBridge"),
        CMonasteryBridge::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Item */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Item"),
        CItem::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_TreasureChest */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_TreasureChest"),
        CTreasureChest::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    ///*Prototype_GameObject_Trigger */
    //if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Trigger"),
    //    CTrigger::Create(m_pDevice, m_pContext))))
    //    return E_FAIL;

    /* Prototype_GameObject_DropItem */
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_DropItem"),
        CDropItem::Create(m_pDevice, m_pContext))))
        return E_FAIL;

#pragma endregion


    lstrcpy(m_szLoadingText, TEXT("게임플레이 로딩이 완료되었습니다."));
    return S_OK;
}

HRESULT CLoader::Ready_Resources_For_Chapter01()
{
    lstrcpy(m_szLoadingText, TEXT("챕터 1(기차역) 리소스 로딩 "));

    const _uint iLevel = ETOUI(LEVEL::CHAP_01);

#pragma region [ Monsters ]
    lstrcpy(m_szLoadingText, TEXT("챕터 1 : 모델 로딩 중"));

    /* Buttler */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Basic"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Basic", Pre);
        } });

    /* Train */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Train"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Train", Pre);
        } });

    /* NoHead */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_NoHead"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_NoHead", Pre);
        } });

    /* Range */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Range"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Range", Pre);
        } });

    /* Elite */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Elite"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Elite", Pre);
        } });

    /* FestivalLeader */
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Festival"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"FestivalLeader", Pre);
        } });

    // F_guide 컷신 전용 FestivalLeader와 ClownPanel 리소스를 Chapter01 로딩 단계에서 미리 등록한다.
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_FestivalLeader"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_FestivalLeader", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_01"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_01", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_02"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_02", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_03"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_03", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_04"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_04", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_05"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_05", PreTransform);
        } });

    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_01_ClownF"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_01_ClownF", PreTransform);
        } });

    // Intro 컷신의 파란 나비를 Chapter01 진입 시 미리 등록한다.
    m_Chapter01_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Butterfly"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Butterfly", PreTransform);
        } });


    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if (nullptr == pPool)
        return E_FAIL;

    for (ModelJob& job : m_Chapter01_ModelJobs)
    {
        ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() -> HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
                return (nullptr != pJob->pResult) ? S_OK : E_FAIL;
            });
    }

    // 로딩 스레드 Wait과 AddPrototype등은 모든 Job을 Enequeue한 뒤인 Loading 후반에서 수행함 

#pragma endregion

    lstrcpy(m_szLoadingText, TEXT("챕터 1 : 애니메이션 로딩 중"));

#pragma region [ ANIMATION ]

    Enqueue_AnimationJob(L"Buttler_Basic", L"Buttler_Basic", false);
    Enqueue_AnimationJob(L"Buttler_Train", L"Buttler_Train", false);
    Enqueue_AnimationJob(L"Buttler_NoHead", L"Buttler_NoHead", false);
    Enqueue_AnimationJob(L"Monster_Range", L"Monster_Range", false);
    Enqueue_AnimationJob(L"Monster_Elite", L"Monster_Elite", false);
    Enqueue_AnimationJob(L"FestivalLeader", L"FestivalLeader", false);
    // F_guide 컷신 애니메이션은 원본 Sequencer와 동일한 Tick 단위로 평가한다.
    Enqueue_AnimationJob(L"CutScene_FestivalLeader", L"CutScene_FestivalLeader", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"ClownPanel_01_ClownF", L"ClownPanel_01_ClownF", false, ANIMATION_TIME_UNIT::TICKS);
    // Intro 나비 애니메이션도 원본 Sequencer의 Tick 단위로 로드한다.
    Enqueue_AnimationJob(L"CutScene_Butterfly", L"CutScene_Butterfly", false, ANIMATION_TIME_UNIT::TICKS);

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Buttler_Basic", L"Buttler_Basic", false)))
    //    return E_FAIL;

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Buttler_Train", L"Buttler_Train", false)))
    //    return E_FAIL;

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Buttler_NoHead", L"Buttler_NoHead", false)))
    //    return E_FAIL;

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Monster_Range", L"Monster_Range", false)))
    //    return E_FAIL;

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"Monster_Elite", L"Monster_Elite", false)))
    //    return E_FAIL;

    //if (FAILED(m_pGameInstance->Ready_RawAnimations(L"FestivalLeader", L"FestivalLeader", false)))
    //    return E_FAIL;
#pragma endregion

    lstrcpy(m_szLoadingText, TEXT("챕터 1 : 객체원형 로딩 중"));

#pragma region [ Monster ]


    /* Prototype_GameObject_Boss_Festival */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Festival"),
        CBoss_Festival::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Buttler */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Buttler"),
        CMonster_Buttler::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Elite */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Elite"),
        CMonster_Elite::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Range */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Range"),
        CMonster_Range::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    //애니메이션이 보장되고나서 풀 생성
    m_AnimBatch.Wait();

    if (m_AnimBatch.IsFailed())
        return E_FAIL;

    HRESULT BasicResult = m_pGameInstance->Create_Pool<CButtler_Basic, 14>();
    if (FAILED(BasicResult))
        return E_FAIL;

    HRESULT TrainResult = m_pGameInstance->Create_Pool<CButtler_Train, 7>();
    if (FAILED(TrainResult))
        return E_FAIL;

    HRESULT NoHeadResult = m_pGameInstance->Create_Pool<CButtler_NoHead, 6>();
    if (FAILED(NoHeadResult))
        return E_FAIL;

    HRESULT EliteResult = m_pGameInstance->Create_Pool<CMonster_Elite, 4>();
    if (FAILED(EliteResult))
        return E_FAIL;

    HRESULT RangeResult = m_pGameInstance->Create_Pool<CMonster_Range, 8>();
    if (FAILED(RangeResult))
        return E_FAIL;

    HRESULT BossFestivalResult = m_pGameInstance->Create_Pool<CBoss_Festival, 2>();
    if (FAILED(BossFestivalResult))
        return E_FAIL;

#pragma endregion

    lstrcpy(m_szLoadingText, TEXT("챕터 1 로딩이 완료되었습니다."));

    return S_OK;
}

HRESULT CLoader::Ready_Resources_For_Chapter02()
{
    const _uint iLevel = ETOUI(LEVEL::CHAP_02);
    lstrcpy(m_szLoadingText, L"챕터 2(호텔) 리소스 로딩 중 ");


    lstrcpy(m_szLoadingText, TEXT("챕터  2(호텔) : 몬스터 리소스 로딩 중 "));

    Enqueue_AnimationJob(L"Training", L"Training", false);

    //모델 생성
 /* Training */
    m_Chapter02_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Training"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Training", Pre);
        } });


    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if (nullptr == pPool)
        return E_FAIL;

    for (ModelJob& job : m_Chapter02_ModelJobs)
    {
        ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() -> HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
                return (nullptr != pJob->pResult) ? S_OK : E_FAIL;
            });
    }

    m_AnimBatch.Wait();

    if (m_AnimBatch.IsFailed())
        return E_FAIL;

    HRESULT TrainingResult = m_pGameInstance->Create_Pool<CMonster_Training, 3>();
    if (FAILED(TrainingResult))
        return E_FAIL;


    lstrcpy(m_szLoadingText, TEXT("챕터 2 로딩이 완료되었습니다."));
    return S_OK;
}

HRESULT CLoader::Ready_Resources_For_Chapter03()
{

    const _uint iLevel = ETOUI(LEVEL::CHAP_03);

    lstrcpy(m_szLoadingText, L"챕터 3(락사시아 맵) 리소스 로딩 중 ");


#pragma region [ Monsters ]

    //애니메이션 로드
    Enqueue_AnimationJob(L"Raxasia_P1", L"Raxasia_P1", false);
    Enqueue_AnimationJob(L"Raxasia_P2", L"Raxasia_P2", false);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p1", L"CutScene_Raxasia_p1", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p2_Broken", L"CutScene_Raxasia_p2_Broken", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p2", L"CutScene_Raxasia_p2", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_Sword", L"CutScene_Raxasia_Sword", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Distruction_Wall", L"CutScene_Distruction_Wall", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p1_Helmet", L"CutScene_Raxasia_p1_Helmet", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Butterfly", L"CutScene_Butterfly", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"Balistar", L"Balistar", false);
    Enqueue_AnimationJob(L"ButterFly", L"ButterFly", false);
    Enqueue_AnimationJob(L"BigDog", L"BigDog", false);
    Enqueue_AnimationJob(L"Monster_Zombie", L"Monster_Zombie", false);
    Enqueue_AnimationJob(L"Priest", L"Priest", false);
    Enqueue_AnimationJob(L"MadDonKey", L"MadDonKey", false);

    /* Raxasia_P1 */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P1"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P1", Pre);
    } });

    /* Raxasia_P2 */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P2"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P2", Pre);
    } });

    // 컷신용 모델도 Chapter03의 기존 ModelJob 흐름에서 함께 생성하고 등록한다.
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p1"), [this]() -> CBase*
    {
        // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p1", PreTransform);
    } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2"), [this]() -> CBase*
{
            // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2", PreTransform);
        } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2_Broken"), [this]() -> CBase*
{
            // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2_Broken", PreTransform);
        } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_Sword"), [this]() -> CBase*
    {
        // 독립 재생하는 검의 Root Animation 좌표계를 컷신 본체와 같은 단위 및 축으로 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_Sword", PreTransform);
    } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_Sword_Split"), [this]() -> CBase*
    {
        // 하나의 Skeleton과 Animator가 칼날 Mesh와 외장 Mesh를 함께 제어한다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_Sword_Split", PreTransform);
    } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p1_Helmet"), [this]() -> CBase*
{
            // 검도 본체와 같은 단위 및 축 변환을 사용해 weapon0_l Bone 공간을 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p1_Helmet", PreTransform);
    } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Distruction_Wall"), [this]() -> CBase*
    {
            // 검도 본체와 같은 단위 및 축 변환을 사용해 weapon0_l Bone 공간을 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Distruction_Wall", PreTransform);
        } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_Shield_Raxasia"), [this]() -> CBase*
    {
        // 일반 락사시아 방패와 동일한 Import Transform을 사용해 컷신 Bone 부착 크기와 축을 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
        return CModel::Create(m_pDevice, m_pContext, L"Shield_Raxasia", PreTransform);
    } });

    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2_Shield"), [this]() -> CBase*
    {
        // 1번 Broken 모델용 방패는 원본 컷신 리소스의 단위만 미터로 변환한다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f);
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2_Shield", PreTransform);
    } });

    /* Balistar */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Balistar"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Balistar", Pre);
    } });

    /* ButterFly */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_ButterFly"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"ButterFly", Pre);
    } });

    // Sophia Dead 컷신의 파란 나비를 일반 몬스터와 분리된 컷신 모델로 등록한다.
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Butterfly"), [this]() -> CBase*
    {
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Butterfly", PreTransform);
    } });

    /* BigDog */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_BigDog"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"BigDog", Pre);
    } });

    /* Monster_Zombie */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Zombie"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Zombie", Pre);
        } });

    /* Priest */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Priest"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Priest", Pre);
        } });

    /* MadDonKey */
    m_Chapter03_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_MadDonKey"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"MadDonKey", Pre);
        } });


    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if (nullptr == pPool)
        return E_FAIL;

    for (ModelJob& job : m_Chapter03_ModelJobs)
    {
        ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() -> HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
                return (nullptr != pJob->pResult) ? S_OK : E_FAIL;
            });
    }

    // 로딩 스레드 Wait과 AddPrototype등은 모든 Job을 Enequeue한 뒤인 Loading 후반에서 수행함

    // 객체 원형 생성
    /* Prototype_GameObject_Boss_Raxasia_P1 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P1"),
        CBoss_Raxasia_P1::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Raxasia_P2 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P2"),
        CBoss_Raxasia_P2::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Balistar */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Balistar"),
        CBalistar::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Butterfly */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Butterfly"),
        CMonster_Butterfly::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_BigDog */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_BigDog"),
        CMonster_BigDog::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    //애니메이션이 보장되고나서 풀 생성
    /* Prototype_GameObject_Monster_Zombie */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Zombie"),
        CMonster_Zombie::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Priest */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Priest"),
        CMonster_Priest::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_MadDonkey */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_MadDonkey"),
        CBoss_MadDonkey::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    m_AnimBatch.Wait();

    if (m_AnimBatch.IsFailed())
        return E_FAIL;

    HRESULT BossRaxasiaP1Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P1, 2>();
    if (FAILED(BossRaxasiaP1Result))
        return E_FAIL;

    HRESULT BossRaxasiaP2Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P2, 2>();
    if (FAILED(BossRaxasiaP2Result))
        return E_FAIL;

    HRESULT BalistarResult = m_pGameInstance->Create_Pool<CBalistar, 2>();
    if (FAILED(BalistarResult))
        return E_FAIL;

    HRESULT ButterflyResult = m_pGameInstance->Create_Pool<CMonster_Butterfly, 2>();
    if (FAILED(ButterflyResult))
        return E_FAIL;

    HRESULT BigDogResult = m_pGameInstance->Create_Pool<CMonster_BigDog, 5>();
    if (FAILED(BigDogResult))
        return E_FAIL;

    HRESULT ZombieResult = m_pGameInstance->Create_Pool<CMonster_Zombie, 30>();
    if (FAILED(ZombieResult))
        return E_FAIL;

    HRESULT PriestResult = m_pGameInstance->Create_Pool<CMonster_Priest, 5>();
    if (FAILED(PriestResult))
        return E_FAIL;

    HRESULT MadDonkeyResult = m_pGameInstance->Create_Pool<CBoss_MadDonkey, 3>();
    if (FAILED(MadDonkeyResult))
        return E_FAIL;

#pragma endregion

    lstrcpy(m_szLoadingText, TEXT("챕터 3(락사시아 맵) 로딩이 완료되었습니다."));

    return S_OK;
}

HRESULT CLoader::Ready_Resources_For_ChapterTest()
{
    lstrcpy(m_szLoadingText, TEXT("테스트 맵 리소스 로딩 "));

    const _uint iLevel = ETOUI(LEVEL::CHAP_TEST);

    lstrcpy(m_szLoadingText, TEXT("테스트 맵 : 몬스터 리소스 로딩 중 "));

#pragma region [ Monsters ]

    //애니메이션 로드 
    Enqueue_AnimationJob(L"Training", L"Training", false);
    Enqueue_AnimationJob(L"Buttler_Basic", L"Buttler_Basic", false);
    Enqueue_AnimationJob(L"Buttler_Train", L"Buttler_Train", false);
    Enqueue_AnimationJob(L"Buttler_NoHead", L"Buttler_NoHead", false);
    Enqueue_AnimationJob(L"Monster_Range", L"Monster_Range", false);
    Enqueue_AnimationJob(L"Monster_Elite", L"Monster_Elite", false);
    Enqueue_AnimationJob(L"FestivalLeader", L"FestivalLeader", false);
    Enqueue_AnimationJob(L"Raxasia_P1", L"Raxasia_P1", false);
    Enqueue_AnimationJob(L"Raxasia_P2", L"Raxasia_P2", false);
    Enqueue_AnimationJob(L"Balistar", L"Balistar", false);
    Enqueue_AnimationJob(L"ButterFly", L"ButterFly", false);
    Enqueue_AnimationJob(L"BigDog", L"BigDog", false);
    Enqueue_AnimationJob(L"Monster_Zombie", L"Monster_Zombie", false);
    Enqueue_AnimationJob(L"Priest", L"Priest", false);
    Enqueue_AnimationJob(L"MadDonKey", L"MadDonKey", false);

    //모델 생성 
    /* Training */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Training"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Training", Pre);
        } });

    /* Buttler */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Basic"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Basic", Pre);
        } });

    /* Train */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Train"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.012f, 0.012f, 0.012f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Train", Pre);
        } });

    /* Elite */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Elite"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Elite", Pre);
        } });

    /* FestivalLeader */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Festival"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"FestivalLeader", Pre);
        } });

    /* Range */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Range"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Range", Pre);
        } });

    /* Raxasia_P1 */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P1"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P1", Pre);
    } });

    /* Raxasia_P2 */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P2"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P2", Pre);
    } });

    /* Balistar */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Balistar"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Balistar", Pre);
    } });

    /* ButterFly */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_ButterFly"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"ButterFly", Pre);
    } });

    /* BigDog */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_BigDog"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"BigDog", Pre);
    } });

    /* Monster_Zombie */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Zombie"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Monster_Zombie", Pre);
    } });

    /* Priest */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Priest"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Priest", Pre);
    } });

    /* MadDonKey */
    m_TestMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_MadDonKey"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"MadDonKey", Pre);
    } });

    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if (nullptr == pPool)
        return E_FAIL;

    for (ModelJob& job : m_TestMap_ModelJobs)
    {
        ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() -> HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
                return (nullptr != pJob->pResult) ? S_OK : E_FAIL;
            });
    }

    // 로딩 스레드 Wait과 AddPrototype등은 모든 Job을 Enequeue한 뒤인 Loading 후반에서 수행함 

    // 객체 원형 생성 
    /* Prototype_GameObject_Monster_Buttler */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Buttler"),
        CMonster_Buttler::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Training */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Training"),
        CMonster_Training::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Festival */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Festival"),
        CBoss_Festival::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Range */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Range"),
        CMonster_Range::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Raxasia_P1 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P1"),
        CBoss_Raxasia_P1::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Raxasia_P2 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P2"),
        CBoss_Raxasia_P2::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Balistar */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Balistar"),
        CBalistar::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Butterfly */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Butterfly"),
        CMonster_Butterfly::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_BigDog */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_BigDog"),
        CMonster_BigDog::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Zombie */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Zombie"),
        CMonster_Zombie::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Priest */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Priest"),
        CMonster_Priest::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_MadDonkey */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_MadDonkey"),
        CBoss_MadDonkey::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    //애니메이션이 보장되고나서 풀 생성
    m_AnimBatch.Wait();

    if (m_AnimBatch.IsFailed())
        return E_FAIL;

    HRESULT TrainingResult = m_pGameInstance->Create_Pool<CMonster_Training, 3>();
    if (FAILED(TrainingResult))
        return E_FAIL;

    HRESULT BasicResult = m_pGameInstance->Create_Pool<CButtler_Basic, 13>();
    if (FAILED(BasicResult))
        return E_FAIL;

    HRESULT TrainResult = m_pGameInstance->Create_Pool<CButtler_Train, 7>();
    if (FAILED(TrainResult))
        return E_FAIL;

    HRESULT NoHeadResult = m_pGameInstance->Create_Pool<CButtler_NoHead, 6>();
    if (FAILED(NoHeadResult))
        return E_FAIL;

    HRESULT EliteResult = m_pGameInstance->Create_Pool<CMonster_Elite, 4>();
    if (FAILED(EliteResult))
        return E_FAIL;

    HRESULT RangeResult = m_pGameInstance->Create_Pool<CMonster_Range, 8>();
    if (FAILED(RangeResult))
        return E_FAIL;

    HRESULT BossFestivalResult = m_pGameInstance->Create_Pool<CBoss_Festival, 2>();
    if (FAILED(BossFestivalResult))
        return E_FAIL;

    HRESULT BossRaxasiaP1Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P1, 2>();
    if (FAILED(BossRaxasiaP1Result))
        return E_FAIL;

    HRESULT BossRaxasiaP2Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P2, 2>();
    if (FAILED(BossRaxasiaP2Result))
        return E_FAIL;

    HRESULT BalistarResult = m_pGameInstance->Create_Pool<CBalistar, 2>();
    if (FAILED(BalistarResult))
        return E_FAIL;

    HRESULT ButterflyResult = m_pGameInstance->Create_Pool<CMonster_Butterfly, 5>();
    if (FAILED(ButterflyResult))
        return E_FAIL;

    HRESULT BigDogResult = m_pGameInstance->Create_Pool<CMonster_BigDog, 5>();
    if (FAILED(BigDogResult))
        return E_FAIL;

    HRESULT ZombieResult = m_pGameInstance->Create_Pool<CMonster_Zombie, 30>();
    if (FAILED(ZombieResult))
        return E_FAIL;

    HRESULT PriestResult = m_pGameInstance->Create_Pool<CMonster_Priest, 5>();
    if (FAILED(PriestResult))
        return E_FAIL;

    HRESULT MadDonkeyResult = m_pGameInstance->Create_Pool<CBoss_MadDonkey, 3>();
    if (FAILED(MadDonkeyResult))
        return E_FAIL;

#pragma endregion 

    lstrcpy(m_szLoadingText, TEXT("테스트 맵 로딩이 완료되었습니다."));

    return S_OK;
}


HRESULT CLoader::Ready_Resources_For_ChapterTestObj()
{


    return S_OK;
}

HRESULT CLoader::Ready_Resources_For_ChapterBossMap()
{
    lstrcpy(m_szLoadingText, TEXT("보스 맵 리소스 로딩 "));

    const _uint iLevel = ETOUI(LEVEL::CHAP_BOSS);

    lstrcpy(m_szLoadingText, TEXT("보스 맵 : 몬스터 리소스 로딩 중 "));

#pragma region [ Monsters ]

    //애니메이션 로드 
    Enqueue_AnimationJob(L"Training", L"Training", false);
    Enqueue_AnimationJob(L"Buttler_Basic", L"Buttler_Basic", false);
    Enqueue_AnimationJob(L"Buttler_Train", L"Buttler_Train", false);
    Enqueue_AnimationJob(L"Buttler_NoHead", L"Buttler_NoHead", false);
    Enqueue_AnimationJob(L"Monster_Range", L"Monster_Range", false);
    Enqueue_AnimationJob(L"Monster_Elite", L"Monster_Elite", false);
    Enqueue_AnimationJob(L"FestivalLeader", L"FestivalLeader", false);
    // BossMap에서도 F_guide 컷신 전용 FestivalLeader와 ClownF 애니메이션을 미리 등록한다.
    Enqueue_AnimationJob(L"CutScene_FestivalLeader", L"CutScene_FestivalLeader", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"ClownPanel_01_ClownF", L"ClownPanel_01_ClownF", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"Raxasia_P1", L"Raxasia_P1", false);
    Enqueue_AnimationJob(L"Raxasia_P2", L"Raxasia_P2", false);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p1", L"CutScene_Raxasia_p1", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p2_Broken", L"CutScene_Raxasia_p2_Broken", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p2", L"CutScene_Raxasia_p2", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_Sword", L"CutScene_Raxasia_Sword", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Distruction_Wall", L"CutScene_Distruction_Wall", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Raxasia_p1_Helmet", L"CutScene_Raxasia_p1_Helmet", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"CutScene_Butterfly", L"CutScene_Butterfly", false, ANIMATION_TIME_UNIT::TICKS);
    Enqueue_AnimationJob(L"Balistar", L"Balistar", false);
    Enqueue_AnimationJob(L"Butterfly", L"Butterfly", false);
    Enqueue_AnimationJob(L"BigDog", L"BigDog", false);
    Enqueue_AnimationJob(L"Monster_Zombie", L"Monster_Zombie", false);
    Enqueue_AnimationJob(L"Priest", L"Priest", false);
    Enqueue_AnimationJob(L"MadDonKey", L"MadDonKey", false);

    //모델 생성 
    /* Training */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Training"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Training", Pre);
        } });

    /* Buttler */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Basic"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Basic", Pre);
        } });

    /* Train */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Buttler_Train"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.012f, 0.012f, 0.012f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Buttler_Train", Pre);
        } });

    /* Elite */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Elite"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Elite", Pre);
        } });

    /* FestivalLeader */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Festival"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"FestivalLeader", Pre);
        } });

    // F_guide 컷신을 BossMap에서도 재생할 수 있도록 Chapter01과 동일한 Prototype Tag로 등록한다.
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_FestivalLeader"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_FestivalLeader", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_01"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_01", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_02"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_02", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_03"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_03", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_04"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_04", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_05"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_05", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_ClownPanel_01_ClownF"), [this]() -> CBase*
        {
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"ClownPanel_01_ClownF", PreTransform);
        } });

    /* Range */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Monster_Range"), [this]() -> CBase*
        {
            _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"Monster_Range", Pre);
        } });

    /* Raxasia_P1 */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P1"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P1", Pre);
    } });

    /* Raxasia_P2 */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Raxasia_P2"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) ;
        return CModel::Create(m_pDevice, m_pContext, L"Raxasia_P2", Pre);
    } });

    // 컷신용 모델도 BossMap의 기존 ModelJob 흐름에서 함께 생성하고 등록한다.
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p1"), [this]() -> CBase*
    {
        // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p1", PreTransform);
    } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2"), [this]() -> CBase*
{
            // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2_Broken"), [this]() -> CBase*
{
            // Bone 이동 단위와 UE 모델 축을 게임 좌표계에 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2_Broken", PreTransform);
        } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_Sword"), [this]() -> CBase*
    {
        // 독립 재생하는 검의 Root Animation 좌표계를 컷신 본체와 같은 단위 및 축으로 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_Sword", PreTransform);
    } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_Sword_Split"), [this]() -> CBase*
    {
        // 하나의 Skeleton과 Animator가 칼날 Mesh와 외장 Mesh를 함께 제어한다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_Sword_Split", PreTransform);
    } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p1_Helmet"), [this]() -> CBase*
    {
            // 검도 본체와 같은 단위 및 축 변환을 사용해 weapon0_l Bone 공간을 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p1_Helmet", PreTransform);
    } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Distruction_Wall"), [this]() -> CBase*
    {
            // 검도 본체와 같은 단위 및 축 변환을 사용해 weapon0_l Bone 공간을 맞춘다.
            const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
            return CModel::Create(m_pDevice, m_pContext, L"CutScene_Distruction_Wall", PreTransform);
     } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_Shield_Raxasia"), [this]() -> CBase*
    {
        // 일반 락사시아 방패와 동일한 Import Transform을 사용해 컷신 Bone 부착 크기와 축을 맞춘다.
        const _matrix PreTransform = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
        return CModel::Create(m_pDevice, m_pContext, L"Shield_Raxasia", PreTransform);
    } });

    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Raxasia_p2_Shield"), [this]() -> CBase*
    {
        // 1번 Broken 모델용 방패는 원본 컷신 리소스의 단위만 미터로 변환한다.
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f);
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Raxasia_p2_Shield", PreTransform);
    } });

    /* Balistar */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Balistar"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Balistar", Pre);
    } });

    /* Butterfly */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_ButterFly"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Butterfly", Pre);
    } });

    // BossMap에서도 Sophia Dead 컷신의 파란 나비 리소스를 같은 태그로 사용할 수 있게 등록한다.
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_CutScene_CutScene_Butterfly"), [this]() -> CBase*
    {
        const _matrix PreTransform = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"CutScene_Butterfly", PreTransform);
    } });

    /* BigDog */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_BigDog"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"BigDog", Pre);
    } });

    /* Monster_Zombie */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Zombie"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.014f, 0.014f, 0.014f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Monster_Zombie", Pre);
    } });

    /* Priest */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_Priest"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"Priest", Pre);
    } });

    /* MadDonKey */
    m_BossMap_ModelJobs.push_back({ iLevel, TEXT("Prototype_Component_Model_MadDonKey"), [this]() -> CBase*
    {
        _matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
        return CModel::Create(m_pDevice, m_pContext, L"MadDonKey", Pre);
    } });

    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if (nullptr == pPool)
        return E_FAIL;

    for (ModelJob& job : m_BossMap_ModelJobs)
    {
        ModelJob* pJob = &job;
        pPool->Enqueue(&m_ModelBatch, [pJob]() -> HRESULT
            {
                pJob->pResult = pJob->CreateFunc();
                return (nullptr != pJob->pResult) ? S_OK : E_FAIL;
            });
    }

    // 로딩 스레드 Wait과 AddPrototype등은 모든 Job을 Enequeue한 뒤인 Loading 후반에서 수행함 

    // 객체 원형 생성 
    /* Prototype_GameObject_Monster_Buttler */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Buttler"),
        CMonster_Buttler::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Training */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Training"),
        CMonster_Training::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Festival */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Festival"),
        CBoss_Festival::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Range */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Range"),
        CMonster_Range::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Raxasia_P1 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P1"),
        CBoss_Raxasia_P1::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_Raxasia_P2 */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_Raxasia_P2"),
        CBoss_Raxasia_P2::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Balistar */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Balistar"),
        CBalistar::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Butterfly */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Butterfly"),
        CMonster_Butterfly::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_BigDog */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_BigDog"),
        CMonster_BigDog::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Zombie */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Zombie"),
        CMonster_Zombie::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Monster_Priest */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Monster_Priest"),
        CMonster_Priest::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Prototype_GameObject_Boss_MadDonkey */
    if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Boss_MadDonkey"),
        CBoss_MadDonkey::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    //애니메이션이 보장되고나서 풀 생성
    m_AnimBatch.Wait();

    if (m_AnimBatch.IsFailed())
        return E_FAIL;

    HRESULT TrainingResult = m_pGameInstance->Create_Pool<CMonster_Training, 3>();
    if (FAILED(TrainingResult))
        return E_FAIL;

    HRESULT BasicResult = m_pGameInstance->Create_Pool<CButtler_Basic, 13>();
    if (FAILED(BasicResult))
        return E_FAIL;

    HRESULT TrainResult = m_pGameInstance->Create_Pool<CButtler_Train, 7>();
    if (FAILED(TrainResult))
        return E_FAIL;

    HRESULT NoHeadResult = m_pGameInstance->Create_Pool<CButtler_NoHead, 6>();
    if (FAILED(NoHeadResult))
        return E_FAIL;

    HRESULT EliteResult = m_pGameInstance->Create_Pool<CMonster_Elite, 4>();
    if (FAILED(EliteResult))
        return E_FAIL;

    HRESULT RangeResult = m_pGameInstance->Create_Pool<CMonster_Range, 8>();
    if (FAILED(RangeResult))
        return E_FAIL;

    HRESULT BossFestivalResult = m_pGameInstance->Create_Pool<CBoss_Festival, 2>();
    if (FAILED(BossFestivalResult))
        return E_FAIL;

    HRESULT BossRaxasiaP1Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P1, 2>();
    if (FAILED(BossRaxasiaP1Result))
        return E_FAIL;

    HRESULT BossRaxasiaP2Result = m_pGameInstance->Create_Pool<CBoss_Raxasia_P2, 2>();
    if (FAILED(BossRaxasiaP2Result))
        return E_FAIL;

    HRESULT BalistarResult = m_pGameInstance->Create_Pool<CBalistar, 2>();
    if (FAILED(BalistarResult))
        return E_FAIL;

    HRESULT ButterflyResult = m_pGameInstance->Create_Pool<CMonster_Butterfly, 2>();
    if (FAILED(ButterflyResult))
        return E_FAIL;

    HRESULT BigDogResult = m_pGameInstance->Create_Pool<CMonster_BigDog, 5>();
    if (FAILED(BigDogResult))
        return E_FAIL;

    HRESULT ZombieResult = m_pGameInstance->Create_Pool<CMonster_Zombie, 30>();
    if (FAILED(ZombieResult))
        return E_FAIL;

    HRESULT PriestResult = m_pGameInstance->Create_Pool<CMonster_Priest, 5>();
    if (FAILED(PriestResult))
        return E_FAIL;

    HRESULT MadDonkeyResult = m_pGameInstance->Create_Pool<CBoss_MadDonkey, 3>();
    if (FAILED(MadDonkeyResult))
        return E_FAIL;

#pragma endregion 

    lstrcpy(m_szLoadingText, TEXT("보스 맵 로딩이 완료되었습니다."));

    return S_OK;
}
void CLoader::Enqueue_AnimationJob(const _wstring& strFolderName, const _wstring& strBaseName, _bool bStatic, ANIMATION_TIME_UNIT eUnit)
{
	CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    
    if (nullptr == pPool)
    {
        if (FAILED(m_pGameInstance->Ready_RawAnimations(strFolderName ,strBaseName, bStatic, eUnit)))
        {
            m_AnimBatch.Mark_Failed();
        }
        return;
    }

    pPool->Enqueue(&m_AnimBatch, [this, strFolderName, strBaseName, bStatic, eUnit]() -> HRESULT
        {
            return m_pGameInstance->Ready_RawAnimations(strFolderName.c_str(), strBaseName.c_str(), bStatic, eUnit);
		}
    );

}


HRESULT CLoader::Join_And_Register_Models()
{
    if (m_pGameInstance->Is_LoadingCancled())
    {
        Clear_ModelJobs();
        return E_FAIL;
    }

    if(m_ModelBatch.IsFailed())
    {
        Clear_ModelJobs();
        MSG_BOX("Loader Join() : Failed to Load Resources");
        return E_FAIL;
	}

    //현재 로딩된 챕터의 Job만 들어있음 

    for (ModelJob& job : m_GamePlay_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr; 
    }

    for (ModelJob& job : m_Chapter01_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    for (ModelJob& job : m_Chapter02_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    for (ModelJob& job : m_Chapter03_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    for (ModelJob& job : m_TestMap_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    for (ModelJob& job : m_TestObjMap_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    for (ModelJob& job : m_BossMap_ModelJobs)
    {
        if (FAILED(m_pGameInstance->Add_Prototype(job.iLevel, job.strPrototypeTag.c_str(), job.pResult)))
        {
            Clear_ModelJobs();
            return E_FAIL;
        }
        job.pResult = nullptr;
    }

    Clear_ModelJobs();

    return S_OK;
}

void CLoader::Clear_ModelJobs()
{
    for (ModelJob& job : m_GamePlay_ModelJobs)
        Safe_Release(job.pResult);
    m_GamePlay_ModelJobs.clear();

    for (ModelJob& job : m_Chapter01_ModelJobs)
        Safe_Release(job.pResult);
    m_Chapter01_ModelJobs.clear();

    for (ModelJob& job : m_Chapter02_ModelJobs)
        Safe_Release(job.pResult);
    m_Chapter02_ModelJobs.clear();

    for (ModelJob& job : m_Chapter03_ModelJobs)
        Safe_Release(job.pResult);
    m_Chapter03_ModelJobs.clear();

    for (ModelJob& job : m_TestMap_ModelJobs)
        Safe_Release(job.pResult);
    m_TestMap_ModelJobs.clear();

    for (ModelJob& job : m_TestObjMap_ModelJobs)
        Safe_Release(job.pResult);
    m_TestObjMap_ModelJobs.clear();

    for (ModelJob& job : m_BossMap_ModelJobs)
        Safe_Release(job.pResult);
    m_BossMap_ModelJobs.clear();

}



HRESULT CLoader::Ready_ChapterMap(const _wstring& strChapterPath)
{
    //const _wstring strChapterPath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\" + GAMEPLAY_CHAPTER_FILE;
    
    CHAPTER_DESC ChapterDesc{};
    unordered_set<_wstring> LoadedMaps{};
    _wstring strError{};

    if (FAILED(CChapterData::Load(strChapterPath, &ChapterDesc, &strError)))
        return E_FAIL;

    const _bool bCreateEditorRealObjects = LEVEL::EDIT == m_eNextLevelID; 

    for (const auto& Pair : ChapterDesc.Sectors)
    {
        //로딩 중 종료 됐다면 반환 
        if (m_pGameInstance->Is_LoadingCancled())
            return E_FAIL;

        const SECTOR_DESC& Sector = Pair.second;

        if (false == LoadedMaps.emplace(Sector.MapName).second)
            continue;

        CMap* pOriginalMap = nullptr;

        if (FAILED(m_pGameInstance->Load_Map(Sector.MapName, &pOriginalMap, Sector.MapScale, bCreateEditorRealObjects)))
            return E_FAIL;

        CMap* pHLODMap = nullptr;
        const HRESULT hrHLOD = m_pGameInstance->Load_HLODMap(Sector.MapName, &pHLODMap);

        if (FAILED(hrHLOD))
            return E_FAIL;

    }

    for (const _wstring& strMapName : LoadedMaps)
    {
 /*       if (FAILED(m_pGameInstance->Load_MapPhysicsSettings(strMapName)))
            return E_FAIL;*/

        if (FAILED(m_pGameInstance->Load_MapPhysics(strMapName)))
            return E_FAIL;
    }

    const _wstring strChapterNavigationKey = CNavigationManager::Make_ChapterNavigationKey(strChapterPath);
    const _wstring strNavigationFolderPath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\Navigation\\" + strChapterNavigationKey + L"\\";

    for (const auto& Pair : ChapterDesc.Sectors)
    {
        const SECTOR_DESC& Sector = Pair.second;
        const _wstring strNavigationPath = strNavigationFolderPath + L"Navigation_Sector_" + to_wstring(Sector.ID) + L".dat";
        const _wstring strNeighborPath = strNavigationFolderPath + L"Neighbor_Sector_" + to_wstring(Sector.ID) + L".dat";

        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(strNavigationPath.c_str()) || INVALID_FILE_ATTRIBUTES == GetFileAttributesW(strNeighborPath.c_str()))
            continue;

        if (FAILED(m_pGameInstance->Load_Navigation(strChapterNavigationKey, Sector.ID, strNavigationPath, strNeighborPath)))
            return E_FAIL;
    }


    return S_OK;
}



//HRESULT CLoader::Ready_Resources_For_Shader()
//{
//    lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중"));
//#pragma region [ SKY_BOX ]
//    _wstring strSkyTextureName = TEXT("SkyBox");
//    _wstring strSkyTexturePath = m_pGameInstance->Get_ResourcePath() + TEXT("\\Map\\SkyBox\\SkyBox.dds");
//
//    CTexture* pSkyTexture = CTexture::Create(m_pDevice, m_pContext, &strSkyTextureName, &strSkyTexturePath);
//    if (pSkyTexture == nullptr)
//    {
//        MSG_BOX("SkyTexture not Created");
//    }
//    else
//    {
//        if (FAILED(m_pGameInstance->Register_Texture(pSkyTexture)))
//        {
//            Safe_Release(pSkyTexture);
//            return E_FAIL;
//        }
//    }
//
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), CSky::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//    lstrcpy(m_szLoadingText, TEXT("정점, 인덱스 버퍼 로딩 중"));
//
//    lstrcpy(m_szLoadingText, TEXT("컴포넌트 로딩 중"));
//#pragma region [ COMPONENET ]
//    /*Prototype_Component_Movement*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement"), CMovement::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /*Prototype_Component_SpringArm*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_SpringArm"), CSpringArm::Create())))
//        return E_FAIL;
//
//    /*Prototype_Component_CCT*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"), CCT::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /*Prototype_Component_Target*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Target"), CTargetComponent::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//    /*Prototype_Component_Movement_AI*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement_AI"), CMovement_AI::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//    lstrcpy(m_szLoadingText, TEXT("모델 로딩 중"));
//#pragma region [ Player_Weapon ]
//    {
//        _matrix PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_GreatSword"),
//            CModel::Create(m_pDevice, m_pContext, L"GreatSword", PreTransformMatrix))))
//            return E_FAIL;
//    }
//#pragma endregion
//
//#pragma region [ Player_Parts ]
//    /*Prototype_Component_Model_PlayerHelmet*/
//    _matrix PreTransformMatrix = XMMatrixIdentity();
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Acc_Eyepatch"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_EyePatch", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_PlayerHelmet*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Helmet_MadDonkey"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_MadDonkey", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_PlayerBody*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseBody", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_PlayerBody*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody2"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Body2", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_Player_Facial*/
//    PreTransformMatrix = XMMatrixIdentity();
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Facial"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Face", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_Player_ProtoArm*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_ProtoArm"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_ProtoArm", PreTransformMatrix))))
//        return E_FAIL;
//
//    /*Prototype_Component_Model_Player_GrappleGun*/
//    PreTransformMatrix = XMMatrixIdentity();
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_GrappleGun"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_GrappleGun", PreTransformMatrix))))
//        return E_FAIL;
//
//
//    /*Prototype_Component_Model_Player_Hair*/
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Hair"),
//        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseHair", PreTransformMatrix))))
//        return E_FAIL;
//#pragma endregion
//
//#pragma region [ Monsters ]
//
//    /* Buttler */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Buttler_Basic"),
//            CModel::Create(m_pDevice, m_pContext, L"Buttler_Basic", PreTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* Train */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Buttler_Train"),
//            CModel::Create(m_pDevice, m_pContext, L"Buttler_Train", PreTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* NoHead */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Buttler_NoHead"),
//            CModel::Create(m_pDevice, m_pContext, L"Buttler_NoHead", PreTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* Range */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster_Range"),
//            CModel::Create(m_pDevice, m_pContext, L"Monster_Range", PreTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* Elite */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster_Elite"),
//            CModel::Create(m_pDevice, m_pContext, L"Monster_Elite", PreTransformMatrix))))
//            return E_FAIL;
//    }
//    
//    /* Training */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Training"),
//            CModel::Create(m_pDevice, m_pContext, L"Training", PreTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* FestivalLeader */
//    {
//        _matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(270.f));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Festival"),
//            CModel::Create(m_pDevice, m_pContext, L"FestivalLeader", PreTransformMatrix))))
//            return E_FAIL;
//    }
//#pragma endregion
//
//#pragma region [ Monster_Weapon ]
//
//    /* Prototype_Component_Model_FestivalHead*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_FestivalHead"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_Fesitval", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//    /* Prototype_Component_Model_Weapon_Candle*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Candle"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_Candle", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//    /* Prototype_Component_Model_Weapon_Knife*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Knife"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_Knife", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//    /* Prototype_Component_Model_Weapon_Elite*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Elite"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_Elite", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//    /* Prototype_Component_Model_Weapon_Train*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Train"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_Train", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//    /* Prototype_Component_Model_Weapon_ShotGun*/
//    {
//        _matrix PreWeaponTransformMatrix =
//            XMMatrixScaling(0.01f, 0.01f, 0.01f) *
//            XMMatrixRotationX(XMConvertToRadians(-90)) *
//            XMMatrixRotationX(XMConvertToRadians(180)) *
//            XMMatrixRotationY(XMConvertToRadians(45));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_ShotGun"),
//            CModel::Create(m_pDevice, m_pContext, L"Weapon_ShotGun", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//
//    /* Prototype_Component_Model_Bullet*/
//    {
//        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(90));
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Bullet"),
//            CModel::Create(m_pDevice, m_pContext, L"Bullet", PreWeaponTransformMatrix))))
//            return E_FAIL;
//    }
//#pragma endregion
//
//#pragma region Map Model
//    {
//        /*Prototype_Component_Stargazer*/
//        PreTransformMatrix = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f);
//
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer_Broken"),
//            CModel::Create(m_pDevice, m_pContext, L"Stargazer_Broken", PreTransformMatrix))))
//            return E_FAIL;
//
//
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer"),
//            CModel::Create(m_pDevice, m_pContext, L"Stargazer", PreTransformMatrix))))
//            return E_FAIL;
//
//        /*Prototype_Component_Model_HotelDoor*/
//        PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);
//
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_HotelDoor"),
//            CModel::Create(m_pDevice, m_pContext, L"Hotel_Door", PreTransformMatrix))))
//            return E_FAIL;
//
//        PreTransformMatrix = XMMatrixScaling(0.001f, 0.001f, 0.001f);
//        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Hotel_InDoor"),
//            CModel::Create(m_pDevice, m_pContext, L"Hotel_InDoor", PreTransformMatrix))))
//            return E_FAIL;
//
//    }
//#pragma endregion
//
//    lstrcpy(m_szLoadingText, TEXT("맵 로딩 중"));
////#pragma region [ Map Loading ]
////    if (FAILED(Ready_ChapterMap_For_GamePlay()))
////        return E_FAIL;
//#pragma endregion
//    lstrcpy(m_szLoadingText, TEXT("객체원형 로딩 중"));
//#pragma region [ Player_Weapon ]
//    /* Prototype_Weapon_GreatSword */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Weapon_GreatSword"),
//        CGreatSword::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//
//#pragma region [ Player_Part ]
//    /* Prototype_GameObject_BodyPart */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_BodyPart"),
//        CBodyPart::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Player */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
//        CPlayer::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//
//#pragma region [ Enviroment ]
//    /* Prototype_GameObject_FogVolume */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_FogVolume"),
//        CFogVolume::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//
//#pragma region [ Monster ]
//    /* Prototype_GameObject_Monster_Training */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Training"),
//        CMonster_Training::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Boss_Festival */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Festival"),
//        CBoss_Festival::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Monster_Buttler */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Buttler"),
//        CMonster_Buttler::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Monster_Elite */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Elite"),
//        CMonster_Elite::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Monster_Range */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Range"),
//        CMonster_Range::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//
//#pragma region [ Monster Weapon ]
//
//    /* Prototype_GameObject_Weapon_Melee */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Melee"),
//        CWeapon_Melee::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_Weapon_Range */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Range"),
//        CWeapon_Range::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    HRESULT hr = m_pGameInstance->Create_Pool<CWeapon_Bullet, 5>();
//    if (FAILED(hr))
//        return E_FAIL;
//
//#pragma endregion
//    lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));
//
//#pragma region [ Map Object ]
//    /* Prototype_GameObject_Stargazer */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Stargazer"),
//        CStargazer::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /* Prototype_GameObject_HotelDoor */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_HotelDoor"),
//        CHotelDoor::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//
//    /*Prototype_GameObject_Trigger */
//    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Trigger"),
//        CTrigger::Create(m_pDevice, m_pContext))))
//        return E_FAIL;
//#pragma endregion
//
//
//    //m_isFinished = true;
//
//    return S_OK;
//}


HRESULT CLoader::Ready_Resources_For_Chapter(_uint iChapterIndex)
{
    if (iChapterIndex >= g_iMaxChapter) return E_FAIL;

    //이미 로딩된 챕터면 SOK반환 
    if (Is_ChapterCached(iChapterIndex)) return S_OK;

    HRESULT hr = S_OK;

    switch (iChapterIndex)
    {
    case 0: hr = Ready_Resources_For_Chapter01(); break;
    case 1: hr = Ready_Resources_For_Chapter02(); break;
    case 2: hr = Ready_Resources_For_Chapter03(); break;
    case 3: hr = Ready_Resources_For_ChapterTest(); break;
    case 4: hr = Ready_Resources_For_ChapterTestObj(); break;
    case 5: hr = Ready_Resources_For_ChapterBossMap(); break;
    default:
        break;
    }

    if (FAILED(hr))
    {
        MSG_BOX("Loader : Chapter Load Failed");
        return E_FAIL;
    }

    lstrcpy(m_szLoadingText, TEXT("맵 리소스 로딩 중"));

    //_wstring strChapterPath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\" + g_ChapterFiles[iChapterIndex];
    _wstring strChapterPath{};
    if (FAILED(CChapterMapManager::Get_ChapterPath(iChapterIndex, strChapterPath))) return E_FAIL;
    if (FAILED(Ready_ChapterMap(strChapterPath))) return E_FAIL;

    g_LoadedChapters.emplace(iChapterIndex);
    return S_OK;

}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd)
{
    CLoader* pInstance = new CLoader(pDevice, pContext, hWnd);

    if (FAILED(pInstance->Initialize(eNextLevelID)))
    {
        MSG_BOX("Failed to Created : CLoader(LevelID)");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iChapterIndex, HWND hWnd)
{
	CLoader* pInstance = new CLoader(pDevice, pContext, hWnd);
    if (FAILED(pInstance->Initialize(iChapterIndex)))
    {
        MSG_BOX("Failed to Created : CLoader(ChapterIndex)");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLoader::Free()
{
    __super::Free();
    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, INFINITE);

        DeleteCriticalSection(&m_CriticalSection);

        CloseHandle(m_hThread);
    }

    Clear_ModelJobs();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}


//기존 게임플레이 로딩 코드(멀티스레딩 전)
/*  //기존 코드 


//게임플레이 전역적으로 필요한 리소스 (플레이어, 컴포넌트, 장비, 맵 오브젝트 등 )
HRESULT CLoader::Ready_Resources_For_GamePlay()
{
    lstrcpy(m_szLoadingText, TEXT("GamePlay : 텍스쳐 로딩 중"));

    _wstring strSkyTextureName = TEXT("SkyBox");
    _wstring strSkyTexturePath = m_pGameInstance->Get_ResourcePath() + TEXT("\\Map\\SkyBox\\SkyBox.dds");

    CTexture* pSkyTexture = CTexture::Create(m_pDevice, m_pContext, &strSkyTextureName, &strSkyTexturePath);
    if (pSkyTexture == nullptr)
    {
        MSG_BOX("SkyTexture not Created");
    }
    else
    {
        if (FAILED(m_pGameInstance->Register_Texture(pSkyTexture)))
        {
            Safe_Release(pSkyTexture);
            return E_FAIL;
        }
    }

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), CSky::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 컴포넌트 로딩 중"));

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement"), CMovement::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_SpringArm"), CSpringArm::Create())))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Target"), CTargetComponent::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"), CCT::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement_AI"), CMovement_AI::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 모델 로딩 중"));
    vector<ModelJob> ModelJobs;

    _matrix PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90.f));
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_GreatSword"),
        CModel::Create(m_pDevice, m_pContext, L"GreatSword", PreTransformMatrix))))
        return E_FAIL;

    PreTransformMatrix = XMMatrixIdentity();
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Acc_Eyepatch"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_EyePatch", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Helmet_MadDonkey"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Helmet_MadDonkey", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseBody", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_PlayerBody2"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Body2", PreTransformMatrix))))
        return E_FAIL;

    PreTransformMatrix = XMMatrixIdentity();
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Facial"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Face", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_ProtoArm"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_ProtoArm", PreTransformMatrix))))
        return E_FAIL;

    PreTransformMatrix = XMMatrixIdentity();
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_GrappleGun"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_Arm_GrappleGun", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Hair"),
        CPartsModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_BaseHair", PreTransformMatrix))))
        return E_FAIL;


    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_FestivalHead"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_Fesitval", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Candle"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_Candle", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Knife"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_Knife", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Elite"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_Elite", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(-90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_Train"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_Train", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    {
        _matrix PreWeaponTransformMatrix =
            XMMatrixScaling(0.01f, 0.01f, 0.01f) *
            XMMatrixRotationX(XMConvertToRadians(-90)) *
            XMMatrixRotationX(XMConvertToRadians(180)) *
            XMMatrixRotationY(XMConvertToRadians(45));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Weapon_ShotGun"),
            CModel::Create(m_pDevice, m_pContext, L"Weapon_ShotGun", PreWeaponTransformMatrix))))
            return E_FAIL;
    }
    PreTransformMatrix = XMMatrixScaling(0.0001f, 0.0001f, 0.0001f);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer_Broken"),
        CModel::Create(m_pDevice, m_pContext, L"Stargazer_Broken", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Stargazer"),
        CModel::Create(m_pDevice, m_pContext, L"Stargazer", PreTransformMatrix))))
        return E_FAIL;

    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_HotelDoor"),
        CModel::Create(m_pDevice, m_pContext, L"Hotel_Door", PreTransformMatrix))))
        return E_FAIL;

    PreTransformMatrix = XMMatrixScaling(0.001f, 0.001f, 0.001f);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Hotel_InDoor"),
        CModel::Create(m_pDevice, m_pContext, L"Hotel_InDoor", PreTransformMatrix))))
        return E_FAIL;

    {
        _matrix PreWeaponTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationX(XMConvertToRadians(90));
        if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Bullet"),
            CModel::Create(m_pDevice, m_pContext, L"Bullet", PreWeaponTransformMatrix))))
            return E_FAIL;
    }

    lstrcpy(m_szLoadingText, TEXT("게임플레이 Level : 객체원형 로딩 중"));

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Weapon_GreatSword"),
        CGreatSword::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_BodyPart"),
        CBodyPart::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_CameraFree"),
        CCamera_Free::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
        CPlayer::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Melee"),
        CWeapon_Melee::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Range"),
        CWeapon_Range::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    HRESULT hr = m_pGameInstance->Create_Pool<CWeapon_Bullet, 5>();
    if (FAILED(hr))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_FogVolume"),
        CFogVolume::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Stargazer"),
        CStargazer::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_HotelDoor"),
        CHotelDoor::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Trigger"),
        CTrigger::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    lstrcpy(m_szLoadingText, TEXT("게임플레이 로딩이 완료되었습니다."));
    return S_OK;
}

*/
