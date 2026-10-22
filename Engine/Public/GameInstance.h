#pragma once

#include "Prototype_Manager.h"
#include "PoolManager.h"
#include "IListener.h"

#include "MapManager.h"
#include "Navigation_Manager.h"
#include "UIAssetManager.h"
#include "UI_RuntimeData.h"
#include "AnimationData.h"

#ifdef _DEBUG
#include "Engine_Reflection.h"
#endif
#include "EventManager.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

struct  StaticCommand;
struct  SkinnedCommand;
struct  EffectCommand;
struct  UICommand;
struct  EMITTER_SHADER_BUNDLE;

class ENGINE_DLL CGameInstance final
{
private:
	CGameInstance();
	CGameInstance(const CGameInstance& rhs) = delete;
	CGameInstance& operator=(const CGameInstance& rhs) = delete;
	virtual ~CGameInstance() = default;

public:
	static CGameInstance* Get_Instance();

	//클라이언트 시작 시 한 번만 호출
	static void					Create_Instance();
	//클라이언트 종료 시 한 번만 호출
	static void					Destroy_Instance();

private:
	static CGameInstance* m_pInstance;

#pragma region ENGINE

public:
	_float2						Get_ViewportDesc() { return m_vViewportDesc; }


	// Editor 모드에서 사용하기
	void						Set_ViewportDesc(_float2 ViewportSize)
	{
		m_vViewportDesc = ViewportSize;
	}

public:
	HRESULT						Initialize_Engine(const ENGINE_DESC& EngineDesc, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext);
	void						Update_Engine(_float fTimeDelta);
	HRESULT						Begin_Draw();
	HRESULT						Draw();
	HRESULT						End_Draw();

public:
	class CLevel* Get_CurrentLevel() const;
	void						Request_Respawn();

	void						Clear_Resources(_int iLevelIndex);
	void						Clear_Layer(_uint iLevelIndex, const _wstring& strLayerTag);
	_bool						Is_LevelChanging() const;

public:
	void						Set_Player(class CCharacter* pPlayer) { m_pPlayer = pPlayer; }
	class CCharacter* Get_Player() const { return m_pPlayer; }

	//HRESULT					Bind_Buffers();

	void 						Set_ResourcePath(const _wstring& strPath);
	const _wstring& Get_ResourcePath() const { return m_strResourcePath; }
	const string& Get_ResourcePathA() const { return m_strResourcePathA; }
	const _wstring& Get_TexturePath() const { return m_strTexturePath; }
	HRESULT						Update_MaterialBuffer();

#ifdef _DEBUG
	_bool						Get_DebugFlag()
	{
		_bool m_bCharacterDebug = { (m_iPhysXDebugMask & ETOUI(PHYSIC_DEBUG_MODE::CHARACTER)) != 0 };
		return m_bDebugFlag && m_bCharacterDebug;
	}

	_uint Get_PhysXDebugMask() const { return m_iPhysXDebugMask; }
#endif

#pragma endregion
#pragma region  GRAPHIC_DEVICE
	HRESULT						Resize_Window(_uint iNewWidth, _uint iNewHeight);
#pragma endregion
#pragma region  INPUT_DEVICE
	_long						Get_DIMouseMove(DIMM eMouseState);
	// 추가
	// 게임 창 내부의 현재 마우스 좌표
	_bool						Get_MouseClientPosition(_float2* pOutPosition) const;

	bool						Key_Down(_ubyte byKeyID);
	bool						Key_Pressing(_ubyte byKeyID);
	bool						Key_Up(_ubyte byKeyID);

	bool						Mouse_Down(DIMB eMouse);
	bool						Mouse_Pressing(DIMB eMouse);
	bool						Mouse_Up(DIMB eMouse);

	void						Set_CursorState(CURSOR_STATE eState);
	CURSOR_STATE				Get_CursorState() const;
	void						Notify_WindowInactive();
#pragma endregion
#pragma region  TIMER_MANAGER
	_float						Get_TimeDelta(const _wstring& strTimerTag);
	HRESULT						Add_Timer(const _wstring& strTimerTag);
	void						Update_Timer(const _wstring& strTimerTag);
	void						Stop_Timer(const _wstring& strTimerTag);
	void						Start_Timer(const _wstring& strTimerTag);

