#include "GameInstance.h"
#include "Graphic_Device.h"
#include "Input_Device.h"
#include "Object_Manager.h"
#include "Timer_Manager.h"
#include "Level_Manager.h"
#include "PipeLine.h"
#include "ResourceManager.h"
#include "Target_Manager.h"
#include "Renderer.h"
#include "MapManager.h"
#include "Light_Manager.h"
#include "Effect_Manager.h"
#include "UI_Manager.h"
#include "PhysicManager.h"
#include "Shader_Resolver.h"
#include "CameraManager.h"
#include "SpringArm.h"
#include "UiObject.h"
#include "UIControllerBase.h"
#include "SoundManager.h"
#include "GPUAnimationSystem.h"
#include "ThreadPool.h"
#include "AnimationLoader.h"
#ifdef _DEBUG

//IMGUI 관련
#include "ImguiManager.h"
#include "MapController.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM
	wParam, LPARAM lParam);

//Reflection 관련 

namespace Engine
{
	//unordered_map<string, FClassInfo> g_ReflectionMap;
	unordered_map<string, FClassInfo>& Get_ReflectionMaps()
	{
		//return g_ReflectionMap;
		static unordered_map<string, FClassInfo> s_ReflectionMap;
		return s_ReflectionMap;
	}
}

#endif

CGameInstance* CGameInstance::m_pInstance = nullptr;

CGameInstance::CGameInstance()
{
}

CGameInstance* CGameInstance::Get_Instance()
{
	return m_pInstance;
}

void CGameInstance::Create_Instance()
{
	if (nullptr == m_pInstance)
		m_pInstance = new CGameInstance;
}

void CGameInstance::Destroy_Instance()
{
	if (nullptr != m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}
}

#pragma region ENGINE

HRESULT CGameInstance::Initialize_Engine(const ENGINE_DESC& EngineDesc, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	m_vViewportDesc = _float2((_float)EngineDesc.iViewportWidth, (_float)EngineDesc.iViewportHeight);

	m_pGraphic_Device = CGraphic_Device::Create(EngineDesc.hWnd, EngineDesc.eWinMode, EngineDesc.iViewportWidth, EngineDesc.iViewportHeight, ppDevice, ppContext);
	if (nullptr == m_pGraphic_Device)
		return E_FAIL;

	m_pInput_Device = CInput_Device::Create(EngineDesc.hInstance, EngineDesc.hWnd);
	if (nullptr == m_pInput_Device)
		return E_FAIL;

	m_pTimer_Manager = CTimer_Manager::Create();
	if (nullptr == m_pTimer_Manager)
		return E_FAIL;

	m_pLevel_Manager = CLevel_Manager::Create(EngineDesc.hWnd);
	if (nullptr == m_pLevel_Manager)
		return E_FAIL;

	m_pPrototype_Manager = CPrototype_Manager::Create(EngineDesc.iNumLevels);
	if (nullptr == m_pPrototype_Manager)
		return E_FAIL;

	m_pObject_Manager = CObject_Manager::Create(EngineDesc.iNumLevels);
	if (nullptr == m_pObject_Manager)
		return E_FAIL;

	m_pPipeLine = CPipeLine::Create();
	if (nullptr == m_pPipeLine)
		return E_FAIL;

	m_pResource_Manager = CResource_Manager::Create();
	if (nullptr == m_pResource_Manager)
		return E_FAIL;

	m_pTarget_Manager = CTarget_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pTarget_Manager)
		return E_FAIL;

	m_pRenderer = CRenderer::Create(*ppDevice, *ppContext);
	if (nullptr == m_pRenderer)
		return E_FAIL;

	m_pMap_Manager = CMap_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pMap_Manager)
		return E_FAIL;

	m_pNavigation_Manager = CNavigation_Manager::Create();
	if (nullptr == m_pNavigation_Manager)
		return E_FAIL;

	m_pLight_Manager = CLight_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pLight_Manager)
		return E_FAIL;

	m_pPool_Manager = CPool_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pPool_Manager)
		return E_FAIL;

	m_pEvent_Manager = CEvent_Manager::Create();
	if (nullptr == m_pEvent_Manager)
		return E_FAIL;

	m_pCamera_Manager = CCamera_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pCamera_Manager)
		return E_FAIL;

	m_pPhysic_Manager = CPhysicManager::Create();
	if (nullptr == m_pPhysic_Manager)
		return E_FAIL;

	m_pEffect_Manager = CEffect_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pEffect_Manager)
		return E_FAIL;

	m_pUI_Manager = CUI_Manager::Create(*ppDevice, *ppContext);
	if (nullptr == m_pUI_Manager)
		return E_FAIL;

	m_pSound_Manager = CSound_Manager::Create();
	if (nullptr == m_pSound_Manager)
		return E_FAIL;

	m_pGPUAnimationSystem = CGPUAnimationSystem::Create(*ppDevice, *ppContext);
	if (nullptr == m_pGPUAnimationSystem)
		return E_FAIL;

	// 최소 2코어 이상 사용 가능하도록 설정
	_uint iUsableCores = thread::hardware_concurrency();
	iUsableCores = (iUsableCores < 2) ? 2 : iUsableCores - 2;

	const _uint iThreadCount = max(2, min(6, iUsableCores));

	m_pThreadPool = CThreadPool::Create(iThreadCount);
	if (nullptr == m_pThreadPool)
		return E_FAIL;

	PxSceneDesc scenecDesc(Get_Default_TolerancesScale());

	scenecDesc.cpuDispatcher = Get_Default_CpuDispatcher();
	scenecDesc.filterShader = CPhysicManager::CustomFilterShader;
	scenecDesc.gravity = PxVec3(0.f, PhysX::g_fGravity, 0.f);
	scenecDesc.flags |= PxSceneFlag::eENABLE_CCD | PxSceneFlag::eENABLE_PCM;

	m_pPhysic_Manager->Create_PxScene(scenecDesc);

	//Material Layout Set 
	CMaterial::Ready_MaterialLayout();

#ifdef _DEBUG

	m_pImguiManager = CImguiManager::Create(*ppDevice, *ppContext, EngineDesc.hWnd);
	if (nullptr == m_pImguiManager)
		return E_FAIL;
#endif

	return S_OK;

}

void CGameInstance::Update_Engine(_float fTimeDelta)
{
	//fTimeDelta : 메인 타이머에 스케일 적용된 델타(월드용). HitStop이나 전환 될 때 느려져야하는것들 대상
	//UnScaledTimeDelta : 실시간 델타(기존 델타. 스케일 적용X)

#ifdef _DEBUG
	if (m_pInput_Device->Key_Down(DIK_F2))
	{
		Start_MainTimer();
	}
	if (m_pInput_Device->Key_Down(DIK_F3))
	{
		Stop_MainTimer();
	}
	if (m_pInput_Device->Key_Down(DIK_F4))
	{
		Slow_MainTimer(1000.f, 0.5f);
	}
#endif // _DEBUG


	_float fUnscaled_TimeDelta = 0.f;
	m_pTimer_Manager->Get_MainTimeDeltaUnscaled(fUnscaled_TimeDelta);

	m_pRenderer->Update_ExposureScale(fUnscaled_TimeDelta);
	m_pRenderer->Update_SceneTime(fUnscaled_TimeDelta);

	m_pInput_Device->Update(fUnscaled_TimeDelta);

	m_pObject_Manager->Priority_Update(fTimeDelta);
	m_pObject_Manager->Update(fTimeDelta);
	m_pLevel_Manager->Update(fUnscaled_TimeDelta); // 페이드 타이머있어서 fUnscaled_TimeDelta로
	m_pCamera_Manager->Update(fTimeDelta);

	// Physic Step만큼 고정된 시간으로 물리 연산 업데이트
	//m_pPhysic_Manager->Accumulate_Time(fTimeDelta);

	//static _float g_fFixedTime = m_pPhysic_Manager->Get_StepTime();
	//while (m_pPhysic_Manager->Is_AccumulatedTime_Enough())
	//{
	//	m_pObject_Manager->Physical_Update(g_fFixedTime);

	//	m_pPhysic_Manager->Start_Simulation();
	//	m_pPhysic_Manager->Finish_Simulation();
	//}

	m_pObject_Manager->Physical_Update(fTimeDelta);
	//One Step 으로 변경
	m_pPhysic_Manager->Simulate_OneStep(fTimeDelta);

	m_pObject_Manager->Late_Update(fTimeDelta);
	m_pCamera_Manager->Late_Update(fTimeDelta);
	m_pPipeLine->Update();

	m_pSound_Manager->Update_Sound(fUnscaled_TimeDelta);

	m_pUI_Manager->Priority_Update(fUnscaled_TimeDelta);
	m_pUI_Manager->Update(fUnscaled_TimeDelta);
	m_pUI_Manager->Late_Update(fUnscaled_TimeDelta);

	m_pEffect_Manager->Tick(fTimeDelta);

#ifdef _DEBUG
	if (m_pPhysic_Manager->Is_VisualDebugger_Enable()) Update_VisualDebugger();
#endif

	//레벨 전환 체크
	m_pLevel_Manager->Late_Update(fUnscaled_TimeDelta);
}

