#include "Level_Shader.h"
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

#include "FogVolume.h"
#include "CCT.h"
#include "UIFactory.h"
#include "Status.h"
#include "StatComponent.h"
#include "StaticRigidBody.h"
#include "Trigger.h"

#include "Loader.h"

#include "Monster_Include.h"

#ifdef _DEBUG
#include "NavigationDebugLineBuffer.h"
#include "NavigationDebugRenderer.h"
#endif

CLevel_Shader::CLevel_Shader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
	: CLevel{ pDevice, pContext, hWnd }
{
}

HRESULT CLevel_Shader::Initialize()
{
	__super::Initialize();

	m_iCurrentChapter = GAMEPLAY_START_CHAPTER;


	if (FAILED(Ready_Effect()))
		return E_FAIL;

	if (FAILED(Rendy_Layer_Sky(TEXT("Layer_Sky"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Map(TEXT("Layer_Map"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Character(TEXT("Layer_Character"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Monster(TEXT("Layer_Monster"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Fog(TEXT("Layer_Fog"))))
		return E_FAIL;

	if (FAILED(Ready_UI()))
		return E_FAIL;

#ifdef _DEBUG
	//피직스 콜라이더 랜더러 생성 
	if (FAILED(Ready_DebugRenderer()))
		return E_FAIL;

	if (FAILED(Ready_NavigationDebugRenderer()))
		return E_FAIL;
#endif //DEBUG


	m_pGameInstance->Play_BGM("AMB_SS_StreetStartPoint", 0.3f);

	m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_RESPAWN, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_CHANGE_CHAPTER, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::UPDATE_RESPAWN_POINT, this);


	return S_OK;
}

void CLevel_Shader::Update(_float fTimeDelta)
{
	Update_ChapterState(fTimeDelta);

	//다음 챕터로 전환
	if (m_pGameInstance->Key_Down(DIK_6))
	{
		Respawn_Point Enter{};
		Enter.iChapterIndex = (m_iCurrentChapter + 1) % g_iMaxChapter;
		Enter.vPosition = g_ChapterPlayer_StartPos[Enter.iChapterIndex];
		Request_ChangeChapter(Enter.iChapterIndex, Enter);
	}
	//현재 맵에서 체크포인트로 리스폰 
	if (m_pGameInstance->Key_Down(DIK_7))
		Request_Respawn();

	__super::Update(fTimeDelta);

#ifdef _DEBUG
	if (m_pGameInstance->Key_Down(DIK_8) && nullptr != m_pNavigationDebugRenderer)
		m_pNavigationDebugRenderer->Toggle_Enable();
#endif
}

void CLevel_Shader::Late_Update(_float fTimeDelta)
{
	//Active시에만 MaterialBuffer 업데이트
	if (m_eChapterState == CHAPTER_STATE::ACTIVE)
		__super::Late_Update(fTimeDelta);

	if (nullptr == m_pChapterMapManager || false == m_pChapterMapManager->Is_ChapterLoaded())
		return;

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

#ifdef _DEBUG
	if (nullptr != m_pNavigationDebugRenderer)
	{
		m_pNavigationDebugRenderer->Update_DebugLines(m_pChapterMapManager);
		m_pNavigationDebugRenderer->Late_Update(fTimeDelta);
	}
#endif
}

HRESULT CLevel_Shader::Render()
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

HRESULT CLevel_Shader::Rendy_Layer_Sky(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), ETOUI(LEVEL::SHADER), TEXT("Layer_Sky"))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Bake_IBL()))
		return E_FAIL;


	return S_OK;
}

HRESULT CLevel_Shader::Ready_Layer_Character(const _wstring& strLayerTag)
{
	CTransform::TRANSFORM_DESC desc;

	desc.vTranslate = g_ChapterPlayer_StartPos[GAMEPLAY_START_CHAPTER];
	desc.fRotationPerSec = XMConvertToRadians(720.f);
	desc.fSpeedPerSec = 10.f;

	m_RespawnPoint.iChapterIndex = GAMEPLAY_START_CHAPTER;
	m_RespawnPoint.vPosition = g_ChapterPlayer_StartPos[GAMEPLAY_START_CHAPTER];

	if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
		ETOUI(LEVEL::SHADER), strLayerTag, &desc)))
		return E_FAIL;

	CCharacter* pPlayer = static_cast<CCharacter*>(m_pGameInstance->Get_Layer(ETOUI(LEVEL::SHADER), strLayerTag)->Get_Objects().front());
	m_pGameInstance->Set_Player(pPlayer);

	return S_OK;
}