	void						Set_TimeScale(const _wstring& strTimerTag, _float fTimeScale);
	void						Reset_TimeScale(const _wstring& strTimerTag);

	HRESULT						Register_MainTimer(const _wstring& strTimerTag);

	void						Set_MainTimeScale(_float fScale);
	void						Slow_MainTimer(_float fDuration, _float fChangeTimer);

	void						Stop_MainTimer();
	void						Start_MainTimer();

#pragma endregion
#pragma region  LEVEL_MANAGER
	HRESULT						Request_ChangeLevel(_int iNewLevelIndex, class CLevel* pNewLevel);
	_int						Get_CurrentLevelIndex() const;
#pragma endregion
#pragma region  PROTOTYPE_MANAGER
	HRESULT						Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype);

	template<typename T>
	T* Clone_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg = nullptr);
#pragma endregion
#pragma region	OBJECT_MANAGER
	CComponent* Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strComponentTag, _uint iIndex = 0);
	CComponent* Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strPartTag, const _wstring& strComponentTag, _uint iIndex = 0);
	class CLayer* Get_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag);
	class CLayer* Get_CurrentLevel_Layer(const _wstring& strLayerTag);
	HRESULT						Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, _uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg = nullptr);
	HRESULT						Add_GameObject(_uint iLayerLevelIndex, const _wstring& strLayerTag, class CGameObject* pObject);

#ifdef _DEBUG

	// IMGUI에서 전체 레이어 확인
	const map<_wstring, class CLayer*>& Get_Layers(_uint iLevelIndex) const;
#endif
#pragma endregion

#pragma region  PIPELINE
	const _float4x4* Get_Transform(D3DTS eState) const;
	const _float4x4* Get_Transform_Inverse(D3DTS eState) const;
	const _float4* Get_CamPosition() const;
	const _float4* Get_CamLook() const;
	const _float4* Get_CamRight() const;

	void						Set_Transform(D3DTS eState, _fmatrix StateMatrix);
	void						Fill_CameraData(PerCameraData& cameraData);

#pragma endregion
#pragma region  RESOURCE_MANAGER
	HRESULT						Get_VIBuffer(_wstring strVIBufferName, CVIBuffer** pOutBuffer);
	HRESULT						Get_Texture(_wstring strTextureName, CTexture** pOutTexture);
	HRESULT						Get_Material(_wstring strMaterialName, CMaterial** pOutMaterial);
	HRESULT						Get_Shader(_wstring strShaderName, CShader** pOutShader);
	HRESULT						Get_Animation(_wstring strAnimationName, class CAnimation** pOutAnimation);
	HRESULT						Get_Animation(_uint64 iAnimationKey, class CAnimation** pOutAnimation);

	HRESULT						Register_VIBuffer(class CVIBuffer* pVIBuffer);
	HRESULT						Register_Shader(class CShader* pShader);
	HRESULT						Register_Texture(class CTexture* pTexture);

	//Material의 이름을 직접 지정할 수 있음
	HRESULT						Register_Material(class CMaterial* pMaterial, _wstring KeyName = L"");


	//HRESULT					Register_StaticAnimation(class CAnimation* pAnimation);
	//HRESULT					Register_DynamicAnimation(class CAnimation* pAnimation);

	HRESULT						Register_StaticAnimation(class CAnimation* pAnimation, _uint64 iKey = 0);
	HRESULT						Register_DynamicAnimation(class CAnimation* pAnimation, _uint64 iKey = 0);

	HRESULT						Register_TextureFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _wstring& strTextureFilePath);
	HRESULT						Register_MaterialFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _wstring& strMaterialFilePath);

	_bool						Is_MaterialDirty();

	HRESULT						Ready_RawAnimations(const _wstring& strFoldername, const _wstring& strAnimationBaseName, _bool bStaticAnimation, ANIMATION_TIME_UNIT eTimeUnit = ANIMATION_TIME_UNIT::MILLISEC);