HRESULT CGameInstance::Begin_Draw()
{
	_float4     vColor = _float4(0.f, 0.f, 1.f, 1.f);

	if (FAILED(m_pGraphic_Device->Clear_BackBuffer_View(&vColor)))
		return E_FAIL;

	if (FAILED(m_pGraphic_Device->Clear_DepthStencil_View()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGameInstance::Draw()
{
	if (FAILED(m_pRenderer->Draw()))
		return E_FAIL;

	if (FAILED(m_pLevel_Manager->Render()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGameInstance::End_Draw()
{
	return m_pGraphic_Device->Present();
}

CLevel* CGameInstance::Get_CurrentLevel() const
{
	return m_pLevel_Manager->Get_CurrentLevel();
}

void CGameInstance::Clear_Resources(_int iLevelIndex)
{
	if (-1 == iLevelIndex) return;

	m_pObject_Manager->Clear(iLevelIndex);
	m_pPrototype_Manager->Clear(iLevelIndex);
}

void CGameInstance::Clear_Layer(_uint iLevelIndex, const _wstring& strLayerTag)
{
	m_pObject_Manager->Clear_Layer(iLevelIndex, strLayerTag);
}

_bool CGameInstance::Is_LevelChanging() const
{
	return m_pLevel_Manager->Is_Changing();
}

void CGameInstance::Set_ResourcePath(const _wstring& strPath)
{
	m_strResourcePath = strPath;
	m_strResourcePathA = WStringToString(m_strResourcePath);
}

HRESULT CGameInstance::Update_MaterialBuffer()
{
	m_pResource_Manager->Clear_MaterialDirtyFlag();
	//vector<CMaterial*>* pMaterialVector = m_pResource_Manager->Get_MaterialVector();
	vector<CMaterial*> MaterialSnapshot;
	m_pResource_Manager->Get_MaterialSnapShot(MaterialSnapshot);

	if (FAILED(m_pRenderer->Update_MaterialBuffer(&MaterialSnapshot)))
	{
		MSG_BOX("Failed to Update Material Buffer");
		return E_FAIL;
	}
	return S_OK;
}
#pragma endregion
#pragma region INPUT_DEVICE

HRESULT CGameInstance::Resize_Window(_uint iNewWidth, _uint iNewHeight)
{
	return m_pGraphic_Device->Resize(iNewWidth, iNewHeight);
}

_long CGameInstance::Get_DIMouseMove(DIMM eMouseState)
{
	return m_pInput_Device->Get_DIMouseMove(eMouseState);
}
_bool CGameInstance::Get_MouseClientPosition(_float2* pOutPosition) const
{
	if (m_pInput_Device == nullptr)
		return false;

	return m_pInput_Device->Get_MouseClientPosition(pOutPosition);
}
bool CGameInstance::Key_Down(_ubyte byKeyID)
{
	return m_pInput_Device->Key_Down(byKeyID);
}
bool CGameInstance::Key_Pressing(_ubyte byKeyID)
{
	return m_pInput_Device->Key_Pressing(byKeyID);
}
bool CGameInstance::Key_Up(_ubyte byKeyID)
{
	return m_pInput_Device->Key_Up(byKeyID);
}

bool CGameInstance::Mouse_Down(DIMB eMouse)
{
	return m_pInput_Device->Mouse_Down(eMouse);
}

bool CGameInstance::Mouse_Pressing(DIMB eMouse)
{
	return m_pInput_Device->Mouse_Pressing(eMouse);
}

bool CGameInstance::Mouse_Up(DIMB eMouse)
{
	return m_pInput_Device->Mouse_Up(eMouse);
}
//아래 함수들은 엔진 생성전에 불릴수있어서 nullptr 체크 필수!!
void CGameInstance::Set_CursorState(CURSOR_STATE eState)
{
	if (m_pInput_Device)
		m_pInput_Device->Set_CursorState(eState);
}

CURSOR_STATE CGameInstance::Get_CursorState() const
{
	if (nullptr == m_pInput_Device)
		return CURSOR_STATE::FREE;

	return m_pInput_Device->Get_CursorState();
}

void CGameInstance::Notify_WindowInactive()
{
	if (m_pInput_Device)
		m_pInput_Device->Notify_WindowInActive();
}
#pragma endregion
#pragma region TIMER_MANAGER

_float CGameInstance::Get_TimeDelta(const _wstring& strTimerTag)
{
	return m_pTimer_Manager->Get_TimeDelta(strTimerTag);
}

HRESULT CGameInstance::Add_Timer(const _wstring& strTimerTag)
{
	return m_pTimer_Manager->Add_Timer(strTimerTag);
}

void CGameInstance::Update_Timer(const _wstring& strTimerTag)
{
	m_pTimer_Manager->Update_Timer(strTimerTag);
}

void CGameInstance::Stop_Timer(const _wstring& strTimerTag)
{
	m_pTimer_Manager->Stop_Timer(strTimerTag);
}
void CGameInstance::Start_Timer(const _wstring& strTimerTag)
{
	m_pTimer_Manager->Start_Timer(strTimerTag);
}

void CGameInstance::Set_TimeScale(const _wstring& strTimerTag, _float fTimeScale)
{
	m_pTimer_Manager->Set_TimeScale(strTimerTag, fTimeScale);
}

void CGameInstance::Reset_TimeScale(const _wstring& strTimerTag)
{
	m_pTimer_Manager->Reset_Timer(strTimerTag);
}

HRESULT CGameInstance::Register_MainTimer(const _wstring& strTimerTag)
{
	return m_pTimer_Manager->Register_MainTimer(strTimerTag);
}

void CGameInstance::Set_MainTimeScale(_float fScale)
{
	m_pTimer_Manager->Set_MainTimeScale(fScale);
}

void CGameInstance::Slow_MainTimer(_float fDuration, _float fChangeTimer)
{
	m_pTimer_Manager->Slow_MainTimer(fDuration, fChangeTimer);
}

void CGameInstance::Stop_MainTimer()
{
	m_pTimer_Manager->Stop_MainTimer();
}

void CGameInstance::Start_MainTimer()
{
	m_pTimer_Manager->Start_MainTimer();
}

#pragma endregion

#pragma region LEVEL_MANAGER
HRESULT CGameInstance::Request_ChangeLevel(_int iNewLevelIndex, CLevel* pNewLevel)
{
	return m_pLevel_Manager->Request_ChangeLevel(iNewLevelIndex, pNewLevel);
}
_int CGameInstance::Get_CurrentLevelIndex() const
{
	return m_pLevel_Manager->Get_CurrentLevelIndex();
}
#pragma endregion

#pragma region PROTOTYPE_MANAGER
HRESULT CGameInstance::Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype)
{
	return m_pPrototype_Manager->Add_Prototype(iLevelIndex, strPrototypeTag, pPrototype);
}
#pragma endregion

#pragma region OBJECT_MANAGER
CComponent* CGameInstance::Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strComponentTag, _uint iIndex)
{
	return m_pObject_Manager->Get_Component(iLevelIndex, strLayerTag, strComponentTag, iIndex);
}

CComponent* CGameInstance::Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strPartTag, const _wstring& strComponentTag, _uint iIndex)
{
	return m_pObject_Manager->Get_Component(iLevelIndex, strLayerTag, strPartTag, strComponentTag, iIndex);
}

CLayer* CGameInstance::Get_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag)
{
	return m_pObject_Manager->Get_Layer(iLayerLevelIndex, strLayerTag);
}

CLayer* CGameInstance::Get_CurrentLevel_Layer(const _wstring& strLayerTag)
{
	_uint iLevelIndex = ETOUI(Get_CurrentLevelIndex());
	return Get_Layer(iLevelIndex, strLayerTag);
}

HRESULT CGameInstance::Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, _uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg)
{
	return m_pObject_Manager->Add_GameObject(iPrototypeLevelIndex, strPrototypeTag, iLayerLevelIndex, strLayerTag, pArg);
}

HRESULT CGameInstance::Add_GameObject(_uint iLayerLevelIndex, const _wstring& strLayerTag, CGameObject* pObject)
{
	if (nullptr == pObject) return E_FAIL;
	return m_pObject_Manager->Add_GameObject(iLayerLevelIndex, strLayerTag, pObject);
}

#ifdef _DEBUG
const map<_wstring, class CLayer*>& CGameInstance::Get_Layers(_uint iLevelIndex) const
{
	return m_pObject_Manager->Get_Layers(iLevelIndex);
}
#endif // _DEBUG



#pragma endregion
#pragma region PIPELINE

const _float4x4* CGameInstance::Get_Transform(D3DTS eState) const
{
	return m_pPipeLine->Get_Transform(eState);
}

const _float4x4* CGameInstance::Get_Transform_Inverse(D3DTS eState) const
{
	return m_pPipeLine->Get_Transform_Inverse(eState);
}

const _float4* CGameInstance::Get_CamPosition() const
{
	return m_pPipeLine->Get_CamPosition();
}

const _float4* CGameInstance::Get_CamLook() const
{
	return m_pPipeLine->Get_CamLook();
}

const _float4* CGameInstance::Get_CamRight() const
{
	return m_pPipeLine->Get_CamRight();
}

void CGameInstance::Set_Transform(D3DTS eState, _fmatrix StateMatrix)
{
	m_pPipeLine->Set_Transform(eState, StateMatrix);
}

void CGameInstance::Fill_CameraData(PerCameraData& cameraData)
{
	m_pPipeLine->Fill_CameraData(cameraData);
}

#pragma endregion
#pragma region RESOURCE_MANAGER

HRESULT CGameInstance::Get_VIBuffer(_wstring strVIBufferName, CVIBuffer** pOutBuffer)
{
	return m_pResource_Manager->Get_VIBuffer(strVIBufferName, pOutBuffer);
}

HRESULT CGameInstance::Get_Texture(_wstring strTextureName, CTexture** pOutTexture)
{
	return m_pResource_Manager->Get_Texture(strTextureName, pOutTexture);
}

HRESULT CGameInstance::Get_Material(_wstring strMaterialName, CMaterial** pOutMaterial)
{
	return m_pResource_Manager->Get_Material(strMaterialName, pOutMaterial);
}

HRESULT CGameInstance::Get_Shader(_wstring strShaderName, CShader** pOutShader)
{
	return m_pResource_Manager->Get_Shader(strShaderName, pOutShader);
}

HRESULT CGameInstance::Get_Animation(_wstring strAnimationName, CAnimation** pOutAnimation)
{
	return m_pResource_Manager->Get_Animation(strAnimationName, pOutAnimation);
}

HRESULT CGameInstance::Get_Animation(_uint64 iAnimationKey, CAnimation** pOutAnimation)
{
	return m_pResource_Manager->Get_Animation(iAnimationKey, pOutAnimation);
}

HRESULT	 CGameInstance::Register_VIBuffer(CVIBuffer* pVIBuffer)
{
	if (pVIBuffer == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_VIBuffer(pVIBuffer);
}

HRESULT	 CGameInstance::Register_Shader(CShader* pShader)
{
	if (pShader == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_Shader(pShader);
}

HRESULT	 CGameInstance::Register_Texture(CTexture* pTexture)
{
	if (pTexture == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_Texture(pTexture);
}

HRESULT CGameInstance::Register_Material(CMaterial* pMaterial, _wstring KeyName)
{
	if (pMaterial == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_Material(pMaterial, KeyName);
}

#pragma region Animation 
//HRESULT CGameInstance::Register_StaticAnimation(CAnimation* pAnimation)
//{
//	if (pAnimation == nullptr) return E_FAIL;
//	return m_pResource_Manager->Input_StaticAnimation(pAnimation);
//}
//
//HRESULT CGameInstance::Register_DynamicAnimation(CAnimation* pAnimation)
//{
//	if (pAnimation == nullptr) return E_FAIL;
//	return m_pResource_Manager->Input_DynamicAnimation(pAnimation);
//}
#pragma endregion

#pragma region Animation
HRESULT CGameInstance::Register_StaticAnimation(CAnimation* pAnimation, _uint64 iKey)
{
	if (pAnimation == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_StaticAnimation(pAnimation, iKey);
}

HRESULT CGameInstance::Register_DynamicAnimation(CAnimation* pAnimation, _uint64 iKey)
{
	if (pAnimation == nullptr) return E_FAIL;
	return m_pResource_Manager->Input_DynamicAnimation(pAnimation, iKey);
}
#pragma endregion


HRESULT CGameInstance::Register_TextureFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _wstring& strTextureFilePath)
{
	if (strTextureFilePath.empty()) return E_FAIL;
	return m_pResource_Manager->Input_TextureFromFile(pDevice, pContext, strTextureFilePath);
}

HRESULT CGameInstance::Register_MaterialFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _wstring& strMaterialFilePath)
{
	if (strMaterialFilePath.empty()) return E_FAIL;
	return m_pResource_Manager->Input_MaterialFromFile(pDevice, pContext, strMaterialFilePath);
}

_bool CGameInstance::Is_MaterialDirty()
{
	return m_pResource_Manager->Is_MaterialDirty();
}
HRESULT CGameInstance::Ready_RawAnimations(const _wstring& strFoldername, const _wstring& strAnimationBaseName, _bool bStaticAnimation, ANIMATION_TIME_UNIT eTimeUnit)
{
	return CAnimationLoader::Ready_RawAnimations(this, strFoldername, strAnimationBaseName, eTimeUnit, bStaticAnimation);
}

#pragma endregion
#pragma region TARGET_MANAGER

HRESULT CGameInstance::Add_RenderTarget(const _wstring& strTargetTag, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
	return m_pTarget_Manager->Add_RenderTarget(strTargetTag, iWidth, iHeight, ePixelFormat, vClearColor);
}

HRESULT CGameInstance::Add_MRT(const _wstring& strMRTTag, const _wstring& strTargetTag)
{
	return m_pTarget_Manager->Add_MRT(strMRTTag, strTargetTag);
}

HRESULT CGameInstance::Begin_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV, _bool bUseDSV, _bool bClear)
{
	return m_pTarget_Manager->Begin_MRT(strMRTTag, pDSV, bUseDSV, bClear);
}

HRESULT CGameInstance::Bind_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV)
{
	return m_pTarget_Manager->Bind_MRT(strMRTTag, pDSV);
}

HRESULT CGameInstance::End_MRT()
{
	return m_pTarget_Manager->End_MRT();
}

HRESULT CGameInstance::Bind_RT_ShaderResource(const _wstring& strTargetTag, _uint iRegisterSlot)
{
	return m_pTarget_Manager->Bind_ShaderResource(strTargetTag, iRegisterSlot);
}

HRESULT CGameInstance::Copy_RT_Resource(const _wstring& strTargetTag, ID3D11Texture2D* pOut)
{
	return m_pTarget_Manager->Copy_Resource(strTargetTag, pOut);
}

ID3D11ShaderResourceView* CGameInstance::Get_RenderTargetSRV(const _wstring strTargetTag)
{
	return m_pTarget_Manager->Get_RenderTargetSRV(strTargetTag);
}

HRESULT CGameInstance::PopRenderTarget_InMRT(const _wstring& strMRTTag, const _wstring& strTargetTag)
{
	return m_pTarget_Manager->PopRenderTarget_InMRT(strMRTTag, strTargetTag);
}

HRESULT	CGameInstance::Remove_RenderTarget(const _wstring& strTargetTag)
{
	return m_pTarget_Manager->Remove_RenderTarget(strTargetTag);
}

#ifdef _DEBUG

HRESULT CGameInstance::Ready_RT_Debug(const _wstring& strTargetTag, _float fX, _float fY, _float fSizeX, _float fSizeY)
{
	return m_pTarget_Manager->Ready_Debug(strTargetTag, fX, fY, fSizeX, fSizeY);
}

HRESULT CGameInstance::Render_RT_Debug(const _wstring& strMRTTag, CShader* pShader, CVIBuffer_Rect* pVIBuffer, CConstantBuffer* pObjectBuffer)
{
	return m_pTarget_Manager->Render_Debug(strMRTTag, pShader, pVIBuffer, pObjectBuffer);
}

#endif
#pragma endregion

#pragma region RENDERER
void CGameInstance::Add_RenderGroup(RENDERID eGroupID, CGameObject* pGameObject)
{
	m_pRenderer->Add_RenderGroup(eGroupID, pGameObject);
}

void CGameInstance::Add_Decal(CDecal* pDecal)
{
	m_pRenderer->Add_Decal(pDecal);
}

RENDERID CGameInstance::Get_CurRenderPass()
{
	return m_pRenderer->Get_CurRenderPass();
}

void CGameInstance::Submit_Static(StaticCommand& StaticCommand)
{
	m_pRenderer->Submit_Static(StaticCommand);
}

void CGameInstance::Submit_Skinned(SkinnedCommand& command)
{
	m_pRenderer->Submit_Skinned(command);
}

void CGameInstance::Submit_Effect(EffectCommand& EffectCommand)
{
	m_pRenderer->Submit_Effect(EffectCommand);
}

void CGameInstance::Submit_UI(UICommand& UICommand)
{
	m_pRenderer->Submit_UI(UICommand);
}
void CGameInstance::Submit_UIText(UITextCommand& Command)
{
	m_pRenderer->Submit_UIText(Command);
}
void CGameInstance::Submit_Static_Resolved(StaticCommand& command, _bool bInstanced)
{
	m_pRenderer->Submit_Static_Resolved(command, bInstanced);
}

void CGameInstance::Submit_Skinned_Resolved(SkinnedCommand& command)
{
	m_pRenderer->Submit_Skinned_Resolved(command);
}

//추가
HRESULT	CGameInstance::Bind_CameraBuffer()
{
	return m_pRenderer->Bind_CameraBuffer();
}

//추가
HRESULT CGameInstance::Command_Execute()
{
	if (m_pGPUAnimationSystem != nullptr)
	{
		if (FAILED(m_pGPUAnimationSystem->Flush_SkinningJobs()))
			return E_FAIL;
	}

	return m_pRenderer->Command_Execute();
}

//추가
HRESULT CGameInstance::Bake_IBL()
{
	return m_pRenderer->Bake_IBL();
}

void CGameInstance::Add_FogVolume(const FogVolume& tVolume)
{
	m_pRenderer->Add_FogVolume(tVolume);
}

ID3D11ShaderResourceView* CGameInstance::Get_HiZSRV() const
{
	return m_pRenderer->Get_HiZSRV();
}

_bool CGameInstance::Get_HiZHistory(HIZ_HISTORY_DESC* pOutHistory) const
{
	return m_pRenderer->Get_HiZHistory(pOutHistory);
}
HRESULT CGameInstance::Resize_HiZ(_uint iViewportWidth, _uint iViewportHeight)
{
	return m_pRenderer->Resize_HiZ(iViewportWidth, iViewportHeight);
}

void CGameInstance::Set_ExposureScale(_float fTarget, _float fBlendTime)
{
	m_pRenderer->Set_ExposureScale(fTarget, fBlendTime);
}

void CGameInstance::Update_SceneTime(_float fTimeDelta)
{
	m_pRenderer->Update_SceneTime(fTimeDelta);
}

void CGameInstance::Set_ReflectionPlane(_float fHeight, _bool bEnable)
{
	m_pRenderer->Set_ReflectionPlane(fHeight, bEnable);
}

#ifdef _DEBUG
const RenderTuning& CGameInstance::Get_RenderTuning() const
{
	return m_pRenderer->Get_RenderTuning();
}

void CGameInstance::Set_RenderTuning(const RenderTuning& t)
{
	m_pRenderer->Set_RenderTuning(t);
}

void CGameInstance::Update_RenderTuning_DEBUG()
{
	m_pRenderer->Update_RenderTuning_DEBUG();
}

const PerIBLData& CGameInstance::Get_IBLData() const
{
	return m_pRenderer->Get_IBLData();
}

void CGameInstance::Set_IBLData(const PerIBLData& t)
{
	m_pRenderer->Set_IBLData(t);
}

void CGameInstance::Update_IBLData_DEBUG()
{
	m_pRenderer->Update_IBLData_DEBUG();
}

const RimProfile& CGameInstance::Get_RimProfile(int i) const
{
	return m_pRenderer->Get_RimProfile(i);
}

void CGameInstance::Set_RimProfile(int i, const RimProfile& t)
{
	m_pRenderer->Set_RimProfile(i, t);
}

void CGameInstance::Update_RimProfiles_DEBUG()
{
	m_pRenderer->Update_RimProfiles_DEBUG();
}

#endif
#pragma endregion

#pragma region LIGHT_MANAGER
const LIGHT_DESC* CGameInstance::Get_LightDesc(LIGHT eLightType, _uint iIndex)
{
	return m_pLight_Manager->Get_LightDesc(eLightType, iIndex);
}

_bool CGameInstance::Is_LightEnabled(LIGHT eLightType, _uint iIndex) const
{
	return m_pLight_Manager->Is_LightEnabled(eLightType, iIndex);
}

_uint CGameInstance::Get_LightCount(LIGHT eLightType) const
{
	return m_pLight_Manager->Get_LightCount(eLightType);
}

HRESULT CGameInstance::Add_Light(LIGHT eLightType, const LIGHT_DESC& LightDesc)
{
	return m_pLight_Manager->Add_Light(eLightType, LightDesc);
}

HRESULT CGameInstance::Fill_LightData(LIGHT eLightType, _uint iIndex, PerLightData& perLightData)
{
	return m_pLight_Manager->Fill_LightData(eLightType, iIndex, perLightData);
}

HRESULT CGameInstance::Set_LightEnable(LIGHT eLightType, _uint iIndex, _bool bEnable)
{
	return m_pLight_Manager->Set_LightEnable(eLightType, iIndex, bEnable);
}

HRESULT CGameInstance::Remove_Light(LIGHT eLightType, _uint iIndex)
{
	if (nullptr == m_pLight_Manager)
		return E_FAIL;

	return m_pLight_Manager->Remove_Light(eLightType, iIndex);
}

HRESULT CGameInstance::Set_LightDesc(LIGHT eLightType, _uint iIndex, const LIGHT_DESC& LightDesc)
{
	return m_pLight_Manager->Set_LightDesc(eLightType, iIndex, LightDesc);
}

#pragma endregion
#pragma region MAP_MANAGER

HRESULT CGameInstance::Load_Map(_wstring strMapName, class CMap** pOutMap, _float fMapScale, _bool bCreateEditorRealObjects)
{
	return m_pMap_Manager->Load_Map(strMapName, pOutMap, fMapScale, bCreateEditorRealObjects);
}

HRESULT CGameInstance::Load_HLODMap(const _wstring& strOriginalMapName, class CMap** pOutMap)
{
	return m_pMap_Manager->Load_HLODMap(strOriginalMapName, pOutMap);
}

HRESULT CGameInstance::Load_MapPhysics(const _wstring& strMapName)
{
	return m_pMap_Manager->Load_MapPhysics(strMapName);
}

void CGameInstance::Unload_MapPhysics(const _wstring& strMapName)
{
	m_pMap_Manager->Unload_MapPhysics(strMapName);
}

void CGameInstance::Set_MapPhysicsActive(const _wstring& strMapName, _bool bActive)
{
	m_pMap_Manager->Set_MapPhysicsActive(strMapName, bActive);
}

CMap* CGameInstance::Get_Map(_wstring strMapName)
{
	return m_pMap_Manager->Get_Map(strMapName);
}

_wstring CGameInstance::Get_MapFolderPath(const _wstring& strMapName) const
{
	return m_pMap_Manager->Get_MapFolderPath(strMapName);
}

_wstring CGameInstance::Get_MapRuntimeRelativePath(const _wstring& strMapName, const _wstring& strFileName) const
{
	return m_pMap_Manager->Get_MapRuntimeRelativePath(strMapName, strFileName);
}

HRESULT CGameInstance::Read_MapMeshGroup(const _wstring& strMapName, _bool bInstanced, _uint iGroupIndex, MeshPassData* pOutPassData) const
{
	return m_pMap_Manager->Read_MapMeshGroup(strMapName, bInstanced, iGroupIndex, pOutPassData);
}

HRESULT CGameInstance::Load_MapPhysicsSettings(const _wstring& strMapName)
{
	return m_pMap_Manager->Load_MapPhysicsSettings(strMapName);
}

HRESULT CGameInstance::Save_MapPhysicsSettings(const _wstring& strMapName)
{
	return m_pMap_Manager->Save_MapPhysicsSettings(strMapName);
}

HRESULT CGameInstance::Build_MapPhysics(const _wstring& strMapName)
{
	return m_pMap_Manager->Build_MapPhysics(strMapName);
}


MAP_PHYSICS_GEOMETRY_TYPE CGameInstance::Get_MapPhysicsType(const MapObjectSourceData& Source) const
{
	return m_pMap_Manager->Get_MapPhysicsType(Source);
}

void CGameInstance::Set_MapPhysicsType(const MapObjectSourceData& Source, MAP_PHYSICS_GEOMETRY_TYPE eGeometryType)
{
	m_pMap_Manager->Set_MapPhysicsType(Source, eGeometryType);
}

MAP_PHYSICS_ROLE CGameInstance::Get_MapPhysicsRole(const MapObjectSourceData& Source) const
{
	return m_pMap_Manager->Get_MapPhysicsRole(Source); // 에디터에서 선택 오브젝트의 충돌 역할을 조회한다.
}

void CGameInstance::Set_MapPhysicsRole(const MapObjectSourceData& Source, MAP_PHYSICS_ROLE eRole)
{
	m_pMap_Manager->Set_MapPhysicsRole(Source, eRole); // 에디터에서 선택 오브젝트의 충돌 역할을 저장한다.
}


#ifdef _DEBUG

_bool CGameInstance::Find_MapObjectSource(const CMapObject* pMapObject, MapObjectSourceData* pOutSource) const
{
	return m_pMap_Manager->Find_MapObjectSource(pMapObject, pOutSource);
}

void CGameInstance::Set_MapPhysicsDebugVisible(const _wstring& strMapName, _bool bVisible)
{
	m_pMap_Manager->Set_MapPhysicsDebugVisible(strMapName, bVisible);
}

#endif // _DEBUG

#pragma endregion





#pragma region PHYSIC_MANAGER

PxScene* CGameInstance::Get_CurrentPxScene()
{
	return m_pPhysic_Manager->Get_CurrentScene();
}

PxCpuDispatcher* CGameInstance::Get_Default_CpuDispatcher()
{
	return m_pPhysic_Manager->Get_Default_CpuDispatcher();
}

PxTolerancesScale& CGameInstance::Get_Default_TolerancesScale()
{
	return m_pPhysic_Manager->Get_Default_TolerancesScale();
}

PxMaterial* CGameInstance::Get_Default_Material()
{
	return m_pPhysic_Manager->Get_Default_Material();
}

void CGameInstance::Bind_TriggerCallback(function<void(PxTriggerPair*, PxU32)> func)
{
	m_pPhysic_Manager->Bind_TriggerCallback(func);
}

void CGameInstance::Bind_ContactCallback(function<void(const PxContactPairHeader&, const PxContactPair*, PxU32)> func)
{
	m_pPhysic_Manager->Bind_ContactCallback(func);
}

PxQueryFilterCallback* CGameInstance::Get_CCTQueryFilter()
{
	return m_pPhysic_Manager->Get_CCTQueryFilter();
}

PxControllerBehaviorCallback* CGameInstance::Get_CCTBehaviorCallback()
{
	return m_pPhysic_Manager->Get_CCTBehaviorCallBack();;
}

bool CGameInstance::Cook_TriangleMesh(PxTriangleMeshDesc& meshDesc, PxDefaultMemoryOutputStream& OutWriteBuffer)
{
	return m_pPhysic_Manager->Cook_TriangleMesh(meshDesc, OutWriteBuffer);
}
void CGameInstance::Reset_Filtering(PxActor* pActor)
{
	m_pPhysic_Manager->Reset_Filtering(pActor);
}

PxScene* CGameInstance::Create_PxScene(const PxSceneDesc& desc)
{
	return m_pPhysic_Manager->Create_PxScene(desc);
}

PxShape* CGameInstance::Create_PxShape(const PxGeometry& geometry, PxMaterial* material, bool isExclusive)
{
	return m_pPhysic_Manager->Create_PxShape(geometry, material, isExclusive);
}

PxMaterial* CGameInstance::Create_PxMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	return m_pPhysic_Manager->Create_PxMaterial(staticFriction, dynamicFriction, restitution);
}

PxRigidDynamic* CGameInstance::Create_RigidDynamic(_fvector vInitPos)
{
	return m_pPhysic_Manager->Create_RigidDynamic(vInitPos);
}

PxTriangleMesh* CGameInstance::Create_TriangleMesh(PxInputStream& stream)
{
	return m_pPhysic_Manager->Create_TriangleMesh(stream);
}

PxRigidStatic* CGameInstance::Create_RigidStatic(_fvector vInitPos, _fvector vInitQuat)
{
	return m_pPhysic_Manager->Create_RigidStatic(vInitPos, vInitQuat);
}

PxController* CGameInstance::Create_CapsuleController(PxCapsuleControllerDesc& desc)
{
	return m_pPhysic_Manager->Create_CapsuleController(desc);
}
PxController* CGameInstance::Create_BoxController(PxBoxControllerDesc& desc)
{
	return m_pPhysic_Manager->Create_BoxController(desc);
}

_bool CGameInstance::Sweep_Sphere(_float fRadius, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	return m_pPhysic_Manager->Sweep_Sphere(fRadius, vOrigin, vDir, fDistance, eTargetFilter, OutResult, QueryFlags);
}

_bool CGameInstance::Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	return m_pPhysic_Manager->Sweep_Capsule(fRadius, fHalfHeight, vOrigin, vDir, fDistance, eTargetFilter, OutResult, QueryFlags);
}

