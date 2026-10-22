#include "UI_Manager.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "UIControllerBase.h"
#include "UIImage.h"
#include "UIProgressBar.h"
#include "UIText.h"
#include "UIButton.h"
#include "UIContainer.h"
#include "Shader.h"
#include "VIBuffer.h"
#include "Material.h"
#include "Texture.h"
#include "Engine_Utility.h"
#include "UIAssetManager.h"
#include "UI_Function.h"

namespace
{
	constexpr const char* UI_BUTTON_HOVER_SOUND_KEY = "SE_UI_Btn_Hovered_Default_02";
	constexpr const char* UI_BUTTON_CLICK_SOUND_KEY = "SE_UI_Btn_Selected_Default_03";

	// Target이 실제로 해당 Button 아래에 있는지 런타임에서도 방어
	_bool IsButtonDescendant(const CUIButton* pButton, const CUIElement* pTarget)
	{
		if (pButton == nullptr || pTarget == nullptr)
			return false;

		for (const CUIElement* pParent = pTarget->Get_Parent(); pParent != nullptr; pParent = pParent->Get_Parent())
		{
			if (pParent == pButton)
				return true;
		}

		return false;
	}

	// Asset 상태 Override의 BindingKey를 런타임 Element로 해석
	HRESULT RegisterButtonVisualState(CUIObject* pUIObject, CUIButton* pButton, const UI_BUTTON_VISUAL_STATE_ASSET_DESC& StateDesc, UI_BUTTON_VISUAL_STATE eRuntimeState, const _wstring& strBindingPrefix)
	{
		if (pUIObject == nullptr || pButton == nullptr)
			return E_FAIL;

		for (const UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC& OverrideDesc : StateDesc.Overrides)
		{
			if (OverrideDesc.strTargetBindingKey.empty())
				return E_FAIL;

			_wstring strRuntimeTargetTag = StringToWString(OverrideDesc.strTargetBindingKey);

			// Component Instance 내부 Button이면 Namespace를 붙인다.
			if (!strBindingPrefix.empty())
				strRuntimeTargetTag = strBindingPrefix + L"." + strRuntimeTargetTag;

			CUIElement* pTarget = pUIObject->Find_Element(strRuntimeTargetTag);

			if (pTarget == nullptr)
				return E_FAIL;

			if (!IsButtonDescendant(pButton, pTarget))
				return E_FAIL;

			CUIButton::UI_BUTTON_VISUAL_OVERRIDE RuntimeOverride = {};

			// 같은 UIObject가 Target을 소유하므로 비소유 포인터
			RuntimeOverride.pTarget = pTarget;
			RuntimeOverride.bOverrideVisible = OverrideDesc.bOverrideVisible;
			RuntimeOverride.bVisible = OverrideDesc.bVisible;
			RuntimeOverride.bOverrideColor = OverrideDesc.bOverrideColor;
			RuntimeOverride.vColor = OverrideDesc.vColor;
			RuntimeOverride.bOverrideScale = OverrideDesc.bOverrideScale;
			RuntimeOverride.vScale = OverrideDesc.vScale;

			if (FAILED(pButton->Add_VisualOverride(eRuntimeState, RuntimeOverride)))
				return E_FAIL;
		}

		return S_OK;
	}
}

CUI_Manager::CUI_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
	, m_pGameInstance { CGameInstance::Get_Instance() }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}


HRESULT CUI_Manager::Initialize()
{
	if (FAILED(m_UIFontRegistry.Initialize(m_pDevice)))
		return E_FAIL;

	return S_OK;
}

void CUI_Manager::Priority_Update(_float fTimeDelta)
{
	for (auto& Pair : m_UIObjects)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Priority_Update(fTimeDelta);
	}
}

void CUI_Manager::Update(_float fTimeDelta)
{
	for (auto& Pair : m_UIControllers)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Update(fTimeDelta);
	}

	// 런타임 상태가 컨트롤러의 표시 요청보다 우선한다.
	for (UI_RUNTIME_SLOT& Slot : m_UIRuntimeSlots)
	{
		if (!Slot.bOccupied || Slot.Instance.pUIObject == nullptr)
			continue;

		if (Slot.Instance.eState != UI_RUNTIME_STATE::ACTIVE)
			Slot.Instance.pUIObject->Set_Visible(false);
	}

	// 레이어 억제와 Input Owner Stack 억제를 합쳐 최종 렌더 상태를 갱신
	Refresh_UIRenderSuppression();

	// 열린 UI의 Button Pointer 입력을 중앙에서 한 번 처리
	Update_ButtonPointerInput();

	for (auto& Pair : m_UIObjects)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Update(fTimeDelta);
	}
}

void CUI_Manager::Late_Update(_float fTimeDelta)
{
	for (auto& Pair : m_UIObjects)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Late_Update(fTimeDelta);
	}

	Flush_UIInputOwnerCommands();
	Flush_PendingCommands();
}

void CUI_Manager::Submit_RenderCommand()
{
	for (auto& Pair : m_UIObjects)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Submit_RenderCommand();
	}
}

// 기존 런타임 저장소 등록 API
HRESULT CUI_Manager::Add_UIObject(const _wstring& strUITag, CUIObject* pUIObject)
{
	if (strUITag.empty() || pUIObject == nullptr)
		return E_FAIL;

	if (Find_UIObject(strUITag) != nullptr)
		return E_FAIL;

	m_UIObjects.emplace(strUITag, pUIObject);
	Safe_AddRef(pUIObject);

	return S_OK;
}

HRESULT CUI_Manager::Add_UIController(const _wstring& strControllerTag, CUIControllerBase* pController)
{
	if (strControllerTag.empty() || pController == nullptr)
		return E_FAIL;

	if (Find_UIController(strControllerTag) != nullptr)
		return E_FAIL;

	m_UIControllers.emplace(strControllerTag, pController);
	Safe_AddRef(pController);

	return S_OK;
}

// 즉시 생성, 호출이 성공하면 UIObject 생성과 Binding이 바로 가능
HRESULT CUI_Manager::Spawn_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	if (pOutHandle == nullptr)
		return E_FAIL;

	pOutHandle->Invalidate();

	if (strAssetID.empty() || strInstanceTag.empty())
		return E_FAIL;

	if (m_PendingSpawnInstanceTags.find(strInstanceTag) != m_PendingSpawnInstanceTags.end())
		return E_FAIL;

	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);
	if (pAssetDesc == nullptr)
		return  E_FAIL;

	const _bool bSingleton = Is_SingletonPolicy(pAssetDesc->eInstancePolicy);

	if (bSingleton)
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end())
		{
			// PendingRelease 상태여도 아직 슬롯을 점유 중이므로 실제 Flush 전까지는 새로 만들지 않음
			if (Is_ValidUIHandle(iterSingleton->second))
				return E_FAIL;

			// 남아 있는 무효 핸들 정리
			m_SingletonUIHandles.erase(iterSingleton);
		}
	}

	// 현재 태그 맵과의 호환을 위해 기존 생성 함수를 이용
	if (FAILED(Create_UIObject(*pAssetDesc, strInstanceTag)))
		return E_FAIL;

	CUIObject* pUIObject = Find_UIObject(strInstanceTag);

	if (pUIObject == nullptr)
		return E_FAIL;
	
	UI_HANDLE Handle = Allocate_UIRuntimeSlot();

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	Slot.Instance.strAssetID = strAssetID;
	Slot.Instance.strInstanceTag = strInstanceTag;
	Slot.Instance.strLayer = pAssetDesc->strLayer;

	Slot.Instance.eInstancePolicy = pAssetDesc->eInstancePolicy;
	Slot.Instance.eLifetime = pAssetDesc->eLifetime;
	Slot.Instance.eState = UI_RUNTIME_STATE::ACTIVE;

	// 현재 실제 소유권은 m_UIObjects가 유지
	Slot.Instance.pUIObject = pUIObject;
	Slot.Instance.pController = nullptr;

	pUIObject->Set_RenderSuppressed(Is_UILayerSuppressed(Slot.Instance.strLayer) || Is_UIInputOwnerRenderSuppressed(Handle));

	if (bSingleton)
		m_SingletonUIHandles[strAssetID] = Handle;

	*pOutHandle = Handle;

	return S_OK;
}

// 핸들로 실제 UIObject 조회, PendingSpawn/PendingRelease 상태에서는 nullptr 반환
CUIObject* CUI_Manager::Find_UIObject(const UI_HANDLE& Handle)
{
	if (!Is_AliveUIObject(Handle))
		return nullptr;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	return Slot.Instance.pUIObject;
}

// 기존 컨트롤러/Binding 호환용 태그 조회
CUIObject* CUI_Manager::Find_UIObject(const _wstring& strUITag)
{
	auto iter = m_UIObjects.find(strUITag);
	
	if (iter == m_UIObjects.end())
		return nullptr;

	return iter->second;
}

CUIControllerBase* CUI_Manager::Find_UIController(const _wstring& strControllerTag)
{
	auto iter = m_UIControllers.find(strControllerTag);

	if (iter == m_UIControllers.end())
		return nullptr;

	return iter->second;
}

// 등록된 SpriteFont 조회
SpriteFont* CUI_Manager::Find_UIFont(const string& strFontAssetID)
{
	return m_UIFontRegistry.Find_Font(strFontAssetID);
}

HRESULT CUI_Manager::Set_UIImageTexture(const UI_HANDLE& Handle, const _wstring& strRuntimeElementTag, const string& strTexturePath)
{
	if (!Handle.Is_Valid() || strRuntimeElementTag.empty() || strTexturePath.empty())
		return E_FAIL;

	CUIObject* pUIObject = Find_UIObject(Handle);

	if (pUIObject == nullptr)
		return E_FAIL;

	CUIImage* pImage = dynamic_cast<CUIImage*>(pUIObject->Find_Element(strRuntimeElementTag));

	if (pImage == nullptr)
		return E_FAIL;

	CMaterial* pMaterial = nullptr;

	// 기존 UI 생성 과정과 동일한 방식으로 텍스처를 등록하고 Material을 생성한다.
	if (FAILED(Create_Material(strTexturePath, &pMaterial)) || pMaterial == nullptr)
	{
		Safe_Release(pMaterial);
		return E_FAIL;
	}

	pImage->Set_Material(pMaterial);
	Safe_Release(pMaterial);

	return S_OK;
}

// 컨트롤러 제거를 안전한 프레임 경계로 예약
void CUI_Manager::Request_RemoveUIController(const _wstring& strControllerTag)
{
	if (strControllerTag.empty())
		return;

	m_PendingRemoveUIControllers.emplace(strControllerTag);
}