#pragma endregion
#pragma region TARGET_MANAGER
	HRESULT						Add_RenderTarget(const _wstring& strTargetTag, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
	void                        Clear_RenderTarget(const _wstring& strTargetTag);

	HRESULT						Add_MRT(const _wstring& strMRTTag, const _wstring& strTargetTag);
	HRESULT						Begin_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV = nullptr, _bool bUseDSV = true, _bool bClear = true);
	HRESULT                     Bind_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV = nullptr);
	HRESULT						End_MRT();

	HRESULT						Bind_RT_ShaderResource(const _wstring& strTargetTag, _uint iRegisterSlot);
	HRESULT						Copy_RT_Resource(const _wstring& strTargetTag, ID3D11Texture2D* pOut);

	ID3D11ShaderResourceView* Get_RenderTargetSRV(const _wstring strTargetTag);
	HRESULT						PopRenderTarget_InMRT(const _wstring& strMRTTag, const _wstring& strTargetTag);
	HRESULT						Remove_RenderTarget(const _wstring& strTargetTag);

#ifdef _DEBUG
public:
	HRESULT						Ready_RT_Debug(const _wstring& strTargetTag, _float fX, _float fY, _float fSizeX, _float fSizeY);
	HRESULT						Render_RT_Debug(const _wstring& strMRTTag, class CShader* pShader, class CVIBuffer_Rect* pVIBuffer, class CConstantBuffer* pObjectBuffer);

#endif
#pragma endregion

#pragma region  RENDERER 
	void						Add_RenderGroup(RENDERID eGroupID, class CGameObject* pGameObject);
	void						Add_Decal(class CDecal* pDecal);
	RENDERID					Get_CurRenderPass();

	void						Submit_Static(StaticCommand& StaticCommand);
	void						Submit_Skinned(SkinnedCommand& SkinnedCommand);
	void						Submit_Effect(EffectCommand& EffectCommand);
	void						Submit_UI(UICommand& UICommand);
	void						Submit_UIText(UITextCommand& Command);

	//Shader 자동 바인딩 

	void						Submit_Static_Resolved(StaticCommand& command, _bool bInstanced = false);
	void						Submit_Skinned_Resolved(SkinnedCommand& command);

	// 추가
	HRESULT						Bind_CameraBuffer();

	// 추가
	HRESULT						Command_Execute();

	// 추가
	HRESULT						Bake_IBL();

	void						Add_FogVolume(const FogVolume& tVolume);

	ID3D11ShaderResourceView* Get_HiZSRV() const;
	_bool						Get_HiZHistory(HIZ_HISTORY_DESC* pOutHistory) const;
	HRESULT						Resize_HiZ(_uint iViewportWidth, _uint iViewportHeight);

	void						Set_ExposureScale(_float fTarget, _float fBlendTime = 0.f);
	void						Update_SceneTime(_float fTimeDelta);
	void						Set_ReflectionPlane(_float fHeight, _bool bEnable);

#ifdef _DEBUG
public:
	const RenderTuning& Get_RenderTuning() const;
	void  Set_RenderTuning(const RenderTuning& t);
	void  Update_RenderTuning_DEBUG();

	const PerIBLData& Get_IBLData() const;
	void  Set_IBLData(const PerIBLData& t);
	void  Update_IBLData_DEBUG();

	const RimProfile& Get_RimProfile(int i) const;
	void  Set_RimProfile(int i, const RimProfile& t);
	void  Update_RimProfiles_DEBUG();