_bool CGameInstance::Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _vector vQuat, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	return m_pPhysic_Manager->Sweep_Capsule(fRadius, fHalfHeight, vOrigin, vQuat, vDir, fDistance, eTargetFilter, OutResult, QueryFlags);
}

_bool CGameInstance::Overlap_Sphere(_float fRadius, _fvector vOrigin, FILTER_GROUP eTargetFilter, vector<CGameObject*>& OutResults, PxQueryFlags QueryFlags)
{
	return m_pPhysic_Manager->Overlap_Sphere(fRadius, vOrigin, eTargetFilter, OutResults, QueryFlags);
}

void CGameInstance::Add_ActorToScene(PxActor* pActor)
{
	m_pPhysic_Manager->Add_ActorToScene(pActor);
}

void CGameInstance::Remove_ActorFromScene(PxActor* pActor)
{
	m_pPhysic_Manager->Remove_ActorFromScene(pActor);
}

_bool CGameInstance::Raycast(const _float3& vRayOrigin, const _float3& vRayDirection, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	return m_pPhysic_Manager->Raycast(vRayOrigin, vRayDirection, fDistance, eTargetFilter, OutResult, QueryFlags);
}

void CGameInstance::Invalidate_AllControllerCache()
{
	m_pPhysic_Manager->Invalidate_AllControllerCache();
}
#ifdef _DEBUG
const PxRenderBuffer& CGameInstance::Get_RenderBuffer() const
{
	return m_pPhysic_Manager->Get_RenderBuffer();
}