// 해당 핸들의 인스턴스 제거를 예약
void CUI_Manager::Request_ReleaseUIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	// 이미 제거 예약된 핸들의 중복 요청 차단
	if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
		return;

	Slot.Instance.eState = UI_RUNTIME_STATE::PENDING_RELEASE;

	Request_RemoveUIInputOwner(Handle);

	// Flush 전까지 객체가 살아 있어도 화면 표시 X
	if (Slot.Instance.pUIObject != nullptr)
		Slot.Instance.pUIObject->Set_Visible(false);

	UI_RUNTIME_COMMAND Command = {};
	Command.eType = UI_RUNTIME_COMMAND_TYPE::RELEASE;
	Command.hUI = Handle;

	m_PendingUICommands.push_back(Command);
}

// 지정한 Lifetime의 모든 인스턴스 제거를 예약
void CUI_Manager::Request_ReleaseLifetime(UI_LIFETIME eLifetime)
{
	for (_uint iSlotIndex = 0; iSlotIndex < m_UIRuntimeSlots.size(); ++iSlotIndex)
	{
		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[iSlotIndex];
		if (!Slot.bOccupied)
			continue;

		if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
			continue;

		if (Slot.Instance.eLifetime != eLifetime)
			continue;

		UI_HANDLE Handle = {};
		Handle.iIndex = iSlotIndex;
		Handle.iGeneration = Slot.iGeneration;

		Request_ReleaseUIObject(Handle);
	}
}

// 지연 생성, 핸들과 슬롯만 즉시 예약하고 실제 UIObject 생성은 Late_Update 끝의 Pending Command 처리 구간에서 수행
HRESULT CUI_Manager::Request_SpawnUIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	if (pOutHandle == nullptr)
		return E_FAIL;

	pOutHandle->Invalidate();

	if (strAssetID.empty() || strInstanceTag.empty())
		return E_FAIL;

	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);

	if (pAssetDesc == nullptr)
		return E_FAIL;

	// 이미 생성되었거나 생성 예약된 동일 태그 차단
	if (Find_UIObject(strInstanceTag) != nullptr)
		return E_FAIL;

	if (m_PendingSpawnInstanceTags.find(strInstanceTag) != m_PendingSpawnInstanceTags.end())
		return E_FAIL;

	const _bool bSingleton = Is_SingletonPolicy(pAssetDesc->eInstancePolicy);

	if (bSingleton)
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end())
		{
			if (Is_ValidUIHandle(iterSingleton->second))
				return E_FAIL;

			m_SingletonUIHandles.erase(iterSingleton);
		}
	}

	UI_HANDLE Handle = Allocate_UIRuntimeSlot();

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	Slot.Instance.strAssetID = strAssetID;
	Slot.Instance.strInstanceTag = strInstanceTag;
	Slot.Instance.strLayer = pAssetDesc->strLayer;
	Slot.Instance.eInstancePolicy = pAssetDesc->eInstancePolicy;
	Slot.Instance.eLifetime = pAssetDesc->eLifetime;
	Slot.Instance.eState = UI_RUNTIME_STATE::PENDING_SPAWN;
	Slot.Instance.pUIObject = nullptr;
	Slot.Instance.pController = nullptr;

	m_PendingSpawnInstanceTags.emplace(strInstanceTag);

	if (bSingleton)
		m_SingletonUIHandles[strAssetID] = Handle;

	UI_RUNTIME_COMMAND Command = {};
	Command.eType = UI_RUNTIME_COMMAND_TYPE::SPAWN;
	Command.hUI = Handle;

	m_PendingUICommands.push_back(Command);

	*pOutHandle = Handle;

	return S_OK;
}

// 열린 UI를 Screen 또는 Modal 입력 소유자로 추가하도록 예약
HRESULT CUI_Manager::Request_PushUIInputOwner(const UI_HANDLE& Handle, UI_INPUT_OWNER_KIND eKind)
{
	if (UI_INPUT_OWNER_KIND::END == eKind)
		return E_FAIL;

	if (!Is_OpenUIObject(Handle))
		return E_FAIL;

	if (Is_UIInputOwner(Handle))
		return E_FAIL;

	if (Has_PendingUIInputOwnerCommand(Handle))
		return E_FAIL;

	if (UI_INPUT_OWNER_KIND::SCREEN == eKind)
	{
		if (Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND::SCREEN) || Has_PendingUIInputOwnerPushKind(UI_INPUT_OWNER_KIND::SCREEN))
			return E_FAIL;
	}

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::PUSH;
	Command.hUI = Handle;
	Command.eKind = eKind;

	m_PendingUIInputOwnerCommands.push_back(Command);

	return S_OK;
}

// 현재 최상단 UI의 입력 소유권 제거를 예약
HRESULT CUI_Manager::Request_PopUIInputOwner(const UI_HANDLE& Handle)
{
	if (m_UIInputOwnerStack.empty())
		return E_FAIL;

	if (!(m_UIInputOwnerStack.back().hUI == Handle))
		return E_FAIL;

	if (Has_PendingUIInputOwnerCommand(Handle))
		return E_FAIL;

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::POP;
	Command.hUI = Handle;

	m_PendingUIInputOwnerCommands.push_back(Command);

	return S_OK;
}

// 외부에서 인스턴스를 사용 할 수 있는지 검사(Close 상태여도 인스턴스가 살아있으면 참)
// 제거 예약 직후에서 인스턴스는 죽은 걸로 취급 따라서 false
_bool CUI_Manager::Is_AliveUIObject(const UI_HANDLE& Handle) const
{
	if (!Is_ValidUIHandle(Handle))
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
		return false;

	return Slot.Instance.pUIObject != nullptr;
}

// 현재 화면에서 열린 상태인지 검사
_bool CUI_Manager::Is_OpenUIObject(const UI_HANDLE& Handle) const
{
	if (!Is_ValidUIHandle(Handle))
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	return Slot.Instance.eState == UI_RUNTIME_STATE::ACTIVE && Slot.Instance.pUIObject != nullptr;
}

_bool CUI_Manager::Is_PendingSpawnUIObject(const UI_HANDLE& Handle) const
{
	if (!Is_ValidUIHandle(Handle))
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	return Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_SPAWN;
}

_bool CUI_Manager::Contains_UIFont(const string& strFontAssetID) const
{
	return m_UIFontRegistry.Contains_Font(strFontAssetID);
}

// 현재 확정된 UI 입력 소유자가 존재하는지 검사
_bool CUI_Manager::Has_UIInputOwner() const
{
	return !m_UIInputOwnerStack.empty();
}

// 전달된 UI가 현재 최상단 입력 소유자인지 검사
_bool CUI_Manager::Is_TopUIInputOwner(const UI_HANDLE& Handle) const
{
	if (m_UIInputOwnerStack.empty())
		return false;

	return m_UIInputOwnerStack.back().hUI == Handle;
}

_bool CUI_Manager::Can_PushUI(UI_INPUT_OWNER_KIND eKind) const
{
	if (UI_INPUT_OWNER_KIND::END == eKind)
		return false;

	// Screen은 확정 Stack과 Pending Push를 포함해 하나만 허용한다.
	if (UI_INPUT_OWNER_KIND::SCREEN == eKind)
		return !Has_UIInputOwnerKind(eKind) && !Has_PendingUIInputOwnerPushKind(eKind);

	// Fullscreen Modal과 Overlay Modal은 현재 최상단 Owner 위에 중첩할 수 있다.
	return true;
}

void CUI_Manager::Acquire_UILayerSuppression(const string& strLayer)
{
	if (strLayer.empty())
		return;

	_uint& iSuppressionCount = m_UILayerSuppressionCounts[strLayer];

	++iSuppressionCount;

	// 이미 억제된 레이어면 최종 표시 상태가 달라지지 않는다.
	if (iSuppressionCount > 1)
		return;

	// 레이어 억제와 Stack 억제를 함께 다시 계산한다.
	Refresh_UIRenderSuppression();
}

void CUI_Manager::Release_UILayerSuppression(const string& strLayer)
{
	const auto iter = m_UILayerSuppressionCounts.find(strLayer);

	if (iter == m_UILayerSuppressionCounts.end())
		return;

	if (iter->second > 1)
	{
		--iter->second;
		return;
	}

	m_UILayerSuppressionCounts.erase(iter);

	// 레이어 억제가 풀려도 Fullscreen Modal 아래의 UI는 계속 억제되어야 한다.
	Refresh_UIRenderSuppression();
}

_bool CUI_Manager::Is_UILayerSuppressed(const string& strLayer) const
{
	const auto iter = m_UILayerSuppressionCounts.find(strLayer);

	if (iter == m_UILayerSuppressionCounts.end())
		return false;

	return iter->second > 0;
}

void CUI_Manager::Clear()
{
	Clear_ButtonFocus();
	Reset_ButtonPointerInput();

	// 기존 컨트롤러 제거 예약은 더 이상 처리하지 않는다.
	m_PendingRemoveUIControllers.clear();

	// 컨트롤러 Free에서 UI Release Command가 추가 될 수 있음
	for (auto& Pair : m_UIControllers)
		Safe_Release(Pair.second);

	m_UIControllers.clear();

	// 컨트롤러  Free에서 새로 들어온 Command도 폐기
	m_PendingRemoveUIControllers.clear();
	m_PendingUICommands.clear();
	m_PendingSpawnInstanceTags.clear();

	// Controller 해제 과정에서 추가된 Input Owner 명령까지 모두 폐기한다.
	m_UIInputOwnerStack.clear();
	m_PendingUIInputOwnerCommands.clear();

	for (auto& Pair : m_UIObjects)
		Safe_Release(Pair.second);

	m_UIObjects.clear();
	
	// 런타임 저장소 비우기
	m_SingletonUIHandles.clear();
	m_UILayerSuppressionCounts.clear();

	// 슬롯의 포인터는 비소유 -> 따로 릴리즈 X
	m_UIRuntimeSlots.clear();
	m_FreeUIRuntimeSlotIndices.clear();
}

HRESULT CUI_Manager::Create_UIObjectFromAsset(const string& strAssetID, const _wstring& strInstanceTag)
{
	UI_HANDLE Handle = {};

	return Spawn_UIObject(strAssetID, strInstanceTag, &Handle);
}

HRESULT CUI_Manager::Register_UIAsset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation, _wstring* pError)
{
	return m_UIAssetManager.Load_Asset(strFilePath, pOutValidation, pError);
}

HRESULT CUI_Manager::Register_UIController(const UI_HANDLE& Handle, const _wstring& strControllerTag, CUIControllerBase* pController)
{
	if (!Is_AliveUIObject(Handle))
		return E_FAIL;

	if (strControllerTag.empty() || pController == nullptr)
		return E_FAIL;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	// 하나의 런타임 인스턴스에 컨트롤러 하나만 연결
	if (Slot.Instance.pController != nullptr)
		return E_FAIL;

	if (Find_UIController(strControllerTag) != nullptr)
		return E_FAIL;

	m_UIControllers.emplace(strControllerTag, pController);

	Safe_AddRef(pController);

	Slot.Instance.strControllerTag = strControllerTag;
	Slot.Instance.pController = pController;

	return S_OK;
}