#endif
#pragma endregion
#pragma region MAP_MANAGER
	HRESULT						Load_Map(_wstring strMapName, class CMap** pOutMap, _float fMapScale = 1.f, _bool bCreateEditorRealObjects = false);

	HRESULT						Load_HLODMap(const _wstring& strOriginalMapName, class CMap** pOutMap);

	HRESULT						Load_MapPhysics(const _wstring& strMapName);
	HRESULT						Load_MapPhysicsSettings(const _wstring& strMapName);
	HRESULT						Save_MapPhysicsSettings(const _wstring& strMapName);
	HRESULT						Build_MapPhysics(const _wstring& strMapName);
	// MapManager의 맵 피직스 활성 상태를 외부 레벨/월드 흐름에서 제어한다.
	void						Set_MapPhysicsActive(const _wstring& strMapName, _bool bActive);
	MAP_PHYSICS_GEOMETRY_TYPE	Get_MapPhysicsType(const MapObjectSourceData& Source) const;
	void						Set_MapPhysicsType(const MapObjectSourceData& Source, MAP_PHYSICS_GEOMETRY_TYPE eGeometryType);

	MAP_PHYSICS_ROLE			Get_MapPhysicsRole(const MapObjectSourceData& Source) const;
	void						Set_MapPhysicsRole(const MapObjectSourceData& Source, MAP_PHYSICS_ROLE eRole);

	void						Unload_MapPhysics(const _wstring& strMapName);
	class CMap* Get_Map(_wstring strMapName);
	_wstring					Get_MapFolderPath(const _wstring& strMapName) const;
	_wstring					Get_MapRuntimeRelativePath(const _wstring& strMapName, const _wstring& strFileName) const;
	HRESULT						Read_MapMeshGroup(const _wstring& strMapName, _bool bInstanced, _uint iGroupIndex, MeshPassData* pOutPassData) const;

#ifdef _DEBUG
	_bool						Find_MapObjectSource(const class CMapObject* pMapObject, MapObjectSourceData* pOutSource) const;
	void						Set_MapPhysicsDebugVisible(const _wstring& strMapName, _bool bVisible);
#endif // _DEBUG


#pragma endregion



#pragma region LIGHT_MANAGER
	const LIGHT_DESC* Get_LightDesc(LIGHT eLightType, _uint iIndex);
	_uint						Get_LightCount(LIGHT eLightType) const;
	HRESULT                     Add_Light(LIGHT eLightType, const LIGHT_DESC& LightDesc);
	HRESULT                     Fill_LightData(LIGHT eLightType, _uint iIndex, PerLightData& perLightData);
	_bool						Is_LightEnabled(LIGHT eLightType, _uint iIndex) const;
	HRESULT						Set_LightEnable(LIGHT eLightType, _uint iIndex, _bool bEnable);

	HRESULT						Remove_Light(LIGHT eLightType, _uint iIndex);

	HRESULT						Set_LightDesc(LIGHT eLightType, _uint iIndex, const LIGHT_DESC& LightDesc);

#pragma endregion
#pragma region POOL_MANAGER
	template<typename T, size_t Size>
	HRESULT Create_Pool()
	{
		return m_pPool_Manager->Create_Pool<T, Size>();
	}
	//필요하면 Activate에 Desc매개변수 전달 
	template<typename T>
	T* Get_PoolObject(void* pArg = nullptr)
	{
		return m_pPool_Manager->Get_Object<T>(pArg);
	}

	template<typename T>
	void Remove_Pool()
	{
		m_pPool_Manager->Remove_Pool<T>();
	}

	void Remove_AllPools()
	{
		m_pPool_Manager->Remove_AllPools();
	}