HRESULT CGameInstance::Create_VisualDebugger(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	return m_pPhysic_Manager->Create_DebugRenderer(pDevice, pContext);
}

void CGameInstance::Update_VisualDebugger()
{
	//피직스 디버그 랜더러 토글 
	if (Key_Down(DIK_H))
	{
		m_pPhysic_Manager->Toggle_VisualDebugger();
		m_bDebugFlag = !m_bDebugFlag;
	}

	//캐릭터 + 스윕 + 트리거 랜더 
	if (Key_Down(DIK_J))
	{
		m_iPhysXDebugMask ^= ETOUI(PHYSIC_DEBUG_MODE::CHARACTER);

		_bool bVisible = { (m_iPhysXDebugMask & ETOUI(PHYSIC_DEBUG_MODE::CHARACTER)) != 0 };
		m_pPhysic_Manager->Apply_DebugVisible(bVisible);
	}
	//지형 랜더 
	if (Key_Down(DIK_K))
	{
		m_iPhysXDebugMask ^= ETOUI(PHYSIC_DEBUG_MODE::TERRAIN);
		
		_bool bVisible = { (m_iPhysXDebugMask & ETOUI(PHYSIC_DEBUG_MODE::TERRAIN)) != 0 };
		m_pMap_Manager->Set_MapPhysicsDebugCategory(bVisible);
	}

	m_pPhysic_Manager->VisualDebugger_Update();
}