HRESULT CLevel_Shader::Ready_Layer_Monster(const _wstring& strLayerTag)
{
	if (nullptr != m_pChapterMapManager)
	{
		if (FAILED(m_pChapterMapManager->Spawn_All()))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevel_Shader::Ready_Layer_Map(const _wstring& strLayerTag)
{
	CHAPTER_DESC ChapterDesc{};
	_wstring strError{};

	m_pChapterMapManager = CChapterMapManager::Create(m_pDevice, m_pContext);

	if (nullptr == m_pChapterMapManager)
		return E_FAIL;

	_wstring strChapterPath = {};
	if (FAILED(CChapterMapManager::Get_ChapterPath(GAMEPLAY_START_CHAPTER, strChapterPath))) return E_FAIL;

	if (FAILED(m_pChapterMapManager->Load_Chapter(strChapterPath, LEVEL::SHADER, Chapter_To_Level(GAMEPLAY_START_CHAPTER))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_Shader::Ready_Layer_Fog(const _wstring& strLayerTag)
{
	return S_OK;
}

HRESULT CLevel_Shader::Ready_Effect()
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

#pragma region [COMMON EFFECT]

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
	if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Mine", strRoot + "Test_Mine.json")))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Sector", strRoot + "Test_Sector.json")))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Bolt_Range", strRoot + "Test_Range.json")))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Load_Effect_File("Effect_Weapon_Spark", strRoot + "Raxasia_Sword_Spark.json")))
		return E_FAIL;

#pragma endregion
	return S_OK;
}

HRESULT CLevel_Shader::Ready_UI()
{
	return CUIFactory::Ready_GameplayHUDs();
}

void CLevel_Shader::m_TriggerCallbackFunc(PxTriggerPair* pairs, PxU32 count)
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

void CLevel_Shader::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
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
	}
}


