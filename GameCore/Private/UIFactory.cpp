#include "UIFactory.h"
#include "GameInstance.h"
#include "Player.h"
#include "StatComponent.h"
#include "Equipment.h"
#include "UIPlayerHUDController.h"
#include "UILockOnController.h"
#include "UIBossHUDController.h"
#include "UIMonsterHUDController.h"
#include "UIInteractionPromptController.h"
#include "UIESCMenuController.h"
#include "UINPCDialogueController.h"
#include "UIStargazerController.h"
#include "UIStargazerTravelController.h"
#include "UIEquipmentController.h"
#include "UILoadingController.h"
#include "UILegionArmChangeController.h"
#include "UILegionArmUpgradeController.h"
#include "UIShopController.h"
#include "UIConfirmationWindowController.h"
#include "Status.h"
#include "UIFontCatalog.h"
#include "UICrosshairHUDController.h"
#include "UIBossClearHUDController.h"
#include "UIPlayerDeadController.h"
#include "UIItemPopupController.h"
#include "Engine_Utility.h"
#include "UITitleController.h"

HRESULT CUIFactory::Ready_GameplayHUDs()
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (FAILED(Register_ControllerFactories()))
		return E_FAIL;

	const vector<UI_FONT_CATALOG_ENTRY>& FontCatalog = CUIFontCatalog::Get_Entries();

	for (const UI_FONT_CATALOG_ENTRY& FontEntry : FontCatalog)
	{
		if (pGameInstance->Contains_UIFont(FontEntry.strFontAssetID))
			continue;

		const _wstring strRuntimeFontPath = pGameInstance->Get_ResourcePath() + L"\\" + StringToWString(FontEntry.strRuntimeSpriteFontPath);

		// Register_UIFont는 경로만 등록한다. 실제 SpriteFont는 해당 ID가 처음 사용될 때 생성된다.

		if (FAILED(pGameInstance->Register_UIFont(FontEntry.strFontAssetID, strRuntimeFontPath)))
		{
			const _wstring strError = L"[UI Font Catalog] Failed to register: " + StringToWString(FontEntry.strFontAssetID) + L"\n";

			OutputDebugStringW(strError.c_str());

			return E_FAIL;
		}
	}

	const _wstring strAssetRoot = pGameInstance->Get_ResourcePath() + L"\\UI\\Runtime_Asset\\";

	size_t iLoadedAssetCount = 0;
	_wstring strAssetLoadError;

	// Runtime_Asset 폴더의 모든 v2 UI 에셋을 검증 후 일괄 등록한다.
	if (FAILED(pGameInstance->Register_UIAssetDirectory(strAssetRoot, &iLoadedAssetCount, &strAssetLoadError)))
	{
		OutputDebugStringW((L"[UI Asset Directory Error] " + strAssetLoadError + L"\n").c_str());

		return E_FAIL;
	}

	UI_HANDLE hPlayerHUD = {};
	UI_HANDLE hLockOn = {};
	UI_HANDLE hBossHUD = {};
	UI_HANDLE hInteractionPrompt = {};
	UI_HANDLE hESCMenu = {};
	UI_HANDLE hNPCDialogue = {};
	UI_HANDLE hStargazer = {};
	UI_HANDLE hStargazerTravel = {};
	UI_HANDLE hEquipment = {};
	UI_HANDLE hLegionArmChange = {};
	UI_HANDLE hLegionArmUpgrade = {};
	UI_HANDLE hShop = {};
	UI_HANDLE hConfirmationWindow = {};
	UI_HANDLE hCrosshairHUD = {};
	UI_HANDLE hBossClearHUD = {};
	UI_HANDLE hPlayerDead = {};
	UI_HANDLE hItemPopup{};

	// 플레이어 HUD
	if (FAILED(Create_SingletonUI("PlayerHUD", L"GameHUD_PlayerHUD", &hPlayerHUD)))
	{
		return E_FAIL;
	} 

	// 락온
	if (FAILED(Create_SingletonUI("LockOn", L"GameHUD_LockOn", &hLockOn)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 보스
	if (FAILED(Create_SingletonUI("BossHUD", L"GameHUD_BossHUD", &hBossHUD)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 알림창
	if (FAILED(Create_SingletonUI("HUD_InteractionPrompt", L"GameHUD_InteractionPrompt", &hInteractionPrompt)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// ESC 메뉴
	if (FAILED(Create_SingletonUI("HUD_EscapeMenu", L"GameHUD_EscapeMenu", &hESCMenu)) || FAILED(pGameInstance->Close_UIObject(hESCMenu)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 대화창
 	if (FAILED(Create_SingletonUI("HUD_NPCDialogue", L"GameHUD_NPCDialogue", &hNPCDialogue)) || FAILED(pGameInstance->Close_UIObject(hNPCDialogue)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 별바라기 상호작용
	if (FAILED(Create_SingletonUI("HUD_Stargazer", L"GameHUD_Stargazer", &hStargazer)) || FAILED(pGameInstance->Close_UIObject(hStargazer)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 장비창
	if (FAILED(Create_SingletonUI("HUD_Equipment", L"GameHUD_Equipment", &hEquipment)) || FAILED(pGameInstance->Close_UIObject(hEquipment)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 리전 암 교체 UI
	if (FAILED(Create_SingletonUI("HUD_LegionArmChange", L"GameHUD_LegionArmChange", &hLegionArmChange)) || FAILED(pGameInstance->Close_UIObject(hLegionArmChange)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 리전 암 강화
	if (FAILED(Create_SingletonUI("HUD_LegionArmUpgrade", L"GameHUD_LegionArmUpgrade", &hLegionArmUpgrade)) || FAILED(pGameInstance->Close_UIObject(hLegionArmUpgrade)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 상점
	if (FAILED(Create_SingletonUI("HUD_Shop", L"GameHUD_Shop", &hShop)) || FAILED(pGameInstance->Close_UIObject(hShop)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 확인창
	if (FAILED(Create_SingletonUI("HUD_ConfirmationWindow", L"GameHUD_ConfirmationWindow", &hConfirmationWindow)) || FAILED(pGameInstance->Close_UIObject(hConfirmationWindow)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 별바라기 이동 UI
	if (FAILED(Create_SingletonUI("HUD_StargazerTravel", L"GameHUD_StargazerTravel", &hStargazerTravel)) || FAILED(pGameInstance->Close_UIObject(hStargazerTravel)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 리전 암 조준점 HUD
	if (FAILED(Create_SingletonUI("CrosshairHUD", L"GameHUD_CrosshairHUD", &hCrosshairHUD)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 보스 클리어 연출 HUD
	if (FAILED(Create_SingletonUI("BossClearHUD", L"GameHUD_BossClearHUD", &hBossClearHUD)) || FAILED(pGameInstance->Close_UIObject(hBossClearHUD)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	// 플레이어 사망 연출 UI
	if (FAILED(Create_SingletonUI("PlayerDead", L"GameHUD_PlayerDead", &hPlayerDead)) || FAILED(pGameInstance->Close_UIObject(hPlayerDead)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	if (FAILED(Create_SingletonUI("ItemPopupWindow", L"GameHUD_ItemPopupWindow", &hItemPopup)) || FAILED(pGameInstance->Close_UIObject(hItemPopup)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	const UI_ASSET_DESC* pMonsterHUDAsset = pGameInstance->Find_UIAsset("MonsterHUD");

	if (pMonsterHUDAsset == nullptr)
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	UI_CONTROLLER_CREATE_CONTEXT MonsterContext = {};

	MonsterContext.strAssetID = "MonsterHUD";
	MonsterContext.strControllerType = pMonsterHUDAsset->strControllerType;

	// 특정 인스턴스 하나에 연결되지 않는 공유 컨트롤러
	MonsterContext.hUI.Invalidate();

	MonsterContext.strInstancePrefix = L"GameHUD_MonsterHUD";

	if (FAILED(pGameInstance->Create_UIController(MonsterContext)))
	{
		pGameInstance->Request_ReleaseUILifetime(UI_LIFETIME::GAMEPLAY);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CUIFactory::Ready_LoadingUI(UI_HANDLE* pOutHandle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (FAILED(Register_ControllerFactories()))
		return E_FAIL;

	const vector<UI_FONT_CATALOG_ENTRY>& FontCatalog = CUIFontCatalog::Get_Entries();

	for (const UI_FONT_CATALOG_ENTRY& FontEntry : FontCatalog)
	{
		if (pGameInstance->Contains_UIFont(FontEntry.strFontAssetID))
			continue;

		const _wstring strRuntimeFontPath = pGameInstance->Get_ResourcePath() + L"\\" + StringToWString(FontEntry.strRuntimeSpriteFontPath);

		if (FAILED(pGameInstance->Register_UIFont(FontEntry.strFontAssetID, strRuntimeFontPath)))
			return E_FAIL;
	}

	if (!pGameInstance->Contains_UIAsset("HUD_Loading"))
	{
		const _wstring strLoadingAssetPath = pGameInstance->Get_ResourcePath() + L"\\UI\\Runtime_Asset\\Loading.ui.json";
		_wstring strAssetLoadError;

		if (FAILED(pGameInstance->Register_UIAsset(strLoadingAssetPath, nullptr, &strAssetLoadError)))
		{
			OutputDebugStringW((L"[Loading UI Asset Error] " + strAssetLoadError + L"\n").c_str());
			return E_FAIL;
		}
	}

	UI_HANDLE LocalHandle = {};
	UI_HANDLE* pResultHandle = pOutHandle != nullptr ? pOutHandle : &LocalHandle;

	// 이미 생성된 로딩 UI라면 기존 컨트롤러를 유지한 채 화면만 다시 연다.
	if (pGameInstance->Find_UIObject(L"Loading_UI") != nullptr)
		return pGameInstance->Open_UIObject("HUD_Loading", L"Loading_UI", pResultHandle);

	// 최초 호출에서만 UI와 컨트롤러를 함께 생성한다.
	return Create_SingletonUI("HUD_Loading", L"Loading_UI", pResultHandle);
}

HRESULT CUIFactory::Ready_TitleUI(UI_HANDLE* pOutHandle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (FAILED(Register_ControllerFactories()))
		return E_FAIL;

	const vector<UI_FONT_CATALOG_ENTRY>& FontCatalog = CUIFontCatalog::Get_Entries();

	for (const UI_FONT_CATALOG_ENTRY& FontEntry : FontCatalog)
	{
		if (pGameInstance->Contains_UIFont(FontEntry.strFontAssetID))
			continue;

		const _wstring strFontPath = pGameInstance->Get_ResourcePath() + L"\\" + StringToWString(FontEntry.strRuntimeSpriteFontPath);

		if (FAILED(pGameInstance->Register_UIFont(FontEntry.strFontAssetID, strFontPath)))
			return E_FAIL;
	}

	// Title이 참조하는 컴포넌트를 먼저 등록한다.
	if (!pGameInstance->Contains_UIAsset("TitleButton"))
	{
		const _wstring strButtonPath = pGameInstance->Get_ResourcePath() + L"\\UI\\Runtime_Asset\\TiitleButtonSlot.ui.json";

		if (FAILED(pGameInstance->Register_UIAsset(strButtonPath)))
			return E_FAIL;
	}

	if (!pGameInstance->Contains_UIAsset("Title"))
	{
		const _wstring strTitlePath = pGameInstance->Get_ResourcePath() + L"\\UI\\Runtime_Asset\\Title.ui.json";

		if (FAILED(pGameInstance->Register_UIAsset(strTitlePath)))
			return E_FAIL;
	}

	return Create_SingletonUI("Title", L"Logo_Title", pOutHandle);
}

HRESULT CUIFactory::Create_SingletonUI(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	UI_HANDLE Handle = {};

	if (FAILED(pGameInstance->Open_UIObject(strAssetID, strInstanceTag, &Handle)))
		return E_FAIL;

	if (FAILED(Create_Controller(strAssetID, Handle)))
	{
		pGameInstance->Request_ReleaseUIObject(Handle);

		return E_FAIL;
	}

	if (pOutHandle != nullptr)
		*pOutHandle = Handle;

	return S_OK;
}

HRESULT CUIFactory::Register_ControllerFactories()
{
	// Gameplay Ready가 다시 호출 되도 중복 등록 X
	static _bool bRegistered = false;

	if (bRegistered)
		return S_OK;

	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (FAILED(pGameInstance->Register_UIControllerFactory("PlayerHUDController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_PlayerHUDController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("LockOnController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			CGameInstance* pGameInstance = CGameInstance::Get_Instance();
			if (pGameInstance == nullptr)
				return E_FAIL;

			const UI_ASSET_DESC* pAssetDesc = pGameInstance->Find_UIAsset(Context.strAssetID);
			if (pAssetDesc == nullptr)
				return E_FAIL;

			return CUIFactory::Create_LockOnController(Context.hUI, pAssetDesc->Projected.vScreenOffset);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("BossHUDController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_BossHUDController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("MonsterHUDController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			CGameInstance* pGameInstance = CGameInstance::Get_Instance();
			if (pGameInstance == nullptr)
				return E_FAIL;

			const UI_ASSET_DESC* pAssetDesc = pGameInstance->Find_UIAsset(Context.strAssetID);
			if (pAssetDesc == nullptr)
				return E_FAIL;

			return CUIFactory::Create_MonsterHUDController(Context.strAssetID, Context.strInstancePrefix,  pAssetDesc->Projected.vScreenOffset);

		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("InteractionPrompt", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
			{
				return CUIFactory::Create_InteractionPromptController(Context.hUI);
			})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("EscapeMenuController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
			{
				return CUIFactory::Create_ESCMenuController(Context.hUI);
			})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("NPCDialogueController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
			{
				return CUIFactory::Create_NPCDialogueController(Context.hUI);
			})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("StargazerController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_StargazerController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("StargazerTravelController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_StargazerTravelController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("EquipmentController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_EquipmentController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("LegionArmChangeController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_LegionArmChangeController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("LoadingController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_LoadingController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("LegionArmUpgradeController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_LegionArmUpgradeController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("ShopController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_ShopController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("ConfirmationWindowController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_ConfirmationWindowController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("CrosshairHUDController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_CrosshairHUDController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("BossClearHUDController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_BossClearHUDController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("PlayerDeadController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_PlayerDeadController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("TitleController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_TitleController(Context.hUI);
		})))
	{
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIControllerFactory("ItemPopupController", [](const UI_CONTROLLER_CREATE_CONTEXT& Context)
		{
			return CUIFactory::Create_ItemPopupController(Context.hUI);
		})))
		return E_FAIL;

	bRegistered = true;

	return S_OK;
}

HRESULT CUIFactory::Create_Controller(const string& strAssetID, const UI_HANDLE& Handle)
{
	if (strAssetID.empty() || !Handle.Is_Valid())
		return E_FAIL;

	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	const UI_ASSET_DESC* pAssetDesc = pGameInstance->Find_UIAsset(strAssetID);

	if (pAssetDesc == nullptr)
		return E_FAIL;

	if (pAssetDesc->strControllerType.empty())
		return S_OK;

	UI_CONTROLLER_CREATE_CONTEXT Context = {};

	Context.strAssetID = strAssetID;
	Context.strControllerType = pAssetDesc->strControllerType;
	Context.hUI = Handle;

	return pGameInstance->Create_UIController(Context);
}

HRESULT CUIFactory::Create_LoadingController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUILoadingController* pController = CUILoadingController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"LoadingController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_PlayerHUDController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();
	if (pGameInstance == nullptr)
		return E_FAIL;

	// 컨트롤러를 만들기 전에 빌더가 뷰를 생성했는지 확인
	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CPlayer* pPlayer = dynamic_cast<CPlayer*>(pGameInstance->Get_Player());
	if (pPlayer == nullptr)
		return E_FAIL;

	CStatComponent* pStatCom = pPlayer->Get_StatComponent();
	if (pStatCom == nullptr)
		return E_FAIL;

	CEquipment* pEquipmentCom = pPlayer->Get_EquipmentComponent();
	if (pEquipmentCom == nullptr)
		return E_FAIL;

	CUIPlayerHUDController::PLAYERHUD_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;
	Desc.pStatCom = pStatCom;
	Desc.pEquipment = pEquipmentCom;
	Desc.fHPDelaySpeed = 1.5f;

	CUIPlayerHUDController* pController = CUIPlayerHUDController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"PlayerHUDController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_LockOnController(const UI_HANDLE& Handle, const _float2& vScreenOffset)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CPlayer* pPlayer = dynamic_cast<CPlayer*>(pGameInstance->Get_Player());
	if (pPlayer == nullptr)
		return E_FAIL;

	CStatusComponent* pStatusCom = pPlayer->Get_StatusComponent();
	if (pStatusCom == nullptr)
		return E_FAIL;

	CUILockOnController::LOCKON_CONTROLLER_DESC Desc = {};

	Desc.hUI = Handle;
	Desc.pStatusCom = pStatusCom;
	Desc.vScreenOffset = vScreenOffset;

	CUILockOnController* pController = CUILockOnController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"LockOnController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_BossHUDController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();
	if (pGameInstance == nullptr)
		return E_FAIL;

	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIBossHUDController::BOSSHUD_CONTROLLER_DESC Desc = {};

	Desc.hUI = Handle;
	Desc.strBossLayerTag = TEXT("Layer_Monster");
	Desc.vFatalScreenOffset = _float2(0.f, 0.f);

	CUIBossHUDController* pController = CUIBossHUDController::Create();
	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"BossHUDController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_MonsterHUDController(const string& strAssetID, const _wstring& strInstancePrefix, const _float2& vScreenOffset)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	// 이미 공유 몬스터 HUD 컨트롤러가 등록됐다면 중복 생성하지 않는다.
	if (pGameInstance->Find_UIController(L"MonsterHPController") != nullptr)
		return S_OK;

	CUIMonsterHUDController::MONSTERHUD_DESC Desc = {};

	// 특정 인스턴스 핸들은 사용하지 않는다
	Desc.hUI.Invalidate();

	Desc.strAssetID = strAssetID;
	Desc.strInstancePrefix = strInstancePrefix;
	Desc.strMonsterLayerTag = TEXT("Layer_Monster");

	Desc.vScreenOffset = vScreenOffset;
	Desc.vFatalScreenOffset = _float2(0.f, 0.f);
	Desc.vGroggyScreenOffset = _float2(0.f, 0.f);
	
	CUIMonsterHUDController* pController = CUIMonsterHUDController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	// 여러 인스턴스를 관리하는 공유 컨트롤러이기 때문에 특정 런타임 슬롯에는 연결 X
	if (FAILED(pGameInstance->Add_UIController(L"MonsterHPController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_NPCDialogueController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc{};

	Desc.hUI = Handle;

	CUINPCDialogueController* pController = CUINPCDialogueController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"NPCDialogueController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_StargazerController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	// Builder가 별바라기 UIObject를 먼저 생성했는지 확인한다.
	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUIStargazerController* pController = CUIStargazerController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"StargazerController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_StargazerTravelController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUIStargazerTravelController* pController = CUIStargazerTravelController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"StargazerTravelController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_EquipmentController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUIEquipmentController* pController = CUIEquipmentController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"EquipmentController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_LegionArmChangeController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUILegionArmChangeController* pController = CUILegionArmChangeController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"LegionArmChangeController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	// UI Manager가 컨트롤러를 보관하므로 팩토리의 지역 참조만 해제한다.
	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_LegionArmUpgradeController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUILegionArmUpgradeController* pController = CUILegionArmUpgradeController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"LegionArmUpgradeController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	// UI Manager가 컨트롤러를 보관하므로 팩토리의 지역 참조만 해제한다.
	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_ShopController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUIShopController* pController = CUIShopController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"ShopController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_ConfirmationWindowController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUIConfirmationWindowController* pController = CUIConfirmationWindowController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"ConfirmationWindowController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);
	return S_OK;
}

HRESULT CUIFactory::Create_CrosshairHUDController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUICrosshairHUDController* pController = CUICrosshairHUDController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"CrosshairHUDController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_BossClearHUDController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc{};
	Desc.hUI = Handle;

	CUIBossClearHUDController* pController = CUIBossClearHUDController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"BossClearHUDController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_PlayerDeadController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc{};
	Desc.hUI = Handle;

	CUIPlayerDeadController* pController = CUIPlayerDeadController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"PlayerDeadController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_TitleController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};
	Desc.hUI = Handle;

	CUITitleController* pController = CUITitleController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"TitleController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_ItemPopupController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr || pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc{};
	Desc.hUI = Handle;

	CUIItemPopupController* pController = CUIItemPopupController::Create();
	if (nullptr == pController)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"ItemPopupController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_InteractionPromptController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	// Builder가 UIObject를 먼저 생성했는지 확인한다.
	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};

	Desc.hUI = Handle;

	CUIInteractionPromptController* pController = CUIInteractionPromptController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"InteractionPromptController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	Safe_Release(pController);

	return S_OK;
}

HRESULT CUIFactory::Create_ESCMenuController(const UI_HANDLE& Handle)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();

	if (pGameInstance == nullptr)
		return E_FAIL;

	/*
	 * Controller보다 UIObject가 먼저 생성됐는지 확인한다.
	 */
	if (pGameInstance->Find_UIObject(Handle) == nullptr)
		return E_FAIL;

	CUIControllerBase::UI_CONTROLLER_DESC Desc = {};

	Desc.hUI = Handle;

	CUIESCMenuController* pController = CUIESCMenuController::Create();

	if (pController == nullptr)
		return E_FAIL;

	if (FAILED(pController->Initialize(&Desc)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	if (FAILED(pGameInstance->Register_UIController(Handle, L"EscapeMenuController", pController)))
	{
		Safe_Release(pController);
		return E_FAIL;
	}

	/*
	 * Register_UIController에서 Controller를 보관하므로
	 * Factory의 지역 참조만 해제한다.
	 */
	Safe_Release(pController);

	return S_OK;
}