#pragma endregion
#pragma region PHYSIC_MANAGER
	PxScene* Get_CurrentPxScene();
	PxCpuDispatcher* Get_Default_CpuDispatcher();
	PxTolerancesScale& Get_Default_TolerancesScale();
	PxMaterial* Get_Default_Material();

	void						Bind_TriggerCallback(function<void(PxTriggerPair*, PxU32)> func);
	void						Bind_ContactCallback(function<void(const PxContactPairHeader&, const PxContactPair*, PxU32)> func);
	class PxQueryFilterCallback* Get_CCTQueryFilter();
	class PxControllerBehaviorCallback* Get_CCTBehaviorCallback();
	bool						Cook_TriangleMesh(PxTriangleMeshDesc& meshDesc, PxDefaultMemoryOutputStream& OutWriteBuffer);
	void						Reset_Filtering(PxActor* pActor);

	PxScene* Create_PxScene(const PxSceneDesc& desc);
	PxShape* Create_PxShape(const PxGeometry& geometry, PxMaterial* material, bool isExclusive = false);
	PxMaterial* Create_PxMaterial(float staticFriction, float dynamicFriction, float restitution);
	PxRigidDynamic* Create_RigidDynamic(_fvector vInitPos);
	PxTriangleMesh* Create_TriangleMesh(PxInputStream& stream);
	PxRigidStatic* Create_RigidStatic(_fvector vInitPos, _fvector vInitQuat);
	PxController* Create_CapsuleController(PxCapsuleControllerDesc& desc);
	PxController* Create_BoxController(PxBoxControllerDesc& desc);

	_bool						Sweep_Sphere(_float fRadius, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
	_bool						Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
	_bool						Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _vector vQuat, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);

	_bool						Overlap_Sphere(_float fRadius, _fvector vOrigin, FILTER_GROUP eTargetFilter, vector<CGameObject*>& OutResults, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);

	void						Add_ActorToScene(PxActor* pActor);
	void						Remove_ActorFromScene(PxActor* pActor);
	_bool						Raycast(const _float3& vRayOrigin, const _float3& vRayDirection, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);

	//모든 컨트롤러(CCT) 캐시 초기화
	void						Invalidate_AllControllerCache();