#endif //_DEBUG


#pragma endregion
#pragma region CAMERA_MANAGER
void CGameInstance::Set_FollowTarget(CSpringArm* pArm)
{
	m_pCamera_Manager->Set_FollowTarget(pArm);
}

CAMERA_MODE CGameInstance::Get_CameraMode()
{
	return m_pCamera_Manager->Get_CameraMode();
}

_bool CGameInstance::Try_Carmera_LockOn(_bool bLockOn)
{
	return m_pCamera_Manager->Try_LockOn(bLockOn);
}

HRESULT CGameInstance::Begin_CutSceneCamera(CCamera* pCutSceneCamera, _float fBlendIn)
{
	return m_pCamera_Manager->Begin_CutSceneCamera(pCutSceneCamera, fBlendIn);
}

HRESULT CGameInstance::Transition_CutSceneCamera(_float fBlendDuration)
{
	return m_pCamera_Manager->Transition_CutSceneCamera(fBlendDuration);
}

void CGameInstance::End_CutSceneCamera(_float fBlendOut)
{
	m_pCamera_Manager->End_CutSceneCamera(fBlendOut);
}

_bool CGameInstance::Is_CutSceneCameraPlaying() const
{
	return m_pCamera_Manager->Is_CutSceneCameraPlaying();
}