void CLevel_Shader::Level_Exit()
{
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

void CLevel_Shader::Request_ChangeChapter(_uint iChapterIndex, const Respawn_Point& SpawnPoint)
{
	//전환 중 요청은 무시 
	if (Can_RequestTransition() == false) return;

	if (iChapterIndex >= g_iMaxChapter || iChapterIndex == m_iCurrentChapter) return;

	Begin_FadeOut(TRANSITION_KIND::CHANGE_CHAPTER);

	m_iPendingChapter = iChapterIndex;
	m_PendingSpawnPoint = SpawnPoint;

}

void CLevel_Shader::Request_Respawn()
{
	//전환 중 요청은 무시 
	if (Can_RequestTransition() == false) return;

	Begin_FadeOut(TRANSITION_KIND::RESPAWN);
}

void CLevel_Shader::Update_ChapterState(_float fTimeDelta)
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


void CLevel_Shader::State_FadeOut(_float fTimeDelta)
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

void CLevel_Shader::State_Unload()
{
	m_pChapterMapManager->Unload_Chapter();
	m_eChapterState = CHAPTER_STATE::LOAD;
}

void CLevel_Shader::State_Load()
{
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

void CLevel_Shader::State_LoadWait()
{
	if (m_pChapterLoader == nullptr) return;
	if (m_pChapterLoader->isFinished() == false) return;

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

void CLevel_Shader::State_Create()
{
	_wstring strChapterPath;
	if (FAILED(CChapterMapManager::Get_ChapterPath(m_iPendingChapter, strChapterPath)))
	{
		MSG_BOX("Level Gameplay : Chapter Path Not Found");
		Begin_FadeIn();
		return;
	}

	if (FAILED(m_pChapterMapManager->Load_Chapter(strChapterPath, LEVEL::SHADER, Chapter_To_Level(m_iPendingChapter))))
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

	m_pChapterMapManager->Spawn_All();

	//챕터 전환용 초기화
	ResetPlayer_ForChangeMap(m_PendingSpawnPoint);
	//Clear가 lock을 풀어서 재잠금
	Lock_PlayerInput(true);

	//체크포인트를 챕터 변경 시점에 갱신
	m_RespawnPoint = m_PendingSpawnPoint;
	m_iCurrentChapter = m_iPendingChapter;
	m_iPendingChapter = UINT_MAX;

	Begin_FadeIn();

}

void CLevel_Shader::State_Resetting()
{
	Respawn_Player(m_RespawnPoint);
	Lock_PlayerInput(true);

	m_pChapterMapManager->Reset_Chapter();

	Begin_FadeIn();
}

void CLevel_Shader::State_FadeIn(_float fTimeDelta)
{
	m_fFadeTimer -= fTimeDelta;
	if (m_fFadeTimer > 0.f) return;

	Lock_PlayerInput(false);

	m_eTransitionKind = TRANSITION_KIND::NONE;
	m_eChapterState = CHAPTER_STATE::ACTIVE;

}

void CLevel_Shader::Begin_FadeOut(TRANSITION_KIND eKind)
{
	m_eTransitionKind = eKind;
	m_eChapterState = CHAPTER_STATE::FADE_OUT;
	m_fFadeTimer = FADE_DURATION;

	Lock_PlayerInput(true);
}

void CLevel_Shader::Begin_FadeIn()
{
	m_pGameInstance->Start_MainTimer();
	m_eChapterState = CHAPTER_STATE::FADE_IN;
	m_fFadeTimer = FADE_DURATION;
}

void CLevel_Shader::Respawn_Player(const Respawn_Point& Point)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
	pPlayer->Respawn(Point);

	_vector vSpawnPosition = XMLoadFloat4(&Point.vPosition);

	if (m_pChapterMapManager)
		m_pChapterMapManager->Refresh_CurrentSector(vSpawnPosition);

}

void CLevel_Shader::ResetPlayer_ForChangeMap(const Respawn_Point& Point)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
	pPlayer->Reset_ForChangeMap(Point);

	_vector vSpawnPosition = XMLoadFloat4(&Point.vPosition);

	if (m_pChapterMapManager)
		m_pChapterMapManager->Refresh_CurrentSector(vSpawnPosition);
}

void CLevel_Shader::Lock_PlayerInput(_bool bLock)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
	if (nullptr == pPlayer)
		return;

	pPlayer->Get_StatusComponent()->Set_InputLock(bLock);
	pPlayer->Get_StatusComponent()->Set_CamMoveLock(bLock);
	pPlayer->Get_StatusComponent()->Set_RotationLock(bLock);
}




#ifdef _DEBUG

HRESULT CLevel_Shader::Ready_DebugRenderer()
{
	if (FAILED(m_pGameInstance->Create_VisualDebugger(m_pDevice, m_pContext)))
		return E_FAIL;
	return S_OK;
}

void CLevel_Shader::Render_DebugWindow()
{
}
HRESULT CLevel_Shader::Ready_NavigationDebugRenderer()
{
	m_pNavigationDebugRenderer = CNavigationDebugRenderer::Create(m_pDevice, m_pContext);

	if (nullptr == m_pNavigationDebugRenderer)
		return E_FAIL;

	return S_OK;
}
#endif // DEBUG

CLevel_Shader* CLevel_Shader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
{
	CLevel_Shader* pInstance = new CLevel_Shader(pDevice, pContext, hWnd);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Shader");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLevel_Shader::Free()
{
	m_pGameInstance->UnSubscribe_AllEvent(this);

#ifdef _DEBUG
	Safe_Release(m_pNavigationDebugRenderer);
#endif

	Safe_Release(m_pChapterLoader);
	Safe_Release(m_pChapterMapManager);
	__super::Free();
}