#ifdef _DEBUG
	const PxRenderBuffer& Get_RenderBuffer() const;
	HRESULT                     Create_VisualDebugger(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void                        Update_VisualDebugger();

#endif

#pragma endregion
#pragma region CAMERA_MANAGER
	void						Set_FollowTarget(class CSpringArm* pArm);
	CAMERA_MODE					Get_CameraMode();
	_bool						Try_Carmera_LockOn(_bool bLockOn);

	/* 가상 컷신 카메라 연결 */
	HRESULT						Begin_CutSceneCamera(class CCamera* pCutSceneCamera, _float fBlendIn);
	HRESULT						Transition_CutSceneCamera(_float fBlendDuration);
	void						End_CutSceneCamera(_float fBlendOut);
	_bool						Is_CutSceneCameraPlaying() const;
	void						Mark_CameraDiscontinuity();
	_uint						Get_CameraDiscontinuityRevision() const;
	_float						Get_CameraFovy() const;

	HRESULT						Load_CameraShakeDatas(const string& strFolderpath);
	HRESULT						Load_CameraSceneDatas(const string& strFolderpath);
	HRESULT						Load_CameraWorkDatas(const string& strFolderpath);

	HRESULT						Play_Shake(const string& strKey);
	void						Stop_Shake(const string& strKey);

	HRESULT						Play_Shake(_uint64 iKeyHash);
	void						Stop_Shake(_uint64 iKeyHash);
	void						Stop_AllShakes();

	HRESULT						Play_Scene(const string& strKey, _float fBlendIn, _float fBlendOut, _float fPlayRate);
	void						Stop_Scene();

	//고정 위치(PCDistance용) 
	HRESULT						Play_CameraWork(const string& strKey, _fvector vTargetWorldPos);
	//타켓위치기준 
	HRESULT						Play_CameraWork(const string& strKey, CGameObject* pTarget);

	void						Stop_CameraWork();
	void						Set_CameraWorkTarget(_fvector vTargetWorldPGetos);

	void						Set_CameraLock(_bool bLock);
	_bool						Is_ScenePlaying() const;
	void						Clear_Camera();
#ifdef _DEBUG
	void						Set_CameraPosition(_fvector vPosition);
#endif // _DEBUG

	_bool						Is_CameraLockOn();
	//_float3						Get_CameraLockTarget();
	void						Set_Camera_LockPos(_vector vPos);

	// UI 프리뷰 전용
	void						Begin_UIPreviewCamera(_fvector vEye, _fvector vAt, _float fFovy);
	void						End_UIPreviewCamera();
	_bool						Is_UIPreviewCameraActive() const;
#pragma endregion

#pragma region IMGUI
#ifdef _DEBUG
	void						Imgui_BeginFrame();
	void						Imgui_Render();
	LRESULT						Imgui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ImGuiContext* Get_ImguiContext();


#endif 
#pragma region SHADER
	class CShader_Resolver* Get_ShaderResolver();
#pragma endregion

#pragma region EVENT
	template<typename EVENT>
	void                        Subscribe(EVENT _event, IListener* _listener)
	{
		m_pEvent_Manager->Subscribe(_event, _listener);
	}

	template<typename EVENT>
	void                        UnSubscribe(EVENT _event, IListener* _listener)
	{
		m_pEvent_Manager->UnSubscribe(_event, _listener);
	}

	template<typename EVENT>
	void                        Broadcast(EVENT _event, Event_Desc* _pData)
	{
		m_pEvent_Manager->Broadcast(_event, _pData);
	}

	void                        UnSubscribe_AllEvent(IListener* _listener);
	void                        Clear_All();
#pragma endregion

#pragma region EFFECT_MANAGER
	void						Register_Effect(const string& strName, const struct EffectDesc& tDesc);
	HRESULT						Load_Effect_File(const string& strName, const string& strPath);
	const EMITTER_SHADER_BUNDLE* Get_EffectShaderBundle() const;

	class CEffect* Play_Effect(const string& strName, const _float3& vWorldPos);
	class CEffect* Play_Effect(const string& strName, const _float4x4* pFollowBoneMat,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	class CEffect* Play_Effect(const string& strName, CModel* pModel, const _char* pBoneName,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	class CEffect* Play_Effect(const string& strName, CModel* pModel, const _char* pBoneName, const _char* pBoneName2,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	class CEffect* Play_Effect_Model(const string& strName, CModel* pModel,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	class CEffect* Play_Effect_Parts(const string& strName, const vector<CModel*>& Parts,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	class CEffect* Play_Effect(const string& strName, const _float4x4* pFollowBoneMat, const _float4x4* pFollowBoneMat2,
		const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	void						Clear_Effects();
#pragma endregion

#pragma region UI_MANAGER
	HRESULT						Add_UIObject(const _wstring& strUITag, class CUIObject* pUIObject);
	HRESULT						Add_UIController(const _wstring& strControllerTag, class CUIControllerBase* pController);
	HRESULT						Spawn_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);

	CUIObject* Find_UIObject(const UI_HANDLE& Handle);
	CUIObject* Find_UIObject(const _wstring& strUITag);
	CUIControllerBase* Find_UIController(const _wstring& strControllerTag);
	HRESULT						Set_UIImageTexture(const UI_HANDLE& Handle, const _wstring& strRuntimeElementTag, const string& strTexturePath);
	void						Submit_UIRenderCommand();

	HRESULT						Register_UIController(const UI_HANDLE& Handle, const _wstring& strControllerTag, CUIControllerBase* pController);
	HRESULT						Register_UIControllerFactory(const string& strControllerType, const UI_CONTROLLER_FACTORY& Factory);
	HRESULT						Register_UIAssetDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount = nullptr, _wstring* pError = nullptr);
	HRESULT						Register_UIFont(const string& strFontAssetID, const _wstring& strFontPath);

	HRESULT						Create_UIController(const UI_CONTROLLER_CREATE_CONTEXT& Context);
	HRESULT						Create_UIObject(const string& strAssetID, const _wstring& strInstanceTag);

	HRESULT						Open_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);
	HRESULT						Open_UIObject(const UI_HANDLE& Handle);
	HRESULT						Close_UIObject(const UI_HANDLE& Handle);

	void						Request_RemoveUIController(const _wstring& strControllerTag);
	void						Request_ReleaseUIObject(const UI_HANDLE& Handle);
	void						Request_ReleaseUILifetime(UI_LIFETIME eLifetime);
	HRESULT						Request_SpawnUIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);
	HRESULT						Request_PushUIInputOwner(const UI_HANDLE& Handle, UI_INPUT_OWNER_KIND eKind);
	HRESULT						Request_PopUIInputOwner(const UI_HANDLE& Handle);

	_bool						Is_AliveUIObject(const UI_HANDLE& Handle) const;
	_bool						Is_OpenUIObject(const UI_HANDLE& Handle) const;
	_bool						Is_PendingSpawnUIObject(const UI_HANDLE& Handle) const;
	_bool						Contains_UIFont(const string& strFontAssetID) const;
	_bool						Has_UIInputOwner() const;
	_bool						Is_TopUIInputOwner(const UI_HANDLE& Handle) const;
	_bool						Can_PushUI(UI_INPUT_OWNER_KIND eKind) const;

	HRESULT						Register_UIAsset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation = nullptr, _wstring* pError = nullptr);

	const UI_ASSET_DESC* Find_UIAsset(const string& strAssetID) const;
	_bool						Contains_UIAsset(const string& strAssetID) const;
	size_t						Get_UIAssetCount() const;

	void						Acquire_UILayerSuppression(const string& strLayer);
	void						Release_UILayerSuppression(const string& strLayer);

#pragma endregion


#pragma region NAVIGATION_MANAGER
	HRESULT						Load_Navigation(const _wstring& strChapterKey, _uint iSectorID, const _wstring& strNavigationPath, const _wstring& strNeighborPath);
	class CNavigation* Get_Navigation(const _wstring& strChapterKey, _uint iSectorID) const;
	void						Clear_Navigations(const _wstring& strChapterKey);
#pragma endregion
#pragma region SOUND_MANAGER
	void                        Ready_Sound();
	void                        Update_Sound(_float fTimeDelta);

	void                        Play_Sound(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
	void                        Play_SFXSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
	void                        Play_BGM(const string& strSoundKey, _float fVolume = 1.0f);
	void                        Play_BGM(const string& strSoundKey, _float fVolume, _float fFadeSec);
	void                        Play_BGM_Sequence(const string& strIntroKey, const string& strLoopKey, _float fVolume);
	void                        Play_MonsterSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
	void                        Play_PlayerSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false, _float fPitch = 1.f);
	void                        Play_WeaponSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
	void                        Play_Ambience(const string& strSoundKey, _float fVolume, _float fFadeSec);
	void                        Stop_Ambience();

	_uint						Play_SoundAt(const string& strSoundKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolume = 1.f, _bool bLoop = false);
	_uint						Play_SoundFollow(const string& strSoundKey, CHANNEL_ID eGroup, const _float4x4* pFollowMat, _float fVolume = 1.f, _bool bLoop = false, _bool bDoppler = true);
	void						Update_SoundSourcePos(_uint hHandle, const _float3& vWorldPos);
	void						Stop_SoundSource(_uint hHandle);
	_bool						Is_SoundSourcePlaying(_uint hHandle);

	void                        SetVolume_ByKey(const string& strSoundKey, _float fVolume);
	void                        Set_GroupVolume(CHANNEL_ID eID, _float fVolume);
	void                        Set_GroupMute(CHANNEL_ID eGroup, _bool bMute);
	void                        Set_MasterMute(_bool bMute);

	_bool                       IsPlaying_ByKey(const string& strSoundKey);

	void						FadeOut_BGM(_float fFadeSec);
	void                        StopSound_ByKey(const string& strSoundKey);
	void                        Stop_GroupSound(CHANNEL_ID eID);
	void                        Stop_AllSound();
	void                        Stop_BGM();

	void                        Pause_Group(CHANNEL_ID eGroup, _bool bPause);
	void                        Pause_All(_bool bPause);

	// 테스트용
	_bool						Can_PlaySoundByInterval(const string& strRuleKey, _float fIntervalSec);
	void						Play_SoundSequence(const CSound_Manager::SOUND_SEQUENCE_DESC& Desc);
	void						Stop_SoundSequence(const string& strSequenceKey);

	_uint						Play_Cue(const string& strCueKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolumeScale = 1.f, _bool bLoop = false);
	_bool						Has_Cue(const string& strCueKey) const;
#pragma endregion

#pragma region GPUANIMATION_SYSTEM
	HRESULT						Allocate_FinalBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset);
	HRESULT						Free_FinalBoneMatrixRange(_uint iOffset, _uint iCount);

	HRESULT						Allocate_CombinedBoneMatrixRange(_uint iBoneMatrixCount, _uint* pOutOffset);
	HRESULT						Free_CombinedBoneMatrixRange(_uint iOffset, _uint iCount);

	ID3D11ShaderResourceView* Get_GlobalFinalBoneMatrixSRV() const;
	_uint						Get_GlobalFinalBoneMatrixCapacity() const;

	HRESULT						Update_GlobalCombinedBoneMatrixRange(_uint iOffset, const _float4x4* pCombinedMatrices, _uint iCount);

	HRESULT						Register_MeshBoneInputRange(const vector<_uint>& BoneIndices, const vector<_float4x4>& OffsetMatrices, _uint* pOutOffset, _uint* pOutCount);
	HRESULT						Free_MeshBoneInputRange(_uint iOffset, _uint iCount);

	HRESULT						Register_ModelSkinningJob(class CModel* pModel);
	HRESULT						Flush_SkinningJobs();
#pragma endregion

#pragma region THREAD_POOL
	class CThreadPool* Get_ThreadPool();

private:
	class CGraphic_Device* m_pGraphic_Device = { nullptr };
	class CInput_Device* m_pInput_Device = { nullptr };
	class CTimer_Manager* m_pTimer_Manager = { nullptr };
	class CLevel_Manager* m_pLevel_Manager = { nullptr };
	CPrototype_Manager* m_pPrototype_Manager = { nullptr };
	class CObject_Manager* m_pObject_Manager = { nullptr };
	class CPipeLine* m_pPipeLine = { nullptr };
	class CResource_Manager* m_pResource_Manager = { nullptr };
	class CTarget_Manager* m_pTarget_Manager = { nullptr };
	class CRenderer* m_pRenderer = { nullptr };
	class CMap_Manager* m_pMap_Manager = { nullptr };
	class CLight_Manager* m_pLight_Manager = { nullptr };
	class CPool_Manager* m_pPool_Manager = { nullptr };
	class CPhysicManager* m_pPhysic_Manager = { nullptr };
	class CEvent_Manager* m_pEvent_Manager = { nullptr };
	class CShader_Resolver* m_pShader_Resolver = { nullptr };
	class CCamera_Manager* m_pCamera_Manager = { nullptr };
	class CEffect_Manager* m_pEffect_Manager = { nullptr };
	class CUI_Manager* m_pUI_Manager = { nullptr };
	class CNavigation_Manager* m_pNavigation_Manager = { nullptr };
	class CSound_Manager* m_pSound_Manager = { nullptr };
	class CGPUAnimationSystem* m_pGPUAnimationSystem = { nullptr };
	class CThreadPool* m_pThreadPool = { nullptr };

#ifdef _DEBUG
	class CImguiManager* m_pImguiManager = { nullptr };
#endif

private:
	_float2						m_vViewportDesc{};
	class CCharacter* m_pPlayer = { nullptr };

	//리소스 경로. 런처 / Editor에서 설정하기 
	_wstring 					m_strResourcePath = {};
	string						m_strResourcePathA = {};
	_wstring					m_strTexturePath = {};

#ifdef _DEBUG
	_bool						m_bDebugFlag = {};
	_uint						m_iPhysXDebugMask = { ETOUI(PHYSIC_DEBUG_MODE::ALL) };
#endif
public:
	void						Release_Engine();

public:
	//로딩 중 종료 대비
	void Request_LoadingCancel()
	{
		m_bLoadingCancel.store(true, memory_order_relaxed);
	}
	_bool Is_LoadingCancled() const
	{
		return m_bLoadingCancel.load(memory_order_relaxed);
	}

private:
	atomic<_bool> m_bLoadingCancel = { false };

};

template<typename T>
inline T* CGameInstance::Clone_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
	return m_pPrototype_Manager->Clone_Prototype<T>(iLevelIndex, strPrototypeTag, pArg);
}


NS_END