void CGameInstance::Mark_CameraDiscontinuity()
{
	if (nullptr != m_pCamera_Manager)
		m_pCamera_Manager->Mark_CameraDiscontinuity();
}

_uint CGameInstance::Get_CameraDiscontinuityRevision() const
{
	return nullptr != m_pCamera_Manager ? m_pCamera_Manager->Get_CameraDiscontinuityRevision() : 0u;
}

_float CGameInstance::Get_CameraFovy() const
{
	return m_pCamera_Manager->Get_MainCameraFovy();
}

HRESULT CGameInstance::Load_CameraShakeDatas(const string& strFolderpath)
{
	return m_pCamera_Manager->Load_ShakeDatas(strFolderpath);
}

HRESULT CGameInstance::Load_CameraSceneDatas(const string& strFolderpath)
{
	return m_pCamera_Manager->Load_SceneDatas(strFolderpath);
}

HRESULT CGameInstance::Load_CameraWorkDatas(const string& strFolderpath)
{
	return m_pCamera_Manager->Load_CameraWorkDatas(strFolderpath);
}

HRESULT	CGameInstance::Play_Shake(const string& strKey)
{
	return m_pCamera_Manager->Play_Shake(strKey);
}

void CGameInstance::Stop_Shake(const string& strKey)
{
	m_pCamera_Manager->Stop_Shake(strKey);
}

HRESULT	CGameInstance::Play_Shake(_uint64 iKeyHash)
{
	return m_pCamera_Manager->Play_Shake(iKeyHash);
}

void CGameInstance::Stop_Shake(_uint64 iKeyHash)
{
	m_pCamera_Manager->Stop_Shake(iKeyHash);
}

void CGameInstance::Stop_AllShakes()
{
	m_pCamera_Manager->Stop_AllShakes();
}


HRESULT	CGameInstance::Play_Scene(const string& strKey, _float fBlendIn, _float fBlendOut, _float fPlayRate)
{
	return m_pCamera_Manager->Play_Scene(strKey, fBlendIn, fBlendOut, fPlayRate);
}

void CGameInstance::Stop_Scene()
{
	m_pCamera_Manager->Stop_Scene();
}

HRESULT	CGameInstance::Play_CameraWork(const string& strKey, _fvector vTargetWorldPos)
{
	return m_pCamera_Manager->Play_CameraWork(strKey, vTargetWorldPos);
}

HRESULT	CGameInstance::Play_CameraWork(const string& strKey, CGameObject* pTarget)
{
	return m_pCamera_Manager->Play_CameraWork(strKey, pTarget);
}

void CGameInstance::Stop_CameraWork()
{
	m_pCamera_Manager->Stop_CameraWork();
}

void CGameInstance::Set_CameraWorkTarget(_fvector vTargetWorldPos)
{
	m_pCamera_Manager->Set_CameraWorkTarget(vTargetWorldPos);
}

void CGameInstance::Set_CameraLock(_bool bLock)
{
	m_pCamera_Manager->Set_CameraLock(bLock);
}

_bool CGameInstance::Is_ScenePlaying() const
{
	return m_pCamera_Manager->Is_ScenePlaying();
}

void CGameInstance::Clear_Camera()
{
	if (m_pCamera_Manager == nullptr) return;
	m_pCamera_Manager->Clear();
}

#ifdef _DEBUG
void CGameInstance::Set_CameraPosition(_fvector vPosition)
{
	m_pCamera_Manager->Set_CameraPosition(vPosition);
}
#endif // _DEBUG

_bool CGameInstance::Is_CameraLockOn()
{
	return m_pCamera_Manager->Is_LockOn();
}

//_float3 CGameInstance::Get_CameraLockTarget()
//{
//	return m_pCamera_Manager->Get_LockOnTarget();
//}

void CGameInstance::Set_Camera_LockPos(_vector vPos)
{
	m_pCamera_Manager->Set_LockOnTarget(vPos);
}

void CGameInstance::Begin_UIPreviewCamera(_fvector vEye, _fvector vAt, _float fFovy)
{
	m_pCamera_Manager->Begin_UIPreviewCamera(vEye, vAt, fFovy);
}

void CGameInstance::End_UIPreviewCamera()
{
	m_pCamera_Manager->End_UIPreviewCamera();
}

_bool CGameInstance::Is_UIPreviewCameraActive() const
{
	return m_pCamera_Manager->Is_UIPreviewCameraActive();
}
#pragma endregion
#pragma region IMGUI
#ifdef _DEBUG

void CGameInstance::Imgui_BeginFrame()
{
	m_pImguiManager->Begin_Frame();
}

void CGameInstance::Imgui_Render()
{
	m_pImguiManager->Render();
}

LRESULT CGameInstance::Imgui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

ImGuiContext* CGameInstance::Get_ImguiContext()
{
	return ImGui::GetCurrentContext();
}
#pragma endregion

#pragma region MAP_CONTROLLER

#pragma endregion


#endif 
#pragma endregion

#pragma region EVENT
//void CGameInstance::Subscribe(EVENT_TYPE _event, IListener* _listener)
//{
//	m_pEvent_Manager->Subscribe(_event, _listener);
//}
//
//void CGameInstance::UnSubscribe(EVENT_TYPE _event, IListener* _listener)
//{
//	m_pEvent_Manager->UnSubscribe(_event, _listener);
//}

void CGameInstance::UnSubscribe_AllEvent(IListener* _listener)
{
	m_pEvent_Manager->UnSubscribe_AllEvent(_listener);
}

//void CGameInstance::Broadcast(EVENT_TYPE _event, Event_Desc* _pData)
//{
//	m_pEvent_Manager->Broadcast(_event, _pData);
//}

void CGameInstance::Clear_All()
{
	m_pEvent_Manager->Clear_All();
}
#pragma endregion

#pragma region EVENT
CShader_Resolver* CGameInstance::Get_ShaderResolver()
{
	// Shader_Resolver 는 모든 Shader 가 생성 되고 나서 Initailze 를 호출 해야 하기때문에 Engine_Initialize 에서 생성을 해두면 안된다.
	// Ready_Prototype_Static 이 끝나고 진행이 되야하기때문에  Get 에서 nullcheck 후 생성 방식으로 진행한다.

	if (nullptr == m_pShader_Resolver)
	{
		m_pShader_Resolver = CShader_Resolver::Create();
		if (nullptr == m_pShader_Resolver)
			return nullptr;
	}

	return m_pShader_Resolver;
}
#pragma endregion

#pragma region EFFECT_MANAGER
void CGameInstance::Register_Effect(const string& strName, const EffectDesc& tDesc)
{
	m_pEffect_Manager->Register_Effect(strName, tDesc);
}

HRESULT CGameInstance::Load_Effect_File(const string& strName, const string& strPath)
{
	return m_pEffect_Manager->Load_Effect_File(strName, strPath);
}

const EMITTER_SHADER_BUNDLE* CGameInstance::Get_EffectShaderBundle() const
{
	return m_pEffect_Manager->Get_ShaderBundle();
}

CEffect* CGameInstance::Play_Effect(const string& strName, const _float3& vWorldPos)
{
	return m_pEffect_Manager->Play(strName, vWorldPos);
}

CEffect* CGameInstance::Play_Effect(const string& strName, const _float4x4* pFollowBoneMat,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, pFollowBoneMat, pOwnerWorld, vOffset, vAngle);
}

CEffect* CGameInstance::Play_Effect(const string& strName, CModel* pModel, const _char* pBoneName,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, pModel, pBoneName, pOwnerWorld, vOffset, vAngle);
}

CEffect* CGameInstance::Play_Effect(const string& strName, CModel* pModel, const _char* pBoneName,
	const _char* pBoneName2, const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, pModel, pBoneName, pBoneName2, pOwnerWorld, vOffset, vAngle);
}

CEffect* CGameInstance::Play_Effect_Model(const string& strName, CModel* pModel,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, pModel, pOwnerWorld, vOffset, vAngle);
}

CEffect* CGameInstance::Play_Effect(const string& strName, const _float4x4* pFollowBoneMat, const _float4x4* pFollowBoneMat2,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, pFollowBoneMat, pFollowBoneMat2, pOwnerWorld, vOffset, vAngle);
}

CEffect* CGameInstance::Play_Effect_Parts(const string& strName, const vector<CModel*>& Parts,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	return m_pEffect_Manager->Play(strName, Parts, pOwnerWorld, vOffset, vAngle);
}

void CGameInstance::Clear_Effects()
{
	m_pEffect_Manager->Clear();
}

#pragma endregion

#pragma region UI_MANAGER
HRESULT CGameInstance::Add_UIObject(const _wstring& strUITag, CUIObject* pUIObject)
{
	return m_pUI_Manager->Add_UIObject(strUITag, pUIObject);
}

