#pragma once
#include "Base.h"

NS_BEGIN(Engine)
class CSpringArm;
class CCamera;
class CCamera_Manager : public CBase
{
private:
	struct CAM_POSE
	{
		_float4	vPosition	= { 0.f, 0.f, 0.f, 1.f };
		_float4	vQuat		= { 0.f, 0.f, 0.f, 1.f };
		_float	fFovy		= 0.f;
	};

	struct CAM_TRANSITION
	{
		_bool	bActive = false;
		_float	fDuration = 0.f;
		_float	fElapsed = 0.f;
		_float4	vStartPos = { 0.f, 0.f, 0.f, 1.f };
		_float4	vStartQuat = { 0.f, 0.f, 0.f, 1.f };
		_float	fStartFovy = 0.f;
	};

	struct SHAKE_INSTANCE
	{
		const SHAKE_DESC*	pDesc = { nullptr };
		//정지요청 -> 블랜드아웃중 체크 
		_bool				bStopping = { false };
		_float				fStopTime = 0.f;
		_float				fTime = 0.f;
		_float				Phases[7] = {};		// Pitch,Yaw,Roll,X,Y,Z,FOV 초기 위상
	};

private:
	CCamera_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CCamera_Manager() = default;

public:
	void					Set_FollowTarget(CSpringArm* pArm) { m_pTargetArm = pArm; }
	//void PlayAction 

	CAMERA_MODE				Get_CameraMode() { return m_CameraMode; }

	// 카메라 컷신용
	HRESULT					Begin_CutSceneCamera(CCamera* pCutSceneCamera, _float fBlendIn);
	HRESULT					Transition_CutSceneCamera(_float fBlendDuration);
	void					End_CutSceneCamera(_float fBlendOut);
	_bool					Is_CutSceneCameraPlaying() const { return CAMERA_MODE::CUTSCENE == m_CameraMode && nullptr != m_pCutSceneCameraSource; }
	void					Mark_CameraDiscontinuity();
	_uint					Get_CameraDiscontinuityRevision() const { return m_iCameraDiscontinuityRevision; }
	_float					Get_MainCameraFovy() const;

#ifdef _DEBUG
	void					Set_CameraPosition(_fvector vPosition); // 에디터 용 
#endif
public:
	HRESULT					Initialize();
	void					Update(_float fTimeDelta);
	void					Late_Update(_float fTimeDelta);

	void					Clear();
public:
	/* Shake */
	HRESULT					Load_ShakeDatas(const string& strFolderpath);
	const SHAKE_DESC*		Get_ShakeDesc(const string& strKey) const;
	HRESULT					Play_Shake(const string& strKey);
	HRESULT					Play_Shake(_uint64 iKeyHash);
	void					Stop_Shake(const string& strKey);
	void					Stop_Shake(_uint64 iKeyHash);
	void					Stop_AllShakes() { m_PlayingShakes.clear(); }


public:
	/* Scene*/
	HRESULT					Load_SceneDatas(const string& strFolderpath);
	const SCENE_DESC*		Get_SceneDesc(const string& strKey) const;
	HRESULT					Play_Scene(const string& strKey,_float fBlendIn, _float fBlendOut, _float fPlayRate);
	void					Stop_Scene();
	_bool					Is_ScenePlaying() const { return m_pPlayingScene != nullptr; }

	void					Capture_Pose();

public:
	/* Camera Work (AlignCurve) : 타겟을 기준으로 프레이밍한 카메라 액션 */
	HRESULT					Load_CameraWorkDatas(const string& strFolderpath);
	//고정 위치
	HRESULT					Play_CameraWork(const string& strKey, _fvector vTargetWorldPos);
	//타겟 오브젝트 기준
	HRESULT					Play_CameraWork(const string& strKey, CGameObject* pTarget);

	void					Set_CameraWorkTarget(_fvector vTargetWorldPos);	
	_bool					Is_CameraWorkPlaying() const { return m_pPlayingWork != nullptr; }
	void					Stop_CameraWork();

public:
	/* LockOn관련 */
	_bool					Is_LockOn() const { return m_bLockOn; }
	_bool					Try_LockOn(_bool bLockOn);
	void					Set_LockOnTarget(_vector vTargetPose);
	void					Set_CameraLock(_bool bLock);

public:
	/* UI 프리뷰 관련*/
	// UI 전용 3D 프리뷰 카메라를 활성화
	void					Begin_UIPreviewCamera(_fvector vEye, _fvector vAt, _float fFovy);
	// UI 프리뷰 카메라를 종료하고 기존 게임 카메라 계산으로 복귀
	void					End_UIPreviewCamera();
	// UI 프리뷰 카메라가 현재 활성 상태인지 반환
	_bool					Is_UIPreviewCameraActive() const { return m_bUIPreviewCamActive; }

private:
	CAM_POSE				Compute_DesiredPose(_float fTimeDelta);
	//Align Curve 방식 
	void					Advance_CameraWork(_float fTimeDelta, CAM_POSE& OutPose);
	//씬 + Work가산 
	void					Apply_SceneAdditive(_float fTimeDelta, CAM_POSE& InOutPose);