HRESULT CUI_Manager::Register_UIControllerFactory(const string& strControllerType, const UI_CONTROLLER_FACTORY& Factory)
{
	if (strControllerType.empty() || !Factory)
		return E_FAIL;

	if (m_UIControllerFactories.find(strControllerType) != m_UIControllerFactories.end())
		return E_FAIL;

	m_UIControllerFactories.emplace(strControllerType, Factory);

	return S_OK;
}

HRESULT CUI_Manager::Register_UIAssetDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount, _wstring* pError)
{
	return m_UIAssetManager.Load_AssetsFromDirectory(strDirectoryPath, pOutLoadedCount, pError);
}

// 런타임에서 사용할 SpriteFont 등록
HRESULT CUI_Manager::Register_UIFont(const string& strFontAssetID, const _wstring& strFontPath)
{
	return m_UIFontRegistry.Register_Font(strFontAssetID, strFontPath);
}

HRESULT CUI_Manager::Create_UIController(const UI_CONTROLLER_CREATE_CONTEXT& Context)
{
	if (Context.strControllerType.empty())
		return S_OK;

	auto iter = m_UIControllerFactories.find(Context.strControllerType);
	if (iter == m_UIControllerFactories.end())
	{
		return E_FAIL;
	}

	if (!iter->second)
		return E_FAIL;

	return iter->second(Context);
}

const UI_ASSET_DESC* CUI_Manager::Find_UIAsset(const string& strAssetID) const
{
	return m_UIAssetManager.Find_Asset(strAssetID);
}

_bool CUI_Manager::Contains_UIAsset(const string& strAssetID) const
{
	return m_UIAssetManager.Contains_Asset(strAssetID);
}

size_t CUI_Manager::Get_UIAssetCount() const
{
	return m_UIAssetManager.Get_AssetCount();
}

// 기존 싱글톤을 열거나, 없으면 생성
HRESULT CUI_Manager::Open_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	if (pOutHandle == nullptr)
		return E_FAIL;

	pOutHandle->Invalidate();

	if (strAssetID.empty() || strInstanceTag.empty())
		return E_FAIL;

	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);

	if (pAssetDesc == nullptr)
		return E_FAIL;

	// Open / Close 싱글톤 계열에서만 사용
	if (!Is_SingletonPolicy(pAssetDesc->eInstancePolicy))
		return E_FAIL;

	auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

	// 아직 생성되지 않았다면 처음 생성
	if (iterSingleton == m_SingletonUIHandles.end())
		return Spawn_UIObject(strAssetID, strInstanceTag, pOutHandle);

	const UI_HANDLE Handle = iterSingleton->second;

	if (!Is_ValidUIHandle(Handle))
	{
		m_SingletonUIHandles.erase(iterSingleton);

		return Spawn_UIObject(strAssetID, strInstanceTag, pOutHandle);
	}

	if (FAILED(Open_UIObject(Handle)))
		return E_FAIL;

	*pOutHandle = Handle;

	return S_OK;
}

// 생성되어 있는 싱글톤을 다시 연다
HRESULT CUI_Manager::Open_UIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return E_FAIL;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (!Is_SingletonPolicy(Slot.Instance.eInstancePolicy))
		return E_FAIL;

	if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
		return E_FAIL;

	if (Slot.Instance.pUIObject == nullptr)
		return E_FAIL;

	Slot.Instance.eState = UI_RUNTIME_STATE::ACTIVE;

	Slot.Instance.pUIObject->Set_Visible(true);
	Slot.Instance.pUIObject->Set_RenderSuppressed(Is_UILayerSuppressed(Slot.Instance.strLayer) || Is_UIInputOwnerRenderSuppressed(Handle));

	return S_OK;
}

// 인스턴스와 핸들은 유지하고 화면에서만 숨긴다
HRESULT CUI_Manager::Close_UIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return E_FAIL;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (!Is_SingletonPolicy(Slot.Instance.eInstancePolicy))
		return E_FAIL;

	if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
		return E_FAIL;

	if (Slot.Instance.pUIObject == nullptr)
		return E_FAIL;

	Slot.Instance.eState = UI_RUNTIME_STATE::CLOSED;

	Slot.Instance.pUIObject->Set_Visible(false);

	return S_OK;
}

HRESULT CUI_Manager::Create_Material(const string& strTexturePath, CMaterial** ppOutMaterial)
{
	if (ppOutMaterial == nullptr)
		return E_FAIL;

	*ppOutMaterial = nullptr;

	if (strTexturePath.empty())
		return S_OK;

	_wstring strTexturePathW = Resolve_TexturePath(strTexturePath);
	_wstring strTextureKey = Get_FileNameFromPath(strTexturePathW);

	CTexture* pTexture = nullptr;

	if (FAILED(m_pGameInstance->Get_Texture(strTextureKey, &pTexture)))
	{
		if (FAILED(m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strTexturePathW)))
			return E_FAIL;

		if (FAILED(m_pGameInstance->Get_Texture(strTextureKey, &pTexture)))
			return E_FAIL;
	}

	CMaterial* pMaterial = CMaterial::Create(m_pDevice, m_pContext);
	if (pMaterial == nullptr)
	{
		Safe_Release(pTexture);
		return E_FAIL;
	}

	pMaterial->Input_Texture(TEXTURE_TYPE::BASE_COLOR, pTexture);

	Safe_Release(pTexture);

	*ppOutMaterial = pMaterial;

	return S_OK;
}

HRESULT CUI_Manager::Create_Material(const string& strTexturePath, const string& strDissolveTexturePath, CMaterial** ppOutMaterial)
{
	if (nullptr == ppOutMaterial)
		return E_FAIL;

	*ppOutMaterial = nullptr;

	// 기존 함수를 이용하여 실제 UI 이미지를 0번 BASE_COLOR 텍스처로 등록한다.
	CMaterial* pMaterial = nullptr;

	if (FAILED(Create_Material(strTexturePath, &pMaterial)) || nullptr == pMaterial)
	{
		Safe_Release(pMaterial);
		return E_FAIL;
	}

	// 디졸브 텍스처를 사용하지 않는 경우 기존 Material을 그대로 반환한다.
	if (strDissolveTexturePath.empty())
	{
		*ppOutMaterial = pMaterial;
		return S_OK;
	}

	// 디졸브 노이즈 텍스처의 실제 경로와 리소스 키를 만든다.
	_wstring strDissolveTexturePathW = Resolve_TexturePath(strDissolveTexturePath);
	_wstring strDissolveTextureKey = Get_FileNameFromPath(strDissolveTexturePathW);

	CTexture* pDissolveTexture = nullptr;

	// 이미 등록된 텍스처가 있는지 먼저 확인한다.
	if (FAILED(m_pGameInstance->Get_Texture(strDissolveTextureKey, &pDissolveTexture)))
	{
		// 등록되지 않았다면 파일에서 새로 등록한다.
		if (FAILED(m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strDissolveTexturePathW)))
		{
			Safe_Release(pMaterial);
			return E_FAIL;
		}

		// 등록한 디졸브 텍스처를 다시 가져온다.
		if (FAILED(m_pGameInstance->Get_Texture(strDissolveTextureKey, &pDissolveTexture)))
		{
			Safe_Release(pMaterial);
			return E_FAIL;
		}
	}

	// 두 번째 BASE_COLOR 텍스처로 등록한다.
	// 셰이더에서는 g_AlbedoTexture[1]로 접근한다.
	if (FAILED(pMaterial->Input_Texture(TEXTURE_TYPE::BASE_COLOR, pDissolveTexture)))
	{
		Safe_Release(pDissolveTexture);
		Safe_Release(pMaterial);
		return E_FAIL;
	}

	Safe_Release(pDissolveTexture);

	*ppOutMaterial = pMaterial;

	return S_OK;
}

HRESULT CUI_Manager::Create_UIObject(const UI_ASSET_DESC& AssetDesc, const _wstring& strInstanceTag)
{
	if (strInstanceTag.empty())
		return E_FAIL;

	// 동일한 런타임 인스턴스 태그의 중복 생성 막기
	if (Find_UIObject(strInstanceTag) != nullptr)
		return E_FAIL;

	if (AssetDesc.Root.eType != UI_ELEMENT_TYPE::CONTAINER)
		return E_FAIL;

	CUIObject* pUIObject = CUIObject::Create(m_pDevice, m_pContext);

	if (pUIObject == nullptr)
		return E_FAIL;

	pUIObject->Set_SortOrder(AssetDesc.iSortOrder);

	const _float2 vRuntimeScale = Get_RuntimeUIScale(AssetDesc);

	// 현재 에셋부터 확장 경로에 넣어 자신으로 돌아오는 순환 참조도 차단한다
	unordered_set<string> ExpansionStack;
	ExpansionStack.insert(AssetDesc.strAssetID);

	if (FAILED(Create_UIRoot(pUIObject, AssetDesc.Root, vRuntimeScale, ExpansionStack)))
	{
		Safe_Release(pUIObject);
		return E_FAIL;
	}

	if (FAILED(pUIObject->Set_Animations(AssetDesc.Animations, vRuntimeScale)))
	{
		Safe_Release(pUIObject);
		return E_FAIL;
	}

	// 모든 Element 생성이 성공한 뒤에만 매니저에 등록
	if (FAILED(Add_UIObject(strInstanceTag, pUIObject)))
	{
		Safe_Release(pUIObject);
		return E_FAIL;
	}

	// Add_UIObject에서 참조 카운트를 올렸으므로 지역 참조 반환
	Safe_Release(pUIObject);

	return S_OK;
}

// 최상위 Asset Root와 자식 트리를 생성한다.
HRESULT CUI_Manager::Create_UIRoot(CUIObject* pUIObject, const UI_ELEMENT_ASSET_DESC& RootDesc, const _float2& vRuntimeScale, unordered_set<string>& ExpansionStack)
{
	if (pUIObject == nullptr)
		return E_FAIL;

	if (RootDesc.eType != UI_ELEMENT_TYPE::CONTAINER)
		return E_FAIL;

	CUIContainer* pRoot = CUIContainer::Create(m_pDevice, m_pContext);
	if (pRoot == nullptr)
		return E_FAIL;

	Apply_UITransform(pRoot, RootDesc.Transform, vRuntimeScale);

	// Root도 Animation Track의 대상이 될 수 있도록 원본 ElementID를 보관한다.
	pRoot->Set_ElementID(RootDesc.iElementID);
	pRoot->Set_RuntimeElementTag(Make_UIRuntimeElementTag(RootDesc, L""));
	// 루트의 상태도 이후 자식 Visibility 계산에 사용
	pRoot->Set_Color(RootDesc.Render.vColor);
	pRoot->Set_SortOrder(RootDesc.Render.iSortOrder);
	pRoot->Set_Visible(RootDesc.Render.bVisible);

	if (FAILED(pUIObject->Set_RootElement(pRoot)))
	{
		Safe_Release(pRoot);
		return E_FAIL;
	}

	if (FAILED(pUIObject->Register_AnimationTarget(pRoot)))
	{
		Safe_Release(pRoot);
		return E_FAIL;
	}

	for (const auto& ChildDesc : RootDesc.Children)
	{
		if (FAILED(Create_UIElement(pUIObject, pRoot, ChildDesc, vRuntimeScale, L"", UINT_MAX, ExpansionStack)))
		{
			Safe_Release(pRoot);
			return E_FAIL;
		}
	}

	Safe_Release(pRoot);

	return S_OK;
}

