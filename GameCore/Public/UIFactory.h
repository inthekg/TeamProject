#pragma once
#include "Core_Defines.h"
#include "UI_RuntimeData.h"

NS_BEGIN(GameCore)

class CUIFactory final
{
private:
	CUIFactory() = delete;
	~CUIFactory() = delete;

public:
	// 게임 플레이에서 사용하는 UI 에셋 등록과 런타임 생성 한 번에 처리
	static HRESULT Ready_GameplayHUDs();
	// 로딩 UI 에셋을 등록하고 싱글톤 인스턴스를 생성
	static HRESULT Ready_LoadingUI(UI_HANDLE* pOutHandle = nullptr);
	static HRESULT Ready_TitleUI(UI_HANDLE* pOutHandle = nullptr);

	// 싱글톤 UI 생성 + 컨트롤러 연결
	static HRESULT Create_SingletonUI(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle = nullptr);

private:
	static HRESULT Register_ControllerFactories();

	// UI Asset의 컨트롤러 타입에 맞는 컨트롤러를 생성하고 UI에 연결한다.
	static HRESULT Create_Controller(const string& strAssetID, const UI_HANDLE& Handle);

	static HRESULT Create_LoadingController(const UI_HANDLE& Handle);

	static HRESULT Create_PlayerHUDController(const UI_HANDLE& Handle);
	static HRESULT Create_LockOnController(const UI_HANDLE& Handle, const _float2& vScreenOffset);
	static HRESULT Create_BossHUDController(const UI_HANDLE& Handle);
	static HRESULT Create_InteractionPromptController(const UI_HANDLE& Handle);
	static HRESULT Create_ESCMenuController(const UI_HANDLE& Handle);
	static HRESULT Create_NPCDialogueController(const UI_HANDLE& Handle);
	static HRESULT Create_StargazerController(const UI_HANDLE& Handle);
	static HRESULT Create_StargazerTravelController(const UI_HANDLE& Handle);
	static HRESULT Create_EquipmentController(const UI_HANDLE& Handle);
	static HRESULT Create_LegionArmChangeController(const UI_HANDLE& Handle);
	static HRESULT Create_LegionArmUpgradeController(const UI_HANDLE& Handle);
	static HRESULT Create_ShopController(const UI_HANDLE& Handle);
	static HRESULT Create_ConfirmationWindowController(const UI_HANDLE& Handle);
	static HRESULT Create_CrosshairHUDController(const UI_HANDLE& Handle);
	static HRESULT Create_BossClearHUDController(const UI_HANDLE& Handle);
	static HRESULT Create_PlayerDeadController(const UI_HANDLE& Handle);
	static HRESULT Create_TitleController(const UI_HANDLE& Handle);
	static HRESULT Create_ItemPopupController(const UI_HANDLE& Handle);
	static HRESULT Create_MonsterHUDController(const string& strAssetID, const _wstring& strInstancePrefix, const _float2& vScreenOffset);
};

NS_END