HRESULT CGameInstance::Add_UIController(const _wstring& strControllerTag, CUIControllerBase* pController)
{
	return m_pUI_Manager->Add_UIController(strControllerTag, pController);
}
HRESULT CGameInstance::Spawn_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	return m_pUI_Manager->Spawn_UIObject(strAssetID, strInstanceTag, pOutHandle);
}
CUIObject* CGameInstance::Find_UIObject(const UI_HANDLE& Handle)
{
	return m_pUI_Manager->Find_UIObject(Handle);
}
CUIObject* CGameInstance::Find_UIObject(const _wstring& strUITag)
{
	return m_pUI_Manager->Find_UIObject(strUITag);
}
CUIControllerBase* CGameInstance::Find_UIController(const _wstring& strControllerTag)
{
	return m_pUI_Manager->Find_UIController(strControllerTag);
}
HRESULT CGameInstance::Set_UIImageTexture(const UI_HANDLE& Handle, const _wstring& strRuntimeElementTag, const string& strTexturePath)
{
	return m_pUI_Manager->Set_UIImageTexture(Handle, strRuntimeElementTag, strTexturePath);
}
void CGameInstance::Submit_UIRenderCommand()
{
	return m_pUI_Manager->Submit_RenderCommand();
}
HRESULT	CGameInstance::Register_UIController(const UI_HANDLE& Handle, const _wstring& strControllerTag, CUIControllerBase* pController)
{
	return m_pUI_Manager->Register_UIController(Handle, strControllerTag, pController);
}
HRESULT CGameInstance::Register_UIControllerFactory(const string& strControllerType, const UI_CONTROLLER_FACTORY& Factory)
{
	return m_pUI_Manager->Register_UIControllerFactory(strControllerType, Factory);
}
HRESULT CGameInstance::Register_UIAssetDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount, _wstring* pError)
{
	return m_pUI_Manager->Register_UIAssetDirectory(strDirectoryPath, pOutLoadedCount, pError);
}
HRESULT CGameInstance::Register_UIFont(const string& strFontAssetID, const _wstring& strFontPath)
{
	return m_pUI_Manager->Register_UIFont(strFontAssetID, strFontPath);
}
HRESULT CGameInstance::Create_UIController(const UI_CONTROLLER_CREATE_CONTEXT& Context)
{
	return m_pUI_Manager->Create_UIController(Context);
}
HRESULT CGameInstance::Create_UIObject(const string& strAssetID, const _wstring& strInstanceTag)
{
	return m_pUI_Manager->Create_UIObjectFromAsset(strAssetID, strInstanceTag);
}
HRESULT CGameInstance::Open_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	return m_pUI_Manager->Open_UIObject(strAssetID, strInstanceTag, pOutHandle);
}
HRESULT CGameInstance::Open_UIObject(const UI_HANDLE& Handle)
{
	return m_pUI_Manager->Open_UIObject(Handle);
}
HRESULT CGameInstance::Close_UIObject(const UI_HANDLE& Handle)
{
	return m_pUI_Manager->Close_UIObject(Handle);
}
void CGameInstance::Request_RemoveUIController(const _wstring& strControllerTag)
{
	return m_pUI_Manager->Request_RemoveUIController(strControllerTag);
}
void CGameInstance::Request_ReleaseUIObject(const UI_HANDLE& Handle)
{
	m_pUI_Manager->Request_ReleaseUIObject(Handle);
}
void CGameInstance::Request_ReleaseUILifetime(UI_LIFETIME eLifetime)
{
	m_pUI_Manager->Request_ReleaseLifetime(eLifetime);
}
HRESULT CGameInstance::Request_SpawnUIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	return m_pUI_Manager->Request_SpawnUIObject(strAssetID, strInstanceTag, pOutHandle);
}
HRESULT CGameInstance::Request_PushUIInputOwner(const UI_HANDLE& Handle, UI_INPUT_OWNER_KIND eKind)
{
	return m_pUI_Manager->Request_PushUIInputOwner(Handle, eKind);
}
HRESULT CGameInstance::Request_PopUIInputOwner(const UI_HANDLE& Handle)
{
	return m_pUI_Manager->Request_PopUIInputOwner(Handle);
}
_bool CGameInstance::Is_AliveUIObject(const UI_HANDLE& Handle) const
{
	return m_pUI_Manager->Is_AliveUIObject(Handle);
}
_bool CGameInstance::Is_OpenUIObject(const UI_HANDLE& Handle) const
{
	return m_pUI_Manager->Is_OpenUIObject(Handle);
}
_bool CGameInstance::Is_PendingSpawnUIObject(const UI_HANDLE& Handle) const
{
	return m_pUI_Manager->Is_PendingSpawnUIObject(Handle);
}
_bool CGameInstance::Contains_UIFont(const string& strFontAssetID) const
{
	return m_pUI_Manager->Contains_UIFont(strFontAssetID);
}
_bool CGameInstance::Has_UIInputOwner() const
{
	return m_pUI_Manager->Has_UIInputOwner();
}
_bool CGameInstance::Is_TopUIInputOwner(const UI_HANDLE& Handle) const
{
	return m_pUI_Manager->Is_TopUIInputOwner(Handle);
}
_bool CGameInstance::Can_PushUI(UI_INPUT_OWNER_KIND eKind) const
{
	return m_pUI_Manager->Can_PushUI(eKind);
}
HRESULT CGameInstance::Register_UIAsset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation, _wstring* pError)
{
	return m_pUI_Manager->Register_UIAsset(strFilePath, pOutValidation, pError);
}
const UI_ASSET_DESC* CGameInstance::Find_UIAsset(const string& strAssetID) const
{
	return m_pUI_Manager->Find_UIAsset(strAssetID);
}
_bool CGameInstance::Contains_UIAsset(const string& strAssetID) const
{
	return m_pUI_Manager->Contains_UIAsset(strAssetID);
}
size_t CGameInstance::Get_UIAssetCount() const
{
	return m_pUI_Manager->Get_UIAssetCount();
}
void CGameInstance::Acquire_UILayerSuppression(const string& strLayer)
{
	m_pUI_Manager->Acquire_UILayerSuppression(strLayer);
}
void CGameInstance::Release_UILayerSuppression(const string& strLayer)
{
	m_pUI_Manager->Release_UILayerSuppression(strLayer);
}
#pragma endregion

#pragma region NAVIGATION_MANAGER

HRESULT CGameInstance::Load_Navigation(const _wstring& strChapterKey, _uint iSectorID, const _wstring& strNavigationPath, const _wstring& strNeighborPath)
{
	return m_pNavigation_Manager->Load_Navigation(strChapterKey, iSectorID, strNavigationPath, strNeighborPath);
}

CNavigation* CGameInstance::Get_Navigation(const _wstring& strChapterKey, _uint iSectorID) const
{
	return m_pNavigation_Manager->Get_Navigation(strChapterKey, iSectorID);
}

void CGameInstance::Clear_Navigations(const _wstring& strChapterKey)
{
	m_pNavigation_Manager->Clear_Navigations(strChapterKey);
}

#pragma endregion
#pragma region SOUND_MANAGER

void CGameInstance::Ready_Sound()
{
	m_pSound_Manager->Ready_Sound();
}

void CGameInstance::Update_Sound(_float fTimeDelta)
{
	m_pSound_Manager->Update_Sound(fTimeDelta);
}

void CGameInstance::Play_Sound(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, _bool bTrack)
{
	m_pSound_Manager->Play_Sound(strSoundKey, eGroup, fVolume, bLoop, bTrack);
}
void CGameInstance::Play_SFXSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
	m_pSound_Manager->Play_SFXSound(strSoundKey, fVolume, bLoop, bTrack);
}

void CGameInstance::Play_BGM(const string& strSoundKey, _float fVolume)
{
	m_pSound_Manager->Play_BGM(strSoundKey, fVolume);
}

void CGameInstance::Play_BGM(const string& strSoundKey, _float fVolume, _float fFadeSec)
{
	if (nullptr == m_pSound_Manager)
		return;

	m_pSound_Manager->Play_BGM(strSoundKey, fVolume, fFadeSec);
}

void CGameInstance::Play_BGM_Sequence(const string& strIntroKey, const string& strLoopKey, _float fVolume)
{
	if (nullptr == m_pSound_Manager)
		return;

	m_pSound_Manager->Play_BGM_Sequence(strIntroKey, strLoopKey, fVolume);
}

void CGameInstance::Play_MonsterSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
	m_pSound_Manager->Play_MonsterSound(strSoundKey, fVolume, bLoop, bTrack);
}

void CGameInstance::Play_PlayerSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack, _float fPitch)
{
	m_pSound_Manager->Play_PlayerSound(strSoundKey, fVolume, bLoop, bTrack, fPitch);
}