	void					Release_WorkTarget();

	CAM_POSE				Apply_Transition(const CAM_POSE& Target, _float fTimeDelta);
	CAM_POSE				Apply_Shake(const CAM_POSE& Base, _float fTimeDelta);

	void					Begin_Transition(_float fDuration);
	void					End_Scene();

private:
	void					Begin_CameraLock();
	void					End_Action(); 
	void					Set_CameraMode(CAMERA_MODE eMode);

#ifdef _DEBUG
	void					Free_Move(_float fTimeDelta);
#endif

private:
	ID3D11Device*			m_pDevice				= { nullptr };
	ID3D11DeviceContext*	m_pContext				= { nullptr };
	class CGameInstance*	m_pGameInstance			= { nullptr };

private:
	const _float			m_OriginFOV				= { 75.f };

	/* Scene */
	unordered_map<string, SCENE_DESC> m_SceneDescs;
	const SCENE_DESC*		m_pPlayingScene			= { nullptr };
	_bool					m_bPlayingShakeScene		= { false };
	_float					m_fScenePlayTime		= { 0.f };
	_float					m_fScenePlayRate		= { 1.f };

	_float4					m_vFreezePos			= { 0.f,0.f, 0.f, 1.f };// 시작 위치 고정용 
	_float4					m_vFreezeQuat			= { 0.f,0.f, 0.f, 1.f }; 
	_float					m_fScenePosScale		= { 0.22f };
	/* FOV Blend */
	_float					m_fSavedFovy			= {};

	/* Shake */
	unordered_map<_uint64, const SHAKE_DESC*>	m_ShakeByHash;
	unordered_map<string, SHAKE_DESC> m_ShakeDescs;
	vector<SHAKE_INSTANCE>	m_PlayingShakes;
	_uint					m_iNextShakeId			= { 1 };

	/* Camera Work */
	unordered_map<string, CAMWORK_DESC> m_CamWorkDescs;
	const CAMWORK_DESC*		m_pPlayingWork			= { nullptr };

	CGameObject*			m_pWorkTargetObject		= { nullptr };
	_float					m_fWorkTime				= { 0.f };
	_float4					m_vWorkLocation			= { 0.f, 0.f, 0.f, 1.f };
	_float3					m_vWorkTargetPos		= { 0.f, 0.f, 0.f };
	//Side 방향 . + 오른쪽 / - 왼쪽
	_float					m_fWorkSideDirSign		= { 1.f };

	/* CamWork Envelope 튜닝/Offset 데이터 */
	_float					m_fWorkOffsetScale		= { 1.f };
	_float					m_fWorkFovScale			= { 10.f };	
	//대상 피벗을 얼마나 높게 잡을지
	_float					m_fWorkPivotHeight		= { 1.2f };
	//WorldFixed 일떄 고정 위치 
	_float4					m_vWorkParkedPos		= { 0.f, 0.f, 0.f, 1.f };
	_float4					m_vWorkParkedOffset		= {};
	/* Scene 가산모드 데이터 */
	_float					m_fSceneBlendIn			= { 0.1f };
	_float					m_fSceneBlendOut		= { 0.3f };

	/* Cam Data */
	CAM_POSE				m_BasePose;
	CAM_TRANSITION			m_Transition;

	_bool					m_bActionEnding			= {};
	_float					m_fBlendDuration		= { 0.3f };

	static constexpr _float m_fAspect				= { 1280.f / 720.f };

private:
	CAMERA_MODE				m_CameraMode			= {};
	CSpringArm*				m_pTargetArm			= {};
	CCamera*				m_pMainCamera			= {};
	class CTransform*		m_pCamTransform			= {};


	// 카메라 컷신용
	CCamera*				m_pCutSceneCameraSource = { nullptr };
	CAM_POSE				m_PreCutScenePose		= {};
	CAMERA_MODE				m_eCameraModeBeforeCutScene = { CAMERA_MODE::FOLLOW };
	_bool					m_bLockOnBeforeCutScene = { false };
	_uint					m_iCameraDiscontinuityRevision = {};

private:
	/* UI 프리뷰 관련 */
	_bool					m_bUIPreviewCamActive	= { false };
	_float4					m_vUIPreviewEye			= { 0.f, 0.f, -5.f, 1.f };
	_float4					m_vUIPreviewAt			= { 0.f, 1.f, 0.f, 1.f };
	_float					m_fUIPreviewFovy		= { XMConvertToRadians(22.f) };

#ifdef _DEBUG
	_float					m_fDebugMouseSensor		= {0.08f};
	_float					m_fMoveSpeed			= {0.5f};

#endif 
	_bool					m_bLockOn				= { false };
	_float3					m_vLockOnTargetPos		= { 0.f, 0.f, 0.f };

	_bool					m_bCameraLocked			= {};
public:
	static CCamera_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void			Free() override;

};
NS_END