// 일반 Element 또는 Component Instance를 생성한다
HRESULT CUI_Manager::Create_UIElement(CUIObject* pUIObject, CUIElement* pParent, const UI_ELEMENT_ASSET_DESC& ElementDesc, const _float2& vRuntimeScale, const _wstring& strBindingPrefix, _uint iSortGroup, unordered_set<string>& ExpansionStack)
{
	if (pUIObject == nullptr || pParent == nullptr)
		return E_FAIL;

	// 컴포넌트 인스턴스는 일반 렌더 엘리먼트가 아닌 참조 컴포넌트를 실제 트리로 확장하는 가상 노드
	if (ElementDesc.eType == UI_ELEMENT_TYPE::COMPONENT_INSTANCE)
	{
		return Create_UIComponentInstance(pUIObject, pParent, ElementDesc, vRuntimeScale, strBindingPrefix, iSortGroup, ExpansionStack);
	}

	CUIElement* pElement = nullptr;
	CUIContainer* pContainer = nullptr;
	CUIImage* pImage = nullptr;
	CUIProgressBar* pProgressBar = nullptr;
	CUIText* pText = nullptr;
	CUIButton* pButton = nullptr;

	switch (ElementDesc.eType)
	{
	case UI_ELEMENT_TYPE::CONTAINER:
		pContainer = CUIContainer::Create(m_pDevice, m_pContext);
		pElement = pContainer;
		break;

	case UI_ELEMENT_TYPE::IMAGE:
		pImage = CUIImage::Create(m_pDevice, m_pContext);
		pElement = pImage;
		break;
	
	case UI_ELEMENT_TYPE::PROGRESS_BAR:
		pProgressBar = CUIProgressBar::Create(m_pDevice, m_pContext);
		pElement = pProgressBar;
		break;

	case UI_ELEMENT_TYPE::TEXT:
		pText = CUIText::Create(m_pDevice, m_pContext);
		pElement = pText;
		break;

	case UI_ELEMENT_TYPE::BUTTON:
		pButton = CUIButton::Create(m_pDevice, m_pContext);
		pElement = pButton;
		break;

	default:
		return E_FAIL;
	}

	if (pElement == nullptr)
		return E_FAIL;

	// 트랜스폼 적용
	Apply_UITransform(pElement, ElementDesc.Transform, vRuntimeScale);

	// 렌더 적용
	pElement->Set_Color(ElementDesc.Render.vColor);
	pElement->Set_UVRect(ElementDesc.Render.vUVRect);
	pElement->Set_RevealType(ElementDesc.Render.eRevealType);
	pElement->Set_RevealProgress(ElementDesc.Render.fRevealProgress);
	pElement->Set_RevealSoftness(ElementDesc.Render.fRevealSoftness);
	pElement->Set_RevealCenter(ElementDesc.Render.vRevealCenter);

	_uint iRuntimeSortOrder = 0;

	if (iSortGroup == UINT_MAX)
	{
		/*
		 * 일반 Element:
		 * 자기 SortOrder가 하나의 독립 그룹이 된다.
		 *
		 * SortOrder 10 → 10,000
		 * SortOrder 11 → 11,000
		 */
		iRuntimeSortOrder = ElementDesc.Render.iSortOrder * UI_ELEMENT_LOCAL_SORT_RANGE;
	}
	else
	{
		/*
		 * Component 내부 Element:
		 * 바깥 Component의 구릅 안에서 자신의 SortOrder를 사용한다.
		 *
		 * Component SortOrder 10, 내부 SortOrder 50
		 * → 10 × 1,000 + 50
		 * → 10,050
		 */
		iRuntimeSortOrder = iSortGroup * UI_ELEMENT_LOCAL_SORT_RANGE + ElementDesc.Render.iSortOrder;
	}

	pElement->Set_SortOrder(iRuntimeSortOrder);	
	pElement->Set_Visible(ElementDesc.Render.bVisible);
	pElement->Set_UseTexture(ElementDesc.Render.bUseTexture);

	if (pContainer == nullptr && pText == nullptr && pButton == nullptr)
	{
		CShader* pShader = nullptr;
		CVIBuffer* pVIBuffer = nullptr;

		if (FAILED(m_pGameInstance->Get_Shader(L"Shader_UI", &pShader)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		if (FAILED(m_pGameInstance->Get_VIBuffer(L"VIBUFFER_RECT", &pVIBuffer)))
		{
			Safe_Release(pShader);
			Safe_Release(pElement);
			return E_FAIL;
		}

		pElement->Set_Shader(pShader);
		pElement->Set_VIBuffer(pVIBuffer);

		Safe_Release(pVIBuffer);
		Safe_Release(pShader);

	}
	
	if (pImage != nullptr)
	{
		pImage->Set_DrawType(static_cast<CUIImage::UI_IMAGE_DRAW_TYPE>(ElementDesc.Image.eDrawType));

		_float4 vSliceMargin = ElementDesc.Image.vSliceMargin;
		_float2 vSliceBaseSize = ElementDesc.Image.vSliceBaseSize;

		// 나인 슬라이스의 픽셀 단위 크기도 UI 배율을 적용
		vSliceMargin.x *= vRuntimeScale.x;
		vSliceMargin.z *= vRuntimeScale.x;
		vSliceMargin.y *= vRuntimeScale.y;
		vSliceMargin.w *= vRuntimeScale.y;

		vSliceBaseSize.x *= vRuntimeScale.x;
		vSliceBaseSize.y *= vRuntimeScale.y;

		pImage->Set_SliceMargin(vSliceMargin);
		// 원본 텍스쳐 크기는 실제 이미지 크기 배율 적용 X
		pImage->Set_TextureSize(ElementDesc.Image.vTextureSize);
		pImage->Set_SliceBaseSize(vSliceBaseSize);

		if (ElementDesc.Render.bUseTexture)
		{
			CMaterial* pMaterial = nullptr;

			if (FAILED(Create_Material(ElementDesc.Image.strTexturePath, ElementDesc.Image.strDissolveTexturePath, &pMaterial)) || nullptr == pMaterial)
			{
				Safe_Release(pElement);
				return E_FAIL;
			}

			pElement->Set_Material(pMaterial);
			Safe_Release(pMaterial);
		}
	}
	else if (pProgressBar != nullptr)
	{
		pProgressBar->Set_LengthScale(ElementDesc.Progress.fLengthScale);
		pProgressBar->Set_FillType(ElementDesc.Progress.eFillType);
		pProgressBar->Set_FillDirection(ElementDesc.Progress.eFillDirection);
		pProgressBar->Set_ResizeDirection(ElementDesc.Progress.eResizeDirection);

		for (const auto& SlotDesc : ElementDesc.Progress.Slots)
		{
			CUIProgressBar::UI_PROGRESS_SLOT Slot = {};

			Slot.iSlotID = SlotDesc.iSlotID;
			Slot.iTargetSlotID = SlotDesc.iTargetSlotID;

			// 컨트롤러 조회는 바인딩 키 우선 사용
			const string& strRuntimeName = SlotDesc.strBindingKey.empty() ? SlotDesc.strName : SlotDesc.strBindingKey;

			Slot.strName = StringToWString(strRuntimeName);
			Slot.eType = static_cast<CUIProgressBar::UI_PROGRESS_SLOT_TYPE>(SlotDesc.eType);
			Slot.eBlendMode = static_cast<CUIProgressBar::UI_PROGRESS_BLEND_MODE>(SlotDesc.eBlendMode);
			
			Slot.fAmount = SlotDesc.fAmount;
			Slot.fStartAmount = SlotDesc.fStartAmount;
			Slot.fEndAmount = SlotDesc.fEndAmount;
			
			Slot.vUVRect = SlotDesc.vUVRect;
			Slot.bStretchUVToRange = SlotDesc.bStretchUVToRange;

			Slot.vColor = SlotDesc.vColor;
			Slot.vColorLT = SlotDesc.vColorLT;
			Slot.vColorRT = SlotDesc.vColorRT;
			Slot.vColorRB = SlotDesc.vColorRB;
			Slot.vColorLB = SlotDesc.vColorLB;

			Slot.iSortOffset = SlotDesc.iSortOffset;
			Slot.bVisible = SlotDesc.bVisible;

			CMaterial* pMaterial = nullptr;

			if (!SlotDesc.strTexturePath.empty())
			{
				if (FAILED(Create_Material(SlotDesc.strTexturePath, &pMaterial)))
				{
					Safe_Release(pMaterial);
					Safe_Release(pElement);
					return E_FAIL;
				}

				Slot.pMaterial = pMaterial;
			}

			if (FAILED(pProgressBar->Add_Slot(Slot)))
			{
				Safe_Release(pMaterial);
				Safe_Release(pElement);
				return E_FAIL;
			}

			Safe_Release(pMaterial);
		}
	}
	else if (pText != nullptr)
	{
		const SpriteFont* pFont = Find_UIFont(ElementDesc.Text.strFontAssetID);

		// JSON이 유효해도 런타임 Registry에 해당 ID가 등록되지 않았다면 Text를 만들 수 없음.
		if (pFont == nullptr)
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		pText->Set_Font(pFont);

		// JSON의 PreviewText는 UTF-8이므로 UTF-16으로 변환
		pText->Set_Text(StringToWString(ElementDesc.Text.strPreviewText));

		/*
		 * Position과 Size처럼 FontSize도 기준 해상도에서
		 * 실제 런타임 해상도로 배율을 적용한다.
		 * 현재 Runtime Scale은 균일 배율이므로 Y 값을 사용한다.
		 */
		pText->Set_FontSize(ElementDesc.Text.fFontSize* vRuntimeScale.y);
		pText->Set_LineSpacing(ElementDesc.Text.fLineSpacing);
		pText->Set_Wrap(ElementDesc.Text.bWrap);
		pText->Set_HorizontalAlign(static_cast<UI_TEXT_HORIZONTAL_ALIGN>(ElementDesc.Text.eHorizontalAlign));
		pText->Set_VerticalAlign(static_cast<UI_TEXT_VERTICAL_ALIGN>(ElementDesc.Text.eVerticalAlign));
	}
	else if (pButton != nullptr)
	{
		pButton->Set_CommandKey(ElementDesc.Button.strCommandKey);
		pButton->Set_FocusOnHover(ElementDesc.Button.bFocusOnHover);
		pButton->Set_SelectOnFocus(ElementDesc.Button.bSelectOnFocus);
		pButton->Set_CommandOnPress(ElementDesc.Button.bCommandOnPress);
		pButton->Set_FocusGroup(ElementDesc.Button.strFocusGroup);
		pButton->Set_FocusOrder(ElementDesc.Button.iFocusOrder);
		pButton->Set_Interactable(ElementDesc.Button.bInteractable);
	}

	const _wstring strRuntimeTag = Make_UIRuntimeElementTag(ElementDesc, strBindingPrefix);

	// Asset 원본 ID와 Namespace가 적용된 런타임 태그를 보관
	pElement->Set_ElementID(ElementDesc.iElementID);
	pElement->Set_RuntimeElementTag(strRuntimeTag);

	if (FAILED(pUIObject->Add_Element(strRuntimeTag, pElement)))
	{
		Safe_Release(pElement);
		return E_FAIL;
	}

	/*
	 * BindingPrefix가 비어 있다면 현재 Screen Asset에 직접 정의된 Element다.
	 * Component 내부 Element ID는 Component마다 중복될 수 있으므로 여기에는 등록하지 않는다.
	 */
	if (strBindingPrefix.empty())
	{
		if (FAILED(pUIObject->Register_AnimationTarget(pElement)))
		{
			pUIObject->Remove_Element(strRuntimeTag);
			Safe_Release(pElement);
			return E_FAIL;
		}
	}

	if (FAILED(pParent->Add_Child(pElement)))
	{
		pUIObject->Remove_Element(strRuntimeTag);
		Safe_Release(pElement);
		return E_FAIL;
	}

   /*
	* 일반 Button은 자신의 SortOrder를 하나의 렌더 그룹으로 사용한다.
	*
	* Button SortOrder 5
	* ├─ Background SortOrder 0 → 5,000
	* ├─ StateLayer SortOrder 1 → 5,001
	* └─ Label SortOrder 2      → 5,002
	*/
	_uint iChildSortGroup = iSortGroup;

	if (pButton != nullptr && iSortGroup == UINT_MAX)
	{
		iChildSortGroup = ElementDesc.Render.iSortOrder;
	}

	for (const auto& ChildDesc : ElementDesc.Children)
	{
		if (FAILED(Create_UIElement(pUIObject, pElement, ChildDesc, vRuntimeScale, strBindingPrefix, iChildSortGroup, ExpansionStack)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}
	}

	if (pButton != nullptr)
	{
		/*
		 * 모든 자식이 UIObject에 등록된 다음 BindingKey를 해석한다.
		 * 이보다 앞에서 실행하면 Find_Element가 자식을 찾지 못한다.
		 */
		if (FAILED(RegisterButtonVisualState(pUIObject, pButton, ElementDesc.Button.Hovered, UI_BUTTON_VISUAL_STATE::HOVERED, strBindingPrefix)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		if (FAILED(RegisterButtonVisualState(pUIObject, pButton, ElementDesc.Button.Pressed, UI_BUTTON_VISUAL_STATE::PRESSED, strBindingPrefix)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		if (FAILED(RegisterButtonVisualState(pUIObject, pButton, ElementDesc.Button.Selected, UI_BUTTON_VISUAL_STATE::SELECTED, strBindingPrefix)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		if (FAILED(RegisterButtonVisualState(pUIObject, pButton, ElementDesc.Button.Focused, UI_BUTTON_VISUAL_STATE::FOCUSED, strBindingPrefix)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		if (FAILED(RegisterButtonVisualState(pUIObject, pButton, ElementDesc.Button.Disabled, UI_BUTTON_VISUAL_STATE::DISABLED, strBindingPrefix)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}

		// 모든 Base 값과 Override가 준비된 뒤 최초 외형 적용
		pButton->Refresh_VisualState();

		// 런타임 입력 Router가 전체 Element를 다시 검색하지 않도록 등록
		if (FAILED(pUIObject->Register_Button(pButton)))
		{
			Safe_Release(pElement);
			return E_FAIL;
		}
	}

	Safe_Release(pElement);

	return S_OK;
}

// Component Instance를 Container 트리로 확장한다
HRESULT CUI_Manager::Create_UIComponentInstance(CUIObject* pUIObject, CUIElement* pParent, const UI_ELEMENT_ASSET_DESC& ElementDesc, const _float2& vRuntimeScale, const _wstring& strBindingPrefix, _uint iSortGroup, unordered_set<string>& ExpansionStack)
{
	if (pUIObject == nullptr || pParent == nullptr)
		return E_FAIL;

	const string& strComponentAssetID = ElementDesc.ComponentInstance.strComponentAssetID;

	if (strComponentAssetID.empty())
		return E_FAIL;

	const UI_ASSET_DESC* pComponentAsset = m_UIAssetManager.Find_Asset(strComponentAssetID);

	if (pComponentAsset == nullptr)
		return E_FAIL;

	if (pComponentAsset->eKind != UI_ASSET_KIND::COMPONENT)
		return E_FAIL;

	if (pComponentAsset->Root.eType != UI_ELEMENT_TYPE::CONTAINER)
		return E_FAIL;

	if (ExpansionStack.find(strComponentAssetID) != ExpansionStack.end())
		return E_FAIL;

	const UI_ELEMENT_ASSET_DESC& ReferenceRoot = pComponentAsset->Root;

	const _float fReferenceWidth = ReferenceRoot.Transform.vSize.x;
	const _float fReferenceHeight = ReferenceRoot.Transform.vSize.y;
		
	if (fReferenceWidth <= FLT_EPSILON || fReferenceHeight <= FLT_EPSILON)
		return E_FAIL;

	// 이 Tag가 해당 컴포넌트의 인스턴스의 Binding Namespace가 된다
	const _wstring strInstanceNamespace = Make_UIRuntimeElementTag(ElementDesc, strBindingPrefix);

	// 최상위 컴포넌트 인스턴스라면 자신의 SortOrder가 그룹이 된다.
	// 중첩 컴포넌트라면 가장 바깥의 컴포넌트 그룹을 그대로 유지
	const _uint iComponentSortGroup = iSortGroup == UINT_MAX ? ElementDesc.Render.iSortOrder : iSortGroup;

	/*
	*	1. Component Instance 자체를 Container로 만든다.
	*
	*	Screen
	*	  └─ Slot0                 ← Instance Container
	*	      └─ RuntimeScale
	*	          └─ ComponentRoot
	*/

	UI_ELEMENT_ASSET_DESC InstanceContainerDesc = ElementDesc;

	InstanceContainerDesc.eType = UI_ELEMENT_TYPE::CONTAINER;

	InstanceContainerDesc.Children.clear();
	InstanceContainerDesc.Render.bUseTexture = false;

	if (FAILED(Create_UIElement(pUIObject, pParent, InstanceContainerDesc, vRuntimeScale, strBindingPrefix, iComponentSortGroup, ExpansionStack)))
		return E_FAIL;

	CUIElement* pInstanceContainer = pUIObject->Find_Element(strInstanceNamespace);

	if (pInstanceContainer == nullptr)
		return E_FAIL;

	/*
	*	2. 참조 컴포넌트 원본 크기를 인스턴스 크기에 맞추는 내부 Scale 컨테이너를 만든다.
	*/
	UI_ELEMENT_ASSET_DESC ScaleContainerDesc = {};

	ScaleContainerDesc.strName = "__Runtime_ComponentScale";
	ScaleContainerDesc.strBindingKey = "__Runtime_ComponentScale";
	ScaleContainerDesc.eType = UI_ELEMENT_TYPE::CONTAINER;
	ScaleContainerDesc.Transform.vSize = ReferenceRoot.Transform.vSize;
	ScaleContainerDesc.Transform.vPivot = ElementDesc.Transform.vPivot;
	ScaleContainerDesc.Transform.eAnchor = UI_ANCHOR::CENTER;
	ScaleContainerDesc.Transform.fRotation = 0.f;
	ScaleContainerDesc.Transform.vScale = _float2(ElementDesc.Transform.vSize.x / fReferenceWidth, ElementDesc.Transform.vSize.y / fReferenceHeight);


	/*
	 *	Center Anchor가 Instance Pivot 때문에 만드는 오프셋을 상쇄한다.
	 *	이렇게 해야 Scale 컨테이너의 원점이 인스턴스 원점과 일치한다.
	 */

	const _float2 vAnchorOffset = Calc_UIAnchorOffset(UI_ANCHOR::CENTER, ElementDesc.Transform.vSize, ElementDesc.Transform.vPivot);

	ScaleContainerDesc.Transform.vPosition = _float2(-vAnchorOffset.x, -vAnchorOffset.y);

	ScaleContainerDesc.Render.bVisible = true;
	ScaleContainerDesc.Render.bUseTexture = false;

	if (FAILED(Create_UIElement(pUIObject, pInstanceContainer, ScaleContainerDesc, vRuntimeScale, strInstanceNamespace, iComponentSortGroup, ExpansionStack)))
	{
		return E_FAIL;
	}

	const _wstring strScaleContainerTag = strInstanceNamespace + L".__Runtime_ComponentScale";

	CUIElement* pScaleContainer = pUIObject->Find_Element(strScaleContainerTag);

	if (pScaleContainer == nullptr)
		return E_FAIL;

	/*
	 *	3. 컴포넌트 루트와 모든 자식을 실제 런타임 엘리먼트로 생성한다.
	 */
	UI_ELEMENT_ASSET_DESC RuntimeRootDesc = ReferenceRoot;

	// 컴포넌트 내부 자식의 Prefix와 충돌하지 않는 런타임 전용 루트 Tag
	RuntimeRootDesc.strBindingKey = "__Runtime_ComponentRoot";

	ExpansionStack.insert(strComponentAssetID);

	const HRESULT hr = Create_UIElement(pUIObject, pScaleContainer, RuntimeRootDesc, vRuntimeScale, strInstanceNamespace, iComponentSortGroup, ExpansionStack);

	// 성공, 실패와 관계 없이 현재 확장 경로에서 제거
	ExpansionStack.erase(strComponentAssetID);

	return hr;
}

// Prefix와 BindingKey를 결합해 실제 런타임 검색 Tag를 만든다.
_wstring CUI_Manager::Make_UIRuntimeElementTag(const UI_ELEMENT_ASSET_DESC& ElementDesc, const _wstring& strBindingPrefix) const
{
	_wstring strLocalTag;

	if (!ElementDesc.strBindingKey.empty())
	{
		strLocalTag = StringToWString(ElementDesc.strBindingKey);
	}
	else
	{
		// BindingKey가 없는 장식 엘리먼트는 ID 기반 내부 키 사용
		strLocalTag = L"__Element_" + to_wstring(ElementDesc.iElementID);
	}

	if (strBindingPrefix.empty())
		return strLocalTag;

	return strBindingPrefix + L"." + strLocalTag;
}

void CUI_Manager::Apply_UITransform(CUIElement* pElement, const UITransform& Transform, const _float2& vRuntimeScale)
{
	if (pElement == nullptr)
		return;

	_float2 vPosition = Transform.vPosition;
	_float2 vSize = Transform.vSize;

	vPosition.x *= vRuntimeScale.x;
	vPosition.y *= vRuntimeScale.y;

	vSize.x *= vRuntimeScale.x;
	vSize.y *= vRuntimeScale.y;

	pElement->Set_Position(vPosition);
	pElement->Set_Size(vSize);
	pElement->Set_Pivot(Transform.vPivot);
	pElement->Set_Anchor(Transform.eAnchor);
	pElement->Set_Rotation(Transform.fRotation);
	pElement->Set_Scale(Transform.vScale);
}

// Free 슬롯을 재사용하거나 새 슬롯을 추가하고 핸들을 발급
UI_HANDLE CUI_Manager::Allocate_UIRuntimeSlot()
{
	UI_HANDLE Handle = {};

	// 이전에 반환된 슬롯이 있으면 재사용
	if (!m_FreeUIRuntimeSlotIndices.empty())
	{
		const _uint iSlotIndex = m_FreeUIRuntimeSlotIndices.back();
		m_FreeUIRuntimeSlotIndices.pop_back();

		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[iSlotIndex];

		Slot.bOccupied = true;

		Handle.iIndex = iSlotIndex;
		Handle.iGeneration = Slot.iGeneration;

		return Handle;
	}

	// 빈 슬롯이 없으면 새 슬롯 생성
	UI_RUNTIME_SLOT Slot = {};
	Slot.bOccupied = true;

	m_UIRuntimeSlots.push_back(Slot);

	Handle.iIndex = static_cast<_uint>(m_UIRuntimeSlots.size() - 1);
	Handle.iGeneration = Slot.iGeneration;

	return Handle;
}

// 슬롯을 비우고 Generation을 증가시킨 뒤 Free 목록에 반환
void CUI_Manager::Release_UIRuntimeSlot(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	// 다음 인스턴스를 위해 기존 런타임 정보 초기화
	Slot.Instance = {};
	Slot.bOccupied = false;

	// 기존 핸들을 무효화하기 위해 세대 증가
	++Slot.iGeneration;

	// 오버플로로 0이 되면 유효 세대인 1로 변경
	if (Slot.iGeneration == 0)
		Slot.iGeneration = 1;

	m_FreeUIRuntimeSlotIndices.push_back(Handle.iIndex);
}

// 실제 런타임 슬롯을 검사, 인덱스가 슬롯 범위 안인지, 슬롯이 차있는 상태인지, 세대가 일치하는지 등
// Flush 전까지 슬롯은 살아있으니 제거 예약 직후에도 true
_bool CUI_Manager::Is_ValidUIHandle(const UI_HANDLE& Handle) const
{
	if (!Handle.Is_Valid())
		return false;

	if (Handle.iIndex >= m_UIRuntimeSlots.size())
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (!Slot.bOccupied)
		return false;

	return Slot.iGeneration == Handle.iGeneration;
}

_bool CUI_Manager::Is_SingletonPolicy(UI_INSTANCE_POLICY ePolicy) const
{
	return ePolicy == UI_INSTANCE_POLICY::SINGLETON || ePolicy == UI_INSTANCE_POLICY::SINGLETON_LAZY;
}

_float2 CUI_Manager::Get_RuntimeUIScale(const UI_ASSET_DESC& AssetDesc)
{
	const _float2& vReferenceResolution = AssetDesc.vReferenceResolution;

	if (vReferenceResolution.x <= 0.f || vReferenceResolution.y <= 0.f)
		return _float2(1.f, 1.f);

	_uint iNumViewports = 1;
	D3D11_VIEWPORT ViewpotDesc = {};

	m_pContext->RSGetViewports(&iNumViewports, &ViewpotDesc);

	// 유효한 뷰포트를 가져오지 못 했다면 원본 크기 사용
	if (iNumViewports == 0 || ViewpotDesc.Width <= 0.f || ViewpotDesc.Height <= 0.f)
		return _float2(1.f, 1.f);

	// ex : 기준 해상도 1280 * 720, 현재 화면을 1920 * 1080 으로 사용한다면
	// fScaleX = 1920 / 1280 = 1.5
	// fScaleY = 1080 / 720  = 1.5
	const _float fScaleX = ViewpotDesc.Width / vReferenceResolution.x;
	const _float fScaleY = ViewpotDesc.Height / vReferenceResolution.y;

	// fMatch == 0 -> 가로 비율만 사용, fMatch == 1 세로 비율만 사용, 0.5면 중간값
	const _float fFinalScale = fScaleX + (fScaleY - fScaleX) * AssetDesc.fMatch;

	return _float2(fFinalScale, fFinalScale);
}

_wstring CUI_Manager::Resolve_TexturePath(const string& strTexturePath)
{
	if (strTexturePath.empty())
		return L"";

	_wstring strTexturePathW = StringToWString(strTexturePath);

	replace(strTexturePathW.begin(), strTexturePathW.end(), L'/', L'\\');

	const _bool bAbsolutePath = strTexturePathW.find(L":") != _wstring::npos || strTexturePathW.find(L"\\\\") == 0;

	if (bAbsolutePath)
		return strTexturePathW;

	return m_pGameInstance->Get_ResourcePath() + L"\\" + strTexturePathW;
}

void CUI_Manager::Remove_UIObjectImmediate(const UI_HANDLE& Handle)
{
	// 핸들 검사
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	const string strAssetID = Slot.Instance.strAssetID;
	const _wstring strInstanceTag = Slot.Instance.strInstanceTag;
	const _wstring strControllerTag = Slot.Instance.strControllerTag;
	
	// 연결된 컨트롤러 제거
	// 컨트롤러가 UIObject와 Element를 비소유로 참조하므로 먼저 제거
	if (!strControllerTag.empty())
	{
		Remove_UIControllerImmediate(strControllerTag);

		Slot.Instance.strControllerTag.clear();
		Slot.Instance.pController = nullptr;
	}

	m_PendingSpawnInstanceTags.erase(strInstanceTag);

	CUIObject* pExpectedObject = Slot.Instance.pUIObject;

	// 싱글톤 Registry 정리
	if (Is_SingletonPolicy(Slot.Instance.eInstancePolicy))
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end() && iterSingleton->second == Handle)
		{
			m_SingletonUIHandles.erase(iterSingleton);
		}
	}

	// UIObject 제거
	auto iter = m_UIObjects.find(strInstanceTag);

	// 같은 태그가 재사용되었더라도 같은 인스턴스인지 확인
	if (iter != m_UIObjects.end() && iter->second == pExpectedObject)
	{
		Safe_Release(iter->second);
		m_UIObjects.erase(iter);
	}

	Remove_UIInputOwnerImmediate(Handle);

	// UIObject 제거가 끝난 뒤 슬롯 반환
	Release_UIRuntimeSlot(Handle);
}

void CUI_Manager::Remove_UIControllerImmediate(const _wstring& strControllerTag)
{
	auto iter = m_UIControllers.find(strControllerTag);
	if (iter == m_UIControllers.end())
		return;

	CUIControllerBase* pController = iter->second;

	// 해당 컨트롤러를 참조 중인 런타임 슬롯 연결 해제
	for (auto& Slot : m_UIRuntimeSlots)
	{
		if (!Slot.bOccupied)
			continue;

		if (Slot.Instance.pController != pController)
			continue;

		Slot.Instance.pController = nullptr;
		Slot.Instance.strControllerTag.clear();
	}

	Safe_Release(iter->second);
	m_UIControllers.erase(iter);
}

// 예약 슬롯을 이용해 실제 UIObject를 생성
void CUI_Manager::Execute_SpawnUIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (Slot.Instance.eState != UI_RUNTIME_STATE::PENDING_SPAWN)
		return;

	const string strAssetID = Slot.Instance.strAssetID;
	const _wstring strInstanceTag = Slot.Instance.strInstanceTag;
	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);

	if (pAssetDesc == nullptr)
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	if (FAILED(Create_UIObject(*pAssetDesc, strInstanceTag)))
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	CUIObject* pUIObject = Find_UIObject(strInstanceTag);

	if (pUIObject == nullptr)
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	Slot.Instance.pUIObject = pUIObject;
	Slot.Instance.eState = UI_RUNTIME_STATE::ACTIVE;

	pUIObject->Set_RenderSuppressed(Is_UILayerSuppressed(Slot.Instance.strLayer) || Is_UIInputOwnerRenderSuppressed(Handle));

	m_PendingSpawnInstanceTags.erase(strInstanceTag);
}

// Spawn 실패 시 싱글톤 등록, 예약 태그, 슬롯을 원상복구한다.
void CUI_Manager::Cancel_PendingSpawn(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	const string strAssetID = Slot.Instance.strAssetID;
	const _wstring strInstanceTag = Slot.Instance.strInstanceTag;

	m_PendingSpawnInstanceTags.erase(strInstanceTag);

	if (Is_SingletonPolicy(Slot.Instance.eInstancePolicy))
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end() && iterSingleton->second == Handle)
			m_SingletonUIHandles.erase(iterSingleton);
	}

	Release_UIRuntimeSlot(Handle);
}

// Late_Update 끝에서 컨트롤러 제거와 UI 커맨드 처리
void CUI_Manager::Flush_PendingCommands()
{
	// 컨트롤러가 UIObject를 비소유로 참조하므로 먼저 제거
	for (const _wstring& strControllerTag : m_PendingRemoveUIControllers)
	{
		Remove_UIControllerImmediate(strControllerTag);
	}

	m_PendingRemoveUIControllers.clear();

	// 처리 중 새 Command가 추가돼도 현재 반복자가 깨지지 않게 교체
	vector<UI_RUNTIME_COMMAND> Commands;
	Commands.swap(m_PendingUICommands);

	for (const UI_RUNTIME_COMMAND& Command : Commands)
	{
		switch (Command.eType)
		{
		case UI_RUNTIME_COMMAND_TYPE::SPAWN:
			Execute_SpawnUIObject(Command.hUI);
			break;

		case UI_RUNTIME_COMMAND_TYPE::RELEASE:
			Remove_UIObjectImmediate(Command.hUI);
			break;

		default:
			break;
		}
	}
}

// 전달된 UI가 확정된 Input Owner Stack 어디에든 존재하는지 검사
_bool CUI_Manager::Is_UIInputOwner(const UI_HANDLE& Handle) const
{
	for (const UI_INPUT_OWNER_ENTRY& Entry : m_UIInputOwnerStack)
	{
		if (Entry.hUI == Handle)
			return true;
	}

	return false;
}

// 같은 UI에 대해 이번 프레임의 Input Owner 명령이 이미 예약됐는지 검사
_bool CUI_Manager::Has_PendingUIInputOwnerCommand(const UI_HANDLE& Handle) const
{
	for (const UI_INPUT_OWNER_COMMAND& Command : m_PendingUIInputOwnerCommands)
	{
		if (Command.hUI == Handle)
			return true;
	}

	return false;
}

// 확정된 Input Owner Stack에 해당 종류가 존재하는지 검사한다.
_bool CUI_Manager::Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND eKind) const
{
	for (const UI_INPUT_OWNER_ENTRY& Entry : m_UIInputOwnerStack)
	{
		if (Entry.eKind == eKind)
			return true;
	}

	return false;
}

// 이번 프레임에 해당 종류의 Push가 이미 예약되어 있는지 검사한다.
_bool CUI_Manager::Has_PendingUIInputOwnerPushKind(UI_INPUT_OWNER_KIND eKind) const
{
	for (const UI_INPUT_OWNER_COMMAND& Command : m_PendingUIInputOwnerCommands)
	{
		if (Command.eType == UI_INPUT_OWNER_COMMAND_TYPE::PUSH && Command.eKind == eKind)
			return true;
	}

	return false;
}

// 전달된 UI가 가장 위쪽 Fullscreen Modal보다 아래에 있는지 검사한다.
_bool CUI_Manager::Is_UIInputOwnerRenderSuppressed(const UI_HANDLE& Handle) const
{
	if (!Handle.Is_Valid())
		return false;

	const size_t iInvalidIndex = m_UIInputOwnerStack.size();
	size_t iOwnerIndex = iInvalidIndex;
	size_t iTopFullscreenModalIndex = iInvalidIndex;

	for (size_t i = 0; i < m_UIInputOwnerStack.size(); ++i)
	{
		const UI_INPUT_OWNER_ENTRY& Entry = m_UIInputOwnerStack[i];

		// 전달된 UI가 Input Owner Stack에서 몇 번째에 있는지 기록한다.
		if (Entry.hUI == Handle)
			iOwnerIndex = i;

		// 순회하면서 계속 갱신하므로 가장 위쪽 Fullscreen Modal의 위치가 남는다.
		if (Entry.eKind == UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)
			iTopFullscreenModalIndex = i;
	}

	// Stack에 없는 일반 UI는 Input Owner 정책으로 숨기지 않는다.
	if (iOwnerIndex == iInvalidIndex)
		return false;

	// Fullscreen Modal이 없으면 Screen과 Overlay Modal을 모두 표시한다.
	if (iTopFullscreenModalIndex == iInvalidIndex)
		return false;

	// 가장 위쪽 Fullscreen Modal보다 아래에 있는 Owner만 숨긴다.
	return iOwnerIndex < iTopFullscreenModalIndex;
}

// UI Release 시 기존 Pending 명령을 취소하고 강제 제거를 예약
void CUI_Manager::Request_RemoveUIInputOwner(const UI_HANDLE& Handle)
{
	m_PendingUIInputOwnerCommands.erase(remove_if(m_PendingUIInputOwnerCommands.begin(), m_PendingUIInputOwnerCommands.end(), 
		[&Handle](const UI_INPUT_OWNER_COMMAND& Command) 
		{ 
			return Command.hUI == Handle; 
		}), m_PendingUIInputOwnerCommands.end());

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::REMOVE;
	Command.hUI = Handle;

	m_PendingUIInputOwnerCommands.push_back(Command);
}

// 전달된 UI를 확정된 Input Owner Stack의 모든 위치에서 즉시 제거
void CUI_Manager::Remove_UIInputOwnerImmediate(const UI_HANDLE& Handle)
{
	m_UIInputOwnerStack.erase(remove_if(m_UIInputOwnerStack.begin(), m_UIInputOwnerStack.end(), 
		[&Handle](const UI_INPUT_OWNER_ENTRY& Entry) 
		{ 
			return Entry.hUI == Handle;
		}), m_UIInputOwnerStack.end());
}

// Pending Input Owner 명령을 분리한 뒤 Push, Pop, Remove를 확정 Stack에 반영
void CUI_Manager::Flush_UIInputOwnerCommands()
{
	if (true == m_PendingUIInputOwnerCommands.empty())
		return;

	vector<UI_INPUT_OWNER_COMMAND> Commands;
	Commands.swap(m_PendingUIInputOwnerCommands);

	_bool bOwnerChanged = false;

	for (const UI_INPUT_OWNER_COMMAND& Command : Commands)
	{
		switch (Command.eType)
		{
		case UI_INPUT_OWNER_COMMAND_TYPE::PUSH:
		{
			if (!Is_OpenUIObject(Command.hUI))
				break;

			if (Is_UIInputOwner(Command.hUI))
				break;

			if (Command.eKind == UI_INPUT_OWNER_KIND::SCREEN && Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND::SCREEN))
				break;

			UI_INPUT_OWNER_ENTRY Entry = {};
			Entry.hUI = Command.hUI;
			Entry.eKind = Command.eKind;

			m_UIInputOwnerStack.push_back(Entry);
			bOwnerChanged = true;
			break;
		}

		case UI_INPUT_OWNER_COMMAND_TYPE::POP:
		{
			if (m_UIInputOwnerStack.empty())
				break;

			if (!(m_UIInputOwnerStack.back().hUI == Command.hUI))
				break;

			m_UIInputOwnerStack.pop_back();
			bOwnerChanged = true;
			break;
		}

		case UI_INPUT_OWNER_COMMAND_TYPE::REMOVE:
		{
			const size_t iPreviousSize = m_UIInputOwnerStack.size();
			Remove_UIInputOwnerImmediate(Command.hUI);

			if (iPreviousSize != m_UIInputOwnerStack.size())
				bOwnerChanged = true;

			break;
		}
		}
	}

	if (bOwnerChanged)
	{
		// 최상단 Input Owner가 바뀌었으므로 기존 버튼의 포커스와 캡처를 제거한다.
		Clear_ButtonFocus();
		Reset_ButtonPointerInput();

		// Push 또는 Pop 결과를 같은 프레임의 렌더링에 즉시 반영한다.
		Refresh_UIRenderSuppression();
	}
}

// 레이어 억제와 Input Owner Stack 억제를 합쳐 모든 런타임 UI의 최종 렌더 상태를 갱신한다.
void CUI_Manager::Refresh_UIRenderSuppression()
{
	for (_uint i = 0; i < static_cast<_uint>(m_UIRuntimeSlots.size()); ++i)
	{
		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[i];

		if (!Slot.bOccupied || Slot.Instance.pUIObject == nullptr)
			continue;

		UI_HANDLE Handle = {};
		Handle.iIndex = i;
		Handle.iGeneration = Slot.iGeneration;

		const _bool bSuppressedByLayer = Is_UILayerSuppressed(Slot.Instance.strLayer);
		const _bool bSuppressedByOwnerStack = Is_UIInputOwnerRenderSuppressed(Handle);

		// HUD 레이어 억제와 Fullscreen Modal 억제 중 하나라도 참이면 렌더링하지 않는다.
		Slot.Instance.pUIObject->Request_RenderSuppressed(bSuppressedByLayer || bSuppressedByOwnerStack);
	}
}

// 현재 마우스 아래에서 가장 위에 있는 Button을 찾는다.
CUIButton* CUI_Manager::Find_TopButtonAtPoint(const _float2& vUIPoint, UI_HANDLE* pOutHandle) const
{
	if (pOutHandle == nullptr)
		return nullptr;

	pOutHandle->Invalidate();

	// Input Owner가 없으면 어떤 Screen UI Button도 마우스 입력을 받지 않는다.
	if (m_UIInputOwnerStack.empty())
		return nullptr;

	const UI_HANDLE hInputOwner = m_UIInputOwnerStack.back().hUI;

	// 최상단 Owner가 잘못된 상태여도 아래 UI로 입력을 통과시키지 않는다.
	if (!Is_ValidUIHandle(hInputOwner))
		return nullptr;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[hInputOwner.iIndex];

	if (!Slot.bOccupied || Slot.Instance.eState != UI_RUNTIME_STATE::ACTIVE)
		return nullptr;

	if (Is_UILayerSuppressed(Slot.Instance.strLayer))
		return nullptr;

	CUIObject* pUIObject = Slot.Instance.pUIObject;

	if (pUIObject == nullptr || !pUIObject->Is_Visible())
		return nullptr;

	CUIButton* pResult = nullptr;
	_uint iBestButtonSort = 0;
	_bool bFound = false;

	// 최상단 Input Owner의 Button들 안에서 가장 높은 SortOrder를 찾는다.
	for (CUIButton* pButton : pUIObject->Get_Buttons())
	{
		if (pButton == nullptr || !pButton->Is_PointInside(vUIPoint))
			continue;

		const _uint iButtonSort = pButton->Get_RenderData().iSortOrder;
		const _bool bHigher = !bFound || iButtonSort >= iBestButtonSort;

		if (!bHigher)
			continue;

		bFound = true;
		iBestButtonSort = iButtonSort;
		pResult = pButton;
	}

	if (pResult != nullptr)
		*pOutHandle = hInputOwner;

	return pResult;
}

// 저장 중인 Button 포인터가 아직 유효한 UI에 속하는지 검사한다.
_bool CUI_Manager::Is_ValidButtonTarget(const UI_HANDLE& Handle, const CUIButton* pButton) const
{
	if (!Is_ButtonOwnedByUI(Handle, pButton))
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (Slot.Instance.eState != UI_RUNTIME_STATE::ACTIVE)
		return false;

	CUIObject* pUIObject = Slot.Instance.pUIObject;

	if (pUIObject == nullptr || !pUIObject->Is_Visible())
		return false;

	return  pButton->Is_InputAvailable();
}

// Button이 해당 UIObject에 여전히 소속되어 있는지 검사
_bool CUI_Manager::Is_ButtonOwnedByUI(const UI_HANDLE& Handle, const CUIButton* pButton) const
{
	if (pButton == nullptr || !Is_ValidUIHandle(Handle))
	{
		return false;
	}

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	CUIObject* pUIObject = Slot.Instance.pUIObject;

	if (pUIObject == nullptr)
		return false;

	const vector<CUIButton*>& Buttons = pUIObject->Get_Buttons();

	return find(Buttons.begin(), Buttons.end(), pButton) != Buttons.end();
}

// 마우스 Hover/Pressed/Pointer Capture를 한 곳에서 처리한다.
void CUI_Manager::Update_ButtonPointerInput()
{
	/*
	 * 이전 프레임에 저장한 포인터가 UI 제거로 무효가 되었는지
	 * 역참조 전에 Handle로 먼저 검사한다.
	 */
	if (!Is_ValidButtonTarget(m_hHoveredUI, m_pHoveredButton))
	{
		m_pHoveredButton = nullptr;
		m_hHoveredUI.Invalidate();
	}

	if (!Is_ValidButtonTarget(m_hCapturedUI, m_pCapturedButton))
	{
		m_pCapturedButton = nullptr;
		m_hCapturedUI.Invalidate();
	}

	_float2 vMouseClient = {};

	if (!m_pGameInstance->Get_MouseClientPosition(&vMouseClient))
	{
		Reset_ButtonPointerInput();
		return;
	}

	const _float2 vViewport = m_pGameInstance->Get_ViewportDesc();

	if (vViewport.x <= 0.f || vViewport.y <= 0.f)
	{
		Reset_ButtonPointerInput();
		return;
	}

	// 마우스가 게임 창의 Client 영역 밖이면 UI 입력을 취소
	if (vMouseClient.x < 0.f || vMouseClient.y < 0.f ||
		vMouseClient.x > vViewport.x || vMouseClient.y > vViewport.y)
	{
		Reset_ButtonPointerInput();
		return;
	}

	// 좌측 상단 기준 좌표를 UI 중앙 기준 좌표로 변환
	const _float2 vMouseUI =
	{
		vMouseClient.x - vViewport.x * 0.5f,
		vViewport.y * 0.5f - vMouseClient.y
	};

	// 이미 누른 Button이 있다면 Mouse Up까지 그 Button이 Pointer 입력을 독점한다.
	if (m_pCapturedButton != nullptr)
	{
		const _bool bInsideCaptured = m_pCapturedButton->Is_PointInside(vMouseUI);

		if (m_pGameInstance->Mouse_Up(DIMB::LBUTTON))
		{
			/*
			 * 멤버를 비우기 전에 클릭 성공 대상 정보를 복사한다.
			 * Controller가 Command 처리 중 UI 제거를 요청할 수도 있다.
			 */
			CUIButton* pClickedButton = m_pCapturedButton;

			const UI_HANDLE hClickedUI = m_hCapturedUI;

			pClickedButton->Set_PointerState(bInsideCaptured ? UI_BUTTON_POINTER_STATE::HOVERED : UI_BUTTON_POINTER_STATE::NORMAL);

			m_pHoveredButton = bInsideCaptured ? pClickedButton : nullptr;
			m_hHoveredUI = bInsideCaptured ? hClickedUI : UI_HANDLE{};

			// Command 전달 전에 Capture부터 종료한다.
			m_pCapturedButton = nullptr;
			m_hCapturedUI.Invalidate();

			// 누른 Button과 뗀 위치가 같은 유효 영역일 때만  Command가 발생
			if (bInsideCaptured && !pClickedButton->Should_CommandOnPress())
				Dispatch_ButtonCommand(hClickedUI, pClickedButton);

			return;
		}

		// 누른 채 밖으로 나가면 Normal, 다시 안으로 들어오면 Pressed로 돌아온다.
		m_pCapturedButton->Set_PointerState(bInsideCaptured ? UI_BUTTON_POINTER_STATE::PRESSED : UI_BUTTON_POINTER_STATE::NORMAL);

		return;
	}

	UI_HANDLE hNewHovered = {};

	CUIButton* pNewHovered = Find_TopButtonAtPoint(vMouseUI, &hNewHovered);

	if (m_pHoveredButton != pNewHovered)
	{
		if (m_pHoveredButton != nullptr)
		{
			m_pHoveredButton->Set_PointerState(UI_BUTTON_POINTER_STATE::NORMAL);
		}

		m_pHoveredButton = pNewHovered;
		m_hHoveredUI = hNewHovered;

		if (m_pHoveredButton != nullptr)
		{
			// 새로운 버튼 영역에 마우스가 진입한 순간에만 호버 사운드를 재생한다.
			m_pGameInstance->Play_Sound(UI_BUTTON_HOVER_SOUND_KEY, SOUND_SFX, 0.65f, false, false);
		}

		// 옵션이 활성화된 Button만  마우스 Hover와 동시에 Focus를 받는다.
		if (m_pHoveredButton != nullptr && m_pHoveredButton->Should_FocusOnHover())
		{
			Set_FocusedButton(m_hHoveredUI, m_pHoveredButton);
		}
	}

	if (m_pHoveredButton == nullptr)
		return;

	m_pHoveredButton->Set_PointerState(UI_BUTTON_POINTER_STATE::HOVERED);

	if (m_pGameInstance->Mouse_Down(DIMB::LBUTTON))
	{
		// 마우스로 누른 Button을 키보드/게임패드 Focus 대상으로 지정
		Set_FocusedButton(m_hHoveredUI, m_pHoveredButton);

		m_pCapturedButton = m_pHoveredButton;
		m_hCapturedUI = m_hHoveredUI;

		m_pCapturedButton->Set_PointerState(UI_BUTTON_POINTER_STATE::PRESSED);

		// 해당 옵션을 사용하는 버튼만 누르는 순간 실행한다.
		if (m_pCapturedButton->Should_CommandOnPress())
		{
			Dispatch_ButtonCommand(m_hCapturedUI, m_pCapturedButton);
		}
	}

}

// 현재 Pointer 상태를 모두 취소한다.
void CUI_Manager::Reset_ButtonPointerInput()
{
	if (Is_ValidButtonTarget(m_hCapturedUI, m_pCapturedButton))
	{
		m_pCapturedButton->Set_PointerState(UI_BUTTON_POINTER_STATE::NORMAL);
	}

	if (m_pHoveredButton != m_pCapturedButton && Is_ValidButtonTarget(m_hHoveredUI, m_pHoveredButton))
	{
		m_pHoveredButton->Set_PointerState(UI_BUTTON_POINTER_STATE::NORMAL);
	}

	m_pHoveredButton = nullptr;
	m_hHoveredUI.Invalidate();

	m_pCapturedButton = nullptr;
	m_hCapturedUI.Invalidate();
}

// 클릭이 성공한 Button의 Command를 연결된 Controller에 전달
void CUI_Manager::Dispatch_ButtonCommand(const UI_HANDLE& Handle, const CUIButton* pButton)
{
	if (!Is_ValidButtonTarget(Handle, pButton))
		return;

	const string& strCommandKey = pButton->Get_CommandKey();

	if (strCommandKey.empty())
		return;

	m_pGameInstance->Play_Sound(UI_BUTTON_CLICK_SOUND_KEY, SOUND_SFX, 0.85f, false, false);

	UI_COMMAND_EVENT Event = {};

	Event.hUI = Handle;
	Event.iElementID = pButton->Get_ElementID();
	Event.strRuntimeElementTag = pButton->Get_RuntimeElementTag();
	Event.strCommandKey = strCommandKey;

#ifdef _DEBUG
	const string strMessage = "[UI Command] " + Event.strCommandKey + "\n";

	OutputDebugStringA(strMessage.c_str());
#endif

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	/*
	 * Controller가 없는 테스트 UI도 Button 상태는 작동한다.
	 * Command는 Debug 출력까지만 하고 종료한다.
	 */
	if (Slot.Instance.pController == nullptr)
		return;

	Slot.Instance.pController->On_UICommand(Event);
}

void CUI_Manager::Dispatch_ButtonFocusEvent(const UI_HANDLE& Handle, const CUIButton* pButton, _bool bFocused)
{
	// Focus 해제 이벤트는 Button이 현재 입력 가능한지보다 해당 UI 소유인지 확인하는 것이 중요
	if (!Is_ButtonOwnedByUI(Handle, pButton))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (Slot.Instance.pController == nullptr)
		return;

	UI_FOCUS_EVENT Event = {};

	Event.hUI = Handle;
	Event.iElementID = pButton->Get_ElementID();
	Event.strRuntimeElementTag = pButton->Get_RuntimeElementTag();

	Event.strFocusGroup = pButton->Get_FocusGroup();
	Event.strCommandKey = pButton->Get_CommandKey();

	Event.bFocused = bFocused;

	Slot.Instance.pController->On_UIFocusChanged(Event);
}

// 현재 Focus Button을 변경한다.
void CUI_Manager::Set_FocusedButton(const UI_HANDLE& Handle, CUIButton* pButton)
{
	// 같은 Button이면 다시 상태를 계산하지 않는다.
	if (m_pFocusedButton == pButton && m_hFocusedUI == Handle)
		return;

	// 이전 Focus Button의 Focused 외형 제거
	if (Is_ButtonOwnedByUI(m_hFocusedUI, m_pFocusedButton))
	{
		CUIButton* pPreviousButton = m_pFocusedButton;

		const UI_HANDLE hPreviousUI = m_hFocusedUI;

		pPreviousButton->Set_Focused(false);

		if (pPreviousButton->Should_SelectOnFocus())
			pPreviousButton->Set_Selected(false);

		// 이전 대상이 Focus를 잃었다고 알린다.
		Dispatch_ButtonFocusEvent(hPreviousUI, pPreviousButton, false);
	}

	m_pFocusedButton = nullptr;
	m_hFocusedUI.Invalidate();

	// 새 Button이 실제 입력 가능한지 확인
	if (!Is_ValidButtonTarget(Handle, pButton))
		return;

	// 동일 FocusGroup에 속하고 SelectOnFocus가 활성화된 버튼들의 Selected를 먼저 모두 해제
	// 이전 Selected가 ViewModel 등 다른 경로로 설정되어 있어도 선택 프레임이 두 개 남지 않게 함
	if (pButton->Should_SelectOnFocus())
	{
		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

		CUIObject* pUIObject = Slot.Instance.pUIObject;

		if (pUIObject != nullptr)
		{
			const string& strFocusGroup = pButton->Get_FocusGroup();

			for (CUIButton* pGroupButton : pUIObject->Get_Buttons())
			{
				if (pGroupButton == nullptr)
					continue;

				if (!pGroupButton->Should_SelectOnFocus())
					continue;

				if (pGroupButton->Get_FocusGroup() != strFocusGroup)
					continue;

				pGroupButton->Set_Selected(false);
			}
		}
	}

	m_pFocusedButton = pButton;
	m_hFocusedUI = Handle;

	m_pFocusedButton->Set_Focused(true);

	if (m_pFocusedButton->Should_SelectOnFocus())
		m_pFocusedButton->Set_Selected(true);


	// 상태 변경이 끝난 뒤 새로운 Focus 정보를 전달한다.
	Dispatch_ButtonFocusEvent(m_hFocusedUI, m_pFocusedButton, true);
}

// 현재 Focus를 해제한다.
void CUI_Manager::Clear_ButtonFocus()
{
	if (Is_ButtonOwnedByUI(m_hFocusedUI, m_pFocusedButton))
	{
		CUIButton* pPreviousButton = m_pFocusedButton;

		const UI_HANDLE hPreviousUI = m_hFocusedUI;

		pPreviousButton->Set_Focused(false);

		if (pPreviousButton->Should_SelectOnFocus())
			pPreviousButton->Set_Selected(false);

		Dispatch_ButtonFocusEvent(hPreviousUI, pPreviousButton, false);
	}

	m_pFocusedButton = nullptr;
	m_hFocusedUI.Invalidate();
}

CUI_Manager* CUI_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_Manager* pInstance = new CUI_Manager(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CUI_Manager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_Manager::Free()
{
	__super::Free();

	Clear();

	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}