void CGameInstance::Play_WeaponSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
	m_pSound_Manager->Play_WeaponSound(strSoundKey, fVolume, bLoop, bTrack);
}

void CGameInstance::Play_Ambience(const string& strSoundKey, _float fVolume, _float fFadeSec)
{
	if (nullptr == m_pSound_Manager)
		return;

	m_pSound_Manager->Play_Ambience(strSoundKey, fVolume, fFadeSec);
}

void CGameInstance::Stop_Ambience()
{
	if (nullptr == m_pSound_Manager)
		return;

	m_pSound_Manager->Stop_Ambience();
}

_uint CGameInstance::Play_SoundAt(const string& strSoundKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolume, _bool bLoop)
{
	return m_pSound_Manager->Play_SoundAt(strSoundKey, eGroup, vWorldPos, fVolume, bLoop);
}

_uint CGameInstance::Play_SoundFollow(const string& strSoundKey, CHANNEL_ID eGroup, const _float4x4* pFollowMat, _float fVolume, _bool bLoop, _bool bDoppler)
{
	return m_pSound_Manager->Play_SoundFollow(strSoundKey, eGroup, pFollowMat, fVolume, bLoop, bDoppler);
}

void CGameInstance::Update_SoundSourcePos(_uint hHandle, const _float3& vWorldPos)
{
	m_pSound_Manager->Update_SoundSourcePos(hHandle, vWorldPos);
}

void CGameInstance::Stop_SoundSource(_uint hHandle)
{
	m_pSound_Manager->Stop_SoundSource(hHandle);
}

_bool CGameInstance::Is_SoundSourcePlaying(_uint hHandle)
{
	return m_pSound_Manager->Is_SoundSourcePlaying(hHandle);
}

void CGameInstance::SetVolume_ByKey(const string& strSoundKey, _float fVolume)
{
	m_pSound_Manager->SetVolume_ByKey(strSoundKey, fVolume);
}

void CGameInstance::Set_GroupVolume(CHANNEL_ID eID, _float fVolume)
{
	m_pSound_Manager->Set_GroupVolume(eID, fVolume);
}

void CGameInstance::Set_GroupMute(CHANNEL_ID eGroup, _bool bMute)
{
	m_pSound_Manager->Set_GroupMute(eGroup, bMute);
}

void CGameInstance::Set_MasterMute(_bool bMute)
{
	m_pSound_Manager->Set_MasterMute(bMute);
}

_bool CGameInstance::IsPlaying_ByKey(const string& strSoundKey)
{
	return m_pSound_Manager->IsPlaying_ByKey(strSoundKey);
}

void CGameInstance::FadeOut_BGM(_float fFadeSec)
{
	m_pSound_Manager->FadeOut_BGM(fFadeSec);
}

void CGameInstance::StopSound_ByKey(const string& strSoundKey)
{
	m_pSound_Manager->StopSound_ByKey(strSoundKey);
}

void CGameInstance::Stop_GroupSound(CHANNEL_ID eID)
{
	m_pSound_Manager->Stop_GroupSound(eID);
}

void CGameInstance::Stop_AllSound()
{
	m_pSound_Manager->Stop_AllSound();

}
void CGameInstance::Stop_BGM()
{
	m_pSound_Manager->Stop_BGM();
}

void CGameInstance::Pause_Group(CHANNEL_ID eGroup, _bool bPause)
{
	m_pSound_Manager->Pause_Group(eGroup, bPause);
}

void CGameInstance::Pause_All(_bool bPause)
{
	m_pSound_Manager->Pause_All(bPause);
}

_bool CGameInstance::Can_PlaySoundByInterval(const string& strRuleKey, _float fIntervalSec)
{
	return m_pSound_Manager->Can_PlaySoundByInterval(strRuleKey, fIntervalSec);
}

void CGameInstance::Play_SoundSequence(const CSound_Manager::SOUND_SEQUENCE_DESC& Desc)
{
	m_pSound_Manager->Play_SoundSequence(Desc);
}

void CGameInstance::Stop_SoundSequence(const string& strSequenceKey)
{
	m_pSound_Manager->Stop_SoundSequence(strSequenceKey);
}

_uint CGameInstance::Play_Cue(const string& strCueKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolumeScale, _bool bLoop)
{
	if (nullptr == m_pSound_Manager)
		return 0;

	return m_pSound_Manager->Play_Cue(strCueKey, eGroup, vWorldPos, fVolumeScale, bLoop);
}

_bool CGameInstance::Has_Cue(const string& strCueKey) const
{
	if (nullptr == m_pSound_Manager)
		return false;

	return m_pSound_Manager->Has_Cue(strCueKey);
}


CThreadPool* CGameInstance::Get_ThreadPool()
{
	return m_pThreadPool;
}

#pragma endregion

#pragma region GPUANIMATION_SYSTEM
HRESULT CGameInstance::Allocate_FinalBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset)
{
	return m_pGPUAnimationSystem->Allocate_FinalBoneMatrixRange(iBoneMatrixCount, pOutOffset);
}
HRESULT CGameInstance::Free_FinalBoneMatrixRange(_uint iOffset, _uint iCount)
{
	return m_pGPUAnimationSystem->Free_FinalBoneMatrixRange(iOffset, iCount);
}
HRESULT CGameInstance::Allocate_CombinedBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset)
{
	return m_pGPUAnimationSystem->Allocate_CombinedBoneMatrixRange(iBoneMatrixCount, pOutOffset);
}
HRESULT CGameInstance::Free_CombinedBoneMatrixRange(_uint iOffset, _uint iCount)
{
	return m_pGPUAnimationSystem->Free_CombinedBoneMatrixRange(iOffset, iCount);
}
ID3D11ShaderResourceView* CGameInstance::Get_GlobalFinalBoneMatrixSRV() const
{
	return m_pGPUAnimationSystem->Get_FinalBoneMatrixSRV();
}
_uint CGameInstance::Get_GlobalFinalBoneMatrixCapacity() const
{
	return m_pGPUAnimationSystem->Get_FinalBoneMatrixCapacity();
}
HRESULT CGameInstance::Update_GlobalCombinedBoneMatrixRange(_uint iOffset, const _float4x4* pCombinedMatrices, _uint iCount)
{
	return m_pGPUAnimationSystem->Update_GlobalCombinedBoneMatrixRange(iOffset, pCombinedMatrices, iCount);
}
HRESULT CGameInstance::Register_MeshBoneInputRange(const vector<_uint>& BoneIndices, const vector<_float4x4>& OffsetMatrices, _uint* pOutOffset, _uint* pOutCount)
{
	return m_pGPUAnimationSystem->Register_MeshBoneInputRange(BoneIndices, OffsetMatrices, pOutOffset, pOutCount);
}
HRESULT CGameInstance::Free_MeshBoneInputRange(_uint iOffset, _uint iCount)
{
	return m_pGPUAnimationSystem->Free_MeshBoneInputRange(iOffset, iCount);
}
HRESULT CGameInstance::Register_ModelSkinningJob(CModel* pModel)
{
	return m_pGPUAnimationSystem->Register_ModelSkinningJob(pModel);
}
HRESULT CGameInstance::Flush_SkinningJobs()
{
	return m_pGPUAnimationSystem->Flush_SkinningJobs();
}
#pragma endregion


void CGameInstance::Release_Engine()
{
	//로딩 스레드에 취소 신호
	Request_LoadingCancel();

	Safe_Release(m_pLevel_Manager);//로더 스레드 join
	Safe_Release(m_pThreadPool); //Level 뒤, 그 중에서 맨 앞 
#ifdef _DEBUG
	Safe_Release(m_pImguiManager);
#endif
	Safe_Release(m_pSound_Manager);
	Safe_Release(m_pUI_Manager);
	Safe_Release(m_pShader_Resolver);
	Safe_Release(m_pPool_Manager);
	Safe_Release(m_pEffect_Manager);
	Safe_Release(m_pRenderer);
	Safe_Release(m_pTarget_Manager);
	Safe_Release(m_pResource_Manager);
	Safe_Release(m_pPipeLine);
	Safe_Release(m_pObject_Manager);
	Safe_Release(m_pPrototype_Manager);
	Safe_Release(m_pMap_Manager); // 오브젝트 뒤에 삭제
	Safe_Release(m_pGPUAnimationSystem);
	Safe_Release(m_pNavigation_Manager);

	Safe_Release(m_pLight_Manager);
	Safe_Release(m_pCamera_Manager);
	Safe_Release(m_pPhysic_Manager); //순서 주의
	Safe_Release(m_pEvent_Manager);
	Safe_Release(m_pTimer_Manager);
	Safe_Release(m_pInput_Device);
	Safe_Release(m_pGraphic_Device);
}
