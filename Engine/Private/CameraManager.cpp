#include "CameraManager.h"
#include "SpringArm.h"
#include "Camera_Main.h"
#include "GameInstance.h"
#include "Transform.h"

#include "Camera_Func.h"

CCamera_Manager::CCamera_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:m_pDevice(pDevice), m_pContext(pContext), m_pGameInstance{ CGameInstance::Get_Instance()}
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CCamera_Manager::Initialize()
{
	CCamera::CAMERA_DESC CameraDesc;
	CameraDesc.vEye = _float3(0.f, 0.f, 0.f);
	CameraDesc.vAt = _float3(0.f, 0.f, 1.f);
	//원작 FOV는 가로라서 세로FOV로 변경해야함 
	CameraDesc.fFovy = 2.f * atanf(tanf(XMConvertToRadians(m_OriginFOV)* 0.5f) / m_fAspect);
	//CameraDesc.fFovy = XMConvertToRadians(m_FOV);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;
#ifdef _DEBUG
	CameraDesc.fSpeedPerSec = 3.f;
	CameraDesc.fRotationPerSec = XMConvertToRadians(90.f);
#endif

	m_pMainCamera = CCamera_Main::Create(m_pDevice, m_pContext, &CameraDesc);
	if (m_pMainCamera == nullptr) return E_FAIL;

	m_pCamTransform = m_pMainCamera->Get_Transform();
	if (m_pCamTransform == nullptr) return E_FAIL;

	XMStoreFloat4(&m_BasePose.vPosition, m_pCamTransform->Get_State(STATE::POSITION));
	XMStoreFloat4(&m_BasePose.vQuat, m_pCamTransform->Get_Quat());
	m_BasePose.fFovy = CameraDesc.fFovy;
	m_fSavedFovy = CameraDesc.fFovy;


	m_CameraMode = CAMERA_MODE::FOLLOW;
	return S_OK;
}

void CCamera_Manager::Update(_float fTimeDelta)
{
	if (m_pTargetArm)
	{
		m_pTargetArm->Set_LockOn(m_bLockOn, m_vLockOnTargetPos);
	}


#ifdef _DEBUG
	if (m_pGameInstance->Key_Down(DIK_1))
	{
		Set_CameraMode(CAMERA_MODE::FOLLOW);
	}
	else if (m_pGameInstance->Key_Down(DIK_2))
	{
		Set_CameraMode(CAMERA_MODE::FREE);
	}
#endif
}

void CCamera_Manager::Late_Update(_float fTimeDelta)
{
	// UI 프리뷰 전용
	if (m_bUIPreviewCamActive)
	{
		m_pCamTransform->Set_State(STATE::POSITION, XMLoadFloat4(&m_vUIPreviewEye));
		m_pCamTransform->LookAt(XMLoadFloat4(&m_vUIPreviewAt));

		m_pMainCamera->Set_Fovy(m_fUIPreviewFovy);

		m_pMainCamera->Late_Update(fTimeDelta);
		return;
	}

#ifdef _DEBUG
	if (m_CameraMode == CAMERA_MODE::FREE)
	{
		Free_Move(fTimeDelta);
		m_pMainCamera->Late_Update(fTimeDelta);
		return;
	}
#endif

	// 모드 별 목표 포즈 계산
	CAM_POSE TargetPose = Compute_DesiredPose(fTimeDelta);

	//전환중이면 블랜드
	if(m_Transition.bActive)
		TargetPose = Apply_Transition(TargetPose, fTimeDelta);

	m_BasePose = TargetPose;
	//BasePose에 일시적 가산
	CAM_POSE Final = Apply_Shake(TargetPose, fTimeDelta);

	m_pCamTransform->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat4(&Final.vPosition),1.f));
	m_pCamTransform->Set_Quat(XMLoadFloat4(&Final.vQuat));
	m_pMainCamera->Set_Fovy(Final.fFovy);

	m_pMainCamera->Late_Update(fTimeDelta);
}

void CCamera_Manager::Clear()
{
	/* Clear Work */
	m_pPlayingWork = nullptr;
	Release_WorkTarget();
	m_fWorkTime = 0.f;
	m_fWorkSideDirSign = 1.f;

	m_vWorkLocation = _float4(0.f, 0.f, 0.f, 1.f);
	m_vWorkParkedPos = _float4(0.f, 0.f, 0.f, 1.f);
	m_vWorkTargetPos = _float3(0.f, 0.f, 0.f);

	/* Clear Scene */
	m_pPlayingScene = nullptr;
	m_fScenePlayTime = 0.f;
	m_fScenePlayRate = 1.f;
	m_vFreezePos = _float4(0.f, 0.f, 0.f, 1.f);
	m_vFreezeQuat = _float4(0.f, 0.f, 0.f, 1.f);

	m_PlayingShakes.clear();

	m_CameraMode = CAMERA_MODE::FOLLOW;
	m_Transition = CAM_TRANSITION{};
	m_bActionEnding = false;
	m_bCameraLocked = false;
	m_fBlendDuration = 0.3f;

	m_bLockOn = false;
	m_vLockOnTargetPos = _float3(0.f, 0.f, 0.f);

	m_fSavedFovy = 2.f * atanf(tanf(XMConvertToRadians(m_OriginFOV) * 0.5f) / m_fAspect);
	m_BasePose.fFovy = m_fSavedFovy;

	if (m_pMainCamera)
		m_pMainCamera->Set_Fovy(m_fSavedFovy);
}


CCamera_Manager::CAM_POSE CCamera_Manager::Compute_DesiredPose(_float fTimeDelta)
{
	CAM_POSE Pose = m_BasePose;

	/* FOLLOW MODE */
	if (m_CameraMode == CAMERA_MODE::FOLLOW)
	{
		if(m_pTargetArm)
		{
			CameraPose TargetPose = m_pTargetArm->Get_FinalCameraPose();
			Pose.vPosition = TargetPose.vPosition;
			Pose.vQuat = TargetPose.vQuat;
		}
		Pose.fFovy = m_fSavedFovy;
	}

	/* CUTSCENE MODE */
	else if (CAMERA_MODE::CUTSCENE == m_CameraMode)
	{
		CTransform* pCutSceneTransform = nullptr == m_pCutSceneCameraSource ? nullptr : m_pCutSceneCameraSource->Get_Transform();

		if (nullptr != pCutSceneTransform)
		{
			// 가상 컷신 카메라의 절대 위치, Quaternion, FOV를 최우선으로 사용한다.
			XMStoreFloat4(&Pose.vPosition, XMVectorSetW(pCutSceneTransform->Get_State(STATE::POSITION), 1.f));
			XMStoreFloat4(&Pose.vQuat, XMQuaternionNormalize(pCutSceneTransform->Get_Quat()));
			Pose.fFovy = m_pCutSceneCameraSource->Get_Fovy();
		}
	}

	/* ACTION MODE */
	else if (m_pPlayingWork)
	{
		Advance_CameraWork(fTimeDelta, Pose);
	}
	else if (m_bActionEnding)
	{
		if (m_pTargetArm)
		{
			CameraPose TargetPose = m_pTargetArm->Get_FinalCameraPose();
			Pose.vPosition = TargetPose.vPosition;
			Pose.vQuat = TargetPose.vQuat;
		}
		Pose.fFovy = m_fSavedFovy;
	}
	else
	{
		// 카메라 락 홀딩 
		Pose.vPosition = m_vFreezePos;
		Pose.vQuat = m_vFreezeQuat;
	}

	// 일반 Camera Scene은 절대 컷신 Pose에 중복 적용하지 않고, Shake가 요청한 Scene만 가산한다.
	if (m_pPlayingScene && (CAMERA_MODE::CUTSCENE != m_CameraMode || m_bPlayingShakeScene))
	{
		Apply_SceneAdditive(fTimeDelta, Pose);
	}


	return Pose;

}

/* 전환 */
CCamera_Manager::CAM_POSE CCamera_Manager::Apply_Transition(const CAM_POSE& Target, _float fTimeDelta)
{
	m_Transition.fElapsed += fTimeDelta;
	_float fBlendTime = (m_Transition.fDuration > 0.f) ? m_Transition.fElapsed / m_Transition.fDuration : 1.f;
	if(fBlendTime >= 1.f)
	{
		m_Transition.bActive = false;
		if (m_bActionEnding) End_Action();
		return Target;
	}

	_float fSmoothStep = fBlendTime * fBlendTime * (3.f - 2.f * fBlendTime);
	CAM_POSE OutPose;

	//위치 보간
	_vector vLerpPos = XMVectorLerp(XMLoadFloat4(&m_Transition.vStartPos), XMLoadFloat4(&Target.vPosition), fSmoothStep);
	XMStoreFloat4(&OutPose.vPosition, XMVectorSetW(vLerpPos, 1.f));
	
	//회전 보간
	_vector vLerpQuat = XMQuaternionSlerp(XMLoadFloat4(&m_Transition.vStartQuat), XMLoadFloat4(&Target.vQuat), fSmoothStep);
	XMStoreFloat4(&OutPose.vQuat, vLerpQuat);

	//FOV 보간 
	OutPose.fFovy = fLerp(m_Transition.fStartFovy, Target.fFovy, fSmoothStep);

	return OutPose;
}

//모든 인스턴스 카메라 Shake 가산 
CCamera_Manager::CAM_POSE CCamera_Manager::Apply_Shake(const CAM_POSE& Base, _float fTimeDelta)
{
	if (m_PlayingShakes.empty()) return Base;

	_float fPitch = 0.f, fYaw = 0.f, fRoll = 0.f;
	_float fLocX = 0.f, fLocY = 0.f, fLocZ = 0.f;
	_float fFov = 0.f;

	for (size_t i = 0; i < m_PlayingShakes.size();)
	{
		SHAKE_INSTANCE& Inst = m_PlayingShakes[i];
		const SHAKE_DESC& Desc = *Inst.pDesc;

		Inst.fTime += fTimeDelta;

		const _float fOut = Desc.fOscillationBlendOutTime;

		//유한 셰이크 
		if (Desc.fOscillationDuration > 0.f && Inst.fTime >= Desc.fOscillationDuration)
		{
			//지우려는 원소자리에 마지막원소를 덮어씌움 
			Inst = m_PlayingShakes.back();
			m_PlayingShakes.pop_back();
			continue;
		}

		//정지 요청됐는데 블랜드 아웃이 끝났으면 
		if (Inst.bStopping && (fOut <= 0.f || Inst.fTime - Inst.fStopTime >= fOut))
		{
			Inst = m_PlayingShakes.back();
			m_PlayingShakes.pop_back();
			continue;
		}

		_float fWeight = Shake_BlendWeight(Desc, Inst.fTime);

		//정지중이면 페이드아웃 
		if (Inst.bStopping && fOut > 0.f)
		{
			_float fStopWeight = 1.f - (Inst.fTime - Inst.fStopTime) / fOut;
			if (fStopWeight < fWeight) fWeight = fStopWeight;
		}

		auto Oscillate = [&](const FOscillatorDesc& Osc, _int iAxis) -> _float
			{
				if (Osc.fAmplitude == 0.f || Osc.fFrequency == 0.f) return 0.f;
				//언리얼 마티니 Shake 오실레이터
				return Osc.fAmplitude * fWeight * sinf(Inst.Phases[iAxis] + Osc.fFrequency * Inst.fTime);
			};

		fPitch += Oscillate(Desc.RotPitch, 0);
		fYaw += Oscillate(Desc.RotYaw, 1);
		fRoll += Oscillate(Desc.RotRoll, 2);

		fLocX += Oscillate(Desc.LocX, 3);
		fLocY += Oscillate(Desc.LocY, 4);
		fLocZ += Oscillate(Desc.LocZ, 5);

		fFov += Oscillate(Desc.FOV, 6);

		++i;
	}

	CAM_POSE OutPose = Base;

	_vector vBaseQuat = XMLoadFloat4(&Base.vQuat);
	_vector vLocalOffset = XMVectorSet(fLocX, fLocY, fLocZ, 0.f);

	_vector vPos = XMVectorSetW(XMLoadFloat4(&Base.vPosition) + XMVector3Rotate(vLocalOffset, vBaseQuat), 1.f);
	XMStoreFloat4(&OutPose.vPosition, vPos);

	_vector vQuat = XMQuaternionMultiply(XMQuaternionRotationRollPitchYaw(fPitch, fYaw, fRoll), vBaseQuat);
	XMStoreFloat4(&OutPose.vQuat, vQuat);

	OutPose.fFovy = Base.fFovy + fFov * (XM_PI / 180.f);


	return OutPose;
}

void CCamera_Manager::Begin_Transition(_float fDuration)
{
	m_Transition.bActive = true;
	m_Transition.fDuration = (fDuration > 0.f) ? fDuration : 0.0001f;
	m_Transition.fElapsed = 0.f;

	m_Transition.vStartPos = m_BasePose.vPosition;
	m_Transition.vStartQuat = m_BasePose.vQuat;
	m_Transition.fStartFovy = m_BasePose.fFovy;
}

void CCamera_Manager::End_Scene()
{
	if(m_pTargetArm) 
		m_pTargetArm->Sync_FromQuat(XMLoadFloat4(&m_BasePose.vQuat));

	m_bActionEnding = true;
	Begin_Transition(m_fBlendDuration);
}

//전환등이 모두 끝나고 액션모드 종료 
void CCamera_Manager::End_Action()
{
	m_bActionEnding = false;
	Set_CameraMode(CAMERA_MODE::FOLLOW);
}

HRESULT CCamera_Manager::Load_ShakeDatas(const string& strFolderpath)
{
	m_PlayingShakes.clear();
	m_ShakeByHash.clear();

	Load_JsonFolder(strFolderpath, m_ShakeDescs, Parse_Shake);

	for (auto& Pair : m_ShakeDescs)
		m_ShakeByHash[Hash_StringA(Pair.first.c_str())] = &Pair.second;

	return S_OK;
}

const SHAKE_DESC* CCamera_Manager::Get_ShakeDesc(const string& strKey) const
{
	auto it = m_ShakeDescs.find(strKey);
	return (it == m_ShakeDescs.end()) ? nullptr : &it->second;
}

HRESULT CCamera_Manager::Play_Shake(const string& strKey)
{
	return Play_Shake(Hash_StringA(strKey.c_str()));
}

HRESULT CCamera_Manager::Play_Shake(_uint64 iKeyHash)
{
	auto it = m_ShakeByHash.find(iKeyHash);
	if (it == m_ShakeByHash.end()) return E_FAIL;

	const SHAKE_DESC* pShake = it->second;

	if (pShake == nullptr) return E_FAIL;

	//씬부터 재생
	if (!pShake->AnimSequenceKey.empty())
	{
		_float fIn = (pShake->fAnimBlendInTime > 0.f) ? pShake->fAnimBlendInTime : 0.15f;
		_float fOut = (pShake->fAnimBlendOutTime > 0.f) ? pShake->fAnimBlendOutTime : 0.4f;

		if (FAILED(Play_Scene(pShake->AnimSequenceKey, fIn, fOut, pShake->fAnimPlayRate)))
			return E_FAIL;

		// Play_Scene과 저장 공간은 공유하지만 CUTSCENE에서도 허용할 Shake Scene임을 구분한다.
		m_bPlayingShakeScene = true;
	}

	SHAKE_INSTANCE* pInst = nullptr;

	//이미 시작중인 놈이 있다면 재시작 
	if (pShake->bSingleInstance)
	{
		for (SHAKE_INSTANCE& Inst : m_PlayingShakes)
		{
			if (Inst.pDesc == pShake)
			{
				pInst = &Inst;
				break;
			}
		}
	}

	if (pInst == nullptr)
	{
		m_PlayingShakes.emplace_back();
		pInst = &m_PlayingShakes.back();
		pInst->pDesc = pShake;
	}

	pInst->fTime = 0.f;
	pInst->bStopping = false;
	pInst->fStopTime = 0.f;
	
	const FOscillatorDesc* pOscs[7] = { &pShake->RotPitch, &pShake->RotYaw, &pShake->RotRoll,
										&pShake->LocX, &pShake->LocY, &pShake->LocZ, &pShake->FOV };

	for (_int i = 0; i < 7; ++i)
		pInst->Phases[i] = (pOscs[i]->iInitialOffset == 0) ? XM_2PI * (rand() / (_float)RAND_MAX) : 0.f;

	return S_OK;
}

void CCamera_Manager::Stop_Shake(const string& strKey)
{
	Stop_Shake(Hash_StringA(strKey.c_str()));
}

void CCamera_Manager::Stop_Shake(_uint64 iKeyHash)
{
	auto it = m_ShakeByHash.find(iKeyHash);
	if (it == m_ShakeByHash.end()) return;

	const SHAKE_DESC* pShake = it->second;
	for (SHAKE_INSTANCE& Inst : m_PlayingShakes)
	{
		if (Inst.pDesc == pShake && !Inst.bStopping)
		{
			Inst.bStopping = true;
			Inst.fStopTime = Inst.fTime;
		}
	}
}


HRESULT CCamera_Manager::Load_SceneDatas(const string& strFolderpath)
{
	Load_JsonFolder(strFolderpath, m_SceneDescs, Parse_Scene);
	return S_OK;
}

const SCENE_DESC* CCamera_Manager::Get_SceneDesc(const string& strKey) const
{
	auto it = m_SceneDescs.find(strKey);
	return (it == m_SceneDescs.end()) ? nullptr : &it->second;
}

HRESULT CCamera_Manager::Play_Scene(const string& strKey,_float fBlendIn, _float fBlendOut, _float fPlayRate)
{
	const SCENE_DESC* pScene = Get_SceneDesc(strKey);
	if (pScene == nullptr) return E_FAIL;


	m_fScenePlayRate = (fPlayRate > 0.f) ? fPlayRate : 1.f;
	m_fSceneBlendIn = fBlendIn;
	m_fSceneBlendOut = fBlendOut;

	m_pPlayingScene = pScene;
	m_bPlayingShakeScene = false;
	m_fScenePlayTime = 0.f;

	return S_OK;
}

void CCamera_Manager::Stop_Scene()
{
	m_pPlayingScene = nullptr;
	m_bPlayingShakeScene = false;
}

void CCamera_Manager::Capture_Pose()
{
	m_vFreezePos = m_BasePose.vPosition;
	m_vFreezeQuat = m_BasePose.vQuat;
	m_fSavedFovy = m_BasePose.fFovy;
}

HRESULT CCamera_Manager::Load_CameraWorkDatas(const string& strFolderpath)
{
	m_pPlayingWork = nullptr;
	Release_WorkTarget();

	Load_JsonFolder(strFolderpath, m_CamWorkDescs, Parse_CamWork);
	return S_OK;
}

HRESULT CCamera_Manager::Play_CameraWork(const string& strKey, _fvector vTargetWorldPos)
{
	if (m_pTargetArm == nullptr) return E_FAIL;

	auto it = m_CamWorkDescs.find(strKey);
	if (it == m_CamWorkDescs.end()) return E_FAIL;

	Release_WorkTarget();
	m_pPlayingScene = nullptr;

	m_pPlayingWork = &it->second;
	m_fWorkTime = 0.f;
	XMStoreFloat3(&m_vWorkTargetPos, vTargetWorldPos);

	//시작 기준 포즈 Freeze 캡처 
	if(m_CameraMode != CAMERA_MODE::ACTION)
		Capture_Pose();

	//보간 시작점 = 현재 카메라 위치 
	m_vWorkLocation = m_BasePose.vPosition;

	//시작 거리 캡쳐 
	_vector vCam = XMLoadFloat4(&m_vFreezePos);
	_vector vPivot = vTargetWorldPos + XMVectorSet(0.f, m_fWorkPivotHeight, 0.f, 0.f);

	_float fStartCamDist = XMVectorGetX(XMVector3Length(vCam - vPivot));
	if (fStartCamDist < 0.01f) fStartCamDist = m_pTargetArm->Get_ArmLength();

	_vector vDirToCam = XMVector3Normalize(vCam - vPivot);

	//PCDistance용 CamWork면 실제 거리를 , AlignCurve면 TargetArm 길이를 기준으로 삼음
	_float fBaseDist = m_pPlayingWork->bUseStartDistance ? fStartCamDist
		: m_pTargetArm->Get_ArmLength();
	_float fDist = fBaseDist * m_pPlayingWork->fDistanceRate;
	
	//카메라 고정 오프셋
	_vector vAnchor = vDirToCam * fDist;

	// 타켓위치 기준으로 Work 스탭 밟을 사이드 방향 결정 
	if (m_pPlayingWork->bUseSide)
	{
		//g_XMIdentityR0 : XMVectorSet(1.f, 0.f, 0.f, 0.f) 인데 이미 정렬되어 올라가있는 상수
		_vector vRight = XMVector3Rotate(g_XMIdentityR0, XMLoadFloat4(&m_vFreezeQuat));
		//RIght벡터와 내적해서 +면 오른쪽, -면 왼쪽
		_float fDotValue = XMVectorGetX(XMVector3Dot(vRight, -vDirToCam));
		
		//if(fDotValue < 0.f) m_fWorkSideDirSign = -1.f; //왼쪽 
		//else m_fWorkSideDirSign = 1.f; // 오른쪽

		if (fabsf(fDotValue) < 0.2f) m_fWorkSideDirSign = 1.f;
		else m_fWorkSideDirSign = (fDotValue < 0.f) ? -1.f : 1.f;

		vAnchor += vRight * (m_pPlayingWork->fSideAmount * m_fWorkSideDirSign);
	}
	
	XMStoreFloat4(&m_vWorkParkedPos, XMVectorSetW(vPivot + vAnchor, 1.f));
	XMStoreFloat4(&m_vWorkParkedOffset, vAnchor);

	m_bActionEnding = false;
	Begin_Transition(m_pPlayingWork->fBlendInTime);
	Set_CameraMode(CAMERA_MODE::ACTION);
	return S_OK;
}

HRESULT CCamera_Manager::Play_CameraWork(const string& strKey, CGameObject* pTarget)
{
	if(pTarget == nullptr || pTarget->Get_Transform() == nullptr) return E_FAIL;
	if (FAILED(Play_CameraWork(strKey, pTarget->Get_Transform()->Get_State(STATE::POSITION))))
		return E_FAIL;

	//이전 대상 정리
	Release_WorkTarget();
	m_pWorkTargetObject = pTarget;	
	Safe_AddRef(m_pWorkTargetObject);
	return S_OK;
}

void CCamera_Manager::Release_WorkTarget()
{
	Safe_Release(m_pWorkTargetObject);
	m_pWorkTargetObject = nullptr;
}


void CCamera_Manager::Set_CameraWorkTarget(_fvector vTargetWorldPos)
{
	XMStoreFloat3(&m_vWorkTargetPos, vTargetWorldPos);
}

void CCamera_Manager::Stop_CameraWork()
{
	if (m_pPlayingWork == nullptr) return;
	if (m_CameraMode != CAMERA_MODE::ACTION || m_bActionEnding) return;

	m_fBlendDuration = m_pPlayingWork->fBlendOutTime;
	m_pPlayingWork = nullptr;
	Release_WorkTarget();
	End_Scene();
}

void CCamera_Manager::Advance_CameraWork(_float fTimeDelta, CAM_POSE& OutPose)
{
	if (m_pWorkTargetObject)
	{
		if (m_pWorkTargetObject->Is_Dead() || m_pWorkTargetObject->Is_InActive())
			Release_WorkTarget();
		else
			XMStoreFloat3(&m_vWorkTargetPos, m_pWorkTargetObject->Get_Transform()->Get_State(STATE::POSITION));
	}

	const CAMWORK_DESC& Work = *m_pPlayingWork;
	m_fWorkTime += fTimeDelta;

	//커브 입력용으로 시간 정규화 
	_float fRatio = (Work.fDuration > 0.f) ? m_fWorkTime / Work.fDuration : 1.f;
	if (fRatio > 1.f) fRatio = 1.f;
	
	//1. 대상 피벗 
	_vector vPivot = XMLoadFloat3(&m_vWorkTargetPos) + XMVectorSet(0.f, m_fWorkPivotHeight, 0.f, 0.f);

	//2. Local Offset
	_vector vLocalOffset = XMVectorSet(
		Sample_Channel(Work.OffsetX, fRatio),
		Sample_Channel(Work.OffsetY, fRatio),
		Sample_Channel(Work.OffsetZ, fRatio),
		0.f) * m_fWorkOffsetScale;

	//3. 카메라 위치
	//_vector vTargetLocation = vPivot + XMLoadFloat4(&m_vWorkAnchorOffset) + vWorldOffset;
	_vector vTargetLocation;
	if (Work.bWorldFixed)
	{
		//vTargetLocation = XMLoadFloat4(&m_vWorkParkedPos);
		vTargetLocation = vPivot + XMLoadFloat4(&m_vWorkParkedOffset);
	}
	else
	{
		CameraPose ArmPose = m_pTargetArm->Get_FinalCameraPose();
		_vector vArmPos = XMLoadFloat4(&ArmPose.vPosition);
		_vector vArmQuat = XMLoadFloat4(&ArmPose.vQuat);
		_vector vArmFwd = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), vArmQuat);
		_float  fArm = m_pTargetArm->Get_ArmLength();

		vTargetLocation = vArmPos + vArmFwd * (fArm * (1.f - Work.fDistanceRate));
		if (Work.bUseSide)
		{
			_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vArmFwd));
			vTargetLocation += vRight * (Work.fSideAmount * m_fWorkSideDirSign);
		}
		vTargetLocation += XMVector3Rotate(vLocalOffset, vArmQuat);
	}


	//4. 위치 보간 
	_vector vCur = XMLoadFloat4(&m_vWorkLocation);
	if (Work.fInterpSpeed > 0.f)
	{
		_float fSpeed = fTimeDelta * Work.fInterpSpeed;
		if (fSpeed > 1.f) fSpeed = 1.f;
		vCur = vCur + (vTargetLocation - vCur) * fSpeed;
	}
	else vCur = vTargetLocation;// 0이면 바로 스냅

	XMStoreFloat4(&m_vWorkLocation, XMVectorSetW(vCur,1.f));

	//항상 룩엣으로 변경함 
	_vector vLook = XMVector3Normalize(vPivot - vCur);
	_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook));
	_vector vUp = XMVector3Cross(vLook, vRight);

	_matrix matCamTo(vRight, vUp, vLook, XMVectorSet(0.f, 0.f, 0.f, 1.f));
	_vector vLookQuat = XMQuaternionRotationMatrix(matCamTo);

	//LookAt 가중치 커브 적용
	_vector vFinalQuat = vLookQuat;
	if (!Work.BoneAlpha.Keys.empty())
	{
		_float fLookWeight = fClamp(Sample_Channel(Work.BoneAlpha, fRatio), 0.f, 1.f);

		CameraPose ArmPose = m_pTargetArm->Get_FinalCameraPose();
		vFinalQuat = XMQuaternionSlerp(XMLoadFloat4(&ArmPose.vQuat), vLookQuat, fLookWeight);
	}

	// 6. FOV ( Radius로 변환 ) 
	_float fFov = m_fSavedFovy + Sample_Channel(Work.FovOffset, fRatio) * m_fWorkFovScale * (XM_PI / 180.f);

	//최종 결과
	OutPose.vPosition = m_vWorkLocation;
	XMStoreFloat4(&OutPose.vQuat, vLookQuat);
	OutPose.fFovy = fFov;

	//fDuration 이면(Door)  무한으로 . 내가 직접Stop 
	if (Work.fDuration > 0.f  && m_fWorkTime >= Work.fDuration)
	{
		m_pPlayingWork = nullptr;
		Release_WorkTarget();

		//카메라 락 유지중이면 마지막 포즈로 고정 
		if (m_bCameraLocked)
		{
			m_vFreezePos = OutPose.vPosition;
			m_vFreezeQuat = OutPose.vQuat;
		}
		else
		{
			m_fBlendDuration = Work.fBlendOutTime;
			End_Scene();
		}
	}


}

//Work Pose위에 Scene 포즈 가산하기 
void CCamera_Manager::Apply_SceneAdditive(_float fTimeDelta, CAM_POSE& InOutPose)
{
	m_fScenePlayTime += fTimeDelta * m_fScenePlayRate;
	
	_float3 vLocalPos, vLocalRot;
	_float fFov;

	Sample_Scene(*m_pPlayingScene, m_fScenePlayTime, vLocalPos, vLocalRot, fFov);

	_float fWeight = 1.f;
	//블랜딩
	if (m_fSceneBlendIn > 0.f && m_fScenePlayTime < m_fSceneBlendIn)
		fWeight = m_fScenePlayTime / m_fSceneBlendIn;

	_float fRemainTime = m_pPlayingScene->fDuration - m_fScenePlayTime;
	if (m_fSceneBlendOut > 0.f && fRemainTime < m_fSceneBlendOut)
		fWeight = min(fWeight, fRemainTime / m_fSceneBlendOut);

	fWeight = fClamp(fWeight, 0.f, 1.f);

	_vector vQuatBase = XMLoadFloat4(&InOutPose.vQuat);

	_vector vPosOffset = XMVectorScale(XMLoadFloat3(&vLocalPos), fWeight* m_fScenePosScale);
	_vector vPos = XMLoadFloat4(&InOutPose.vPosition) + XMVector3Rotate(vPosOffset, vQuatBase);

	_vector vRotOffset = XMQuaternionRotationRollPitchYaw(vLocalRot.x * fWeight, vLocalRot.y * fWeight, vLocalRot.z * fWeight);
	_vector vQuat = XMQuaternionMultiply(vRotOffset, vQuatBase);

	XMStoreFloat4(&InOutPose.vPosition, XMVectorSetW(vPos, 1.f));
	XMStoreFloat4(&InOutPose.vQuat, vQuat);

	if (m_fScenePlayTime >= m_pPlayingScene->fDuration)
	{
		m_pPlayingScene = nullptr;
		m_bPlayingShakeScene = false;
	}
}



_bool CCamera_Manager::Try_LockOn(_bool bLockOn)
{
	if (m_pTargetArm == nullptr || m_CameraMode == CAMERA_MODE::ACTION || m_CameraMode == CAMERA_MODE::CUTSCENE)
	{
		m_bLockOn = false;
		return false;
	}

	m_bLockOn = bLockOn;
	return m_bLockOn;
}

void CCamera_Manager::Set_LockOnTarget(_vector vTargetPose)
{
	XMStoreFloat3(&m_vLockOnTargetPos, vTargetPose);
}

void CCamera_Manager::Set_CameraLock(_bool bLock)
{
	if (m_pPlayingWork != nullptr) return;
	if (bLock == m_bCameraLocked) return;
	m_bCameraLocked = bLock;

	if (bLock)
	{
		Begin_CameraLock();
	}
	else if( m_CameraMode == CAMERA_MODE::ACTION)
	{
		End_Scene();
	}
}

void CCamera_Manager::Begin_UIPreviewCamera(_fvector vEye, _fvector vAt, _float fFovy)
{
	// 호출자가 넘긴 위치와 주시점을 프리뷰 카메라 상태로 저장
	XMStoreFloat4(&m_vUIPreviewEye, XMVectorSetW(vEye, 1.f));
	XMStoreFloat4(&m_vUIPreviewAt, XMVectorSetW(vAt, 1.f));

	// 잘못된 FOV가 들어오면 기존 기본값을 유지
	if (0.f < fFovy)
		m_fUIPreviewFovy = fFovy;

	m_bUIPreviewCamActive = true;
}

void CCamera_Manager::End_UIPreviewCamera()
{
	m_bUIPreviewCamActive = false;
}

void CCamera_Manager::Begin_CameraLock()
{
	//현재 기준 포즈에서 정지.
	Capture_Pose();

	m_fScenePlayTime = 0.f;
	m_bActionEnding = false;
	m_Transition.bActive = false;

	Set_CameraMode(CAMERA_MODE::ACTION);
}


void CCamera_Manager::Set_CameraMode(CAMERA_MODE eMode)
{
	if (eMode == m_CameraMode) return;
	m_CameraMode = eMode;
	
	if (eMode == CAMERA_MODE::FOLLOW) m_pGameInstance->Broadcast(EVENT_TYPE::CAMERA_ACTION_END, nullptr);
	else
	{
		m_pGameInstance->Broadcast(EVENT_TYPE::CAMERA_ACTION_BEGIN, nullptr);
		m_bLockOn = false;
	}
}


HRESULT CCamera_Manager::Begin_CutSceneCamera(CCamera* pCutSceneCamera, _float fBlendIn)
{
	if (nullptr == pCutSceneCamera || nullptr == pCutSceneCamera->Get_Transform())
		return E_FAIL;

	if (pCutSceneCamera == m_pCutSceneCameraSource && CAMERA_MODE::CUTSCENE == m_CameraMode)
		return S_OK;

	// 같은 카메라를 다시 사용하는 경우에는 참조 횟수를 변경하지 않는다.
	// 다른 카메라로 변경할 때만 새 소유 참조를 확보하고 기존 참조를 반환한다.
	if (pCutSceneCamera != m_pCutSceneCameraSource)
	{
		Safe_AddRef(pCutSceneCamera);
		Safe_Release(m_pCutSceneCameraSource);
		m_pCutSceneCameraSource = nullptr;
		m_pCutSceneCameraSource = pCutSceneCamera;
	}

	// 컷신 시작 전 실제 메인 카메라 Pose를 저장한다.
	XMStoreFloat4(&m_PreCutScenePose.vPosition, m_pCamTransform->Get_State(STATE::POSITION));
	XMStoreFloat4(&m_PreCutScenePose.vQuat, m_pCamTransform->Get_Quat());
	m_PreCutScenePose.fFovy = m_pMainCamera->Get_Fovy();
	m_bLockOnBeforeCutScene = m_bLockOn;

	// 에디터 FREE만 이전 모드로 복구하고, ACTION은 종료된 작업이므로 FOLLOW로 복구한다.
	m_eCameraModeBeforeCutScene = CAMERA_MODE::FREE == m_CameraMode ? CAMERA_MODE::FREE : CAMERA_MODE::FOLLOW;

	// 기존 CameraWork와 일반 Camera Scene은 정리하되, 이미 재생 중인 Shake Scene은 유지한다.
	m_pPlayingWork = nullptr;
	Release_WorkTarget();
	if (false == m_bPlayingShakeScene)
		m_pPlayingScene = nullptr;
	m_bActionEnding = false;
	m_Transition.bActive = false;
	m_bCameraLocked = false;

	Set_CameraMode(CAMERA_MODE::CUTSCENE);

	if (fBlendIn > 0.f)
		Begin_Transition(fBlendIn);
	else
		Mark_CameraDiscontinuity();

	return S_OK;
}

HRESULT CCamera_Manager::Transition_CutSceneCamera(_float fBlendDuration)
{
	if (CAMERA_MODE::CUTSCENE != m_CameraMode || nullptr == m_pCutSceneCameraSource)
		return E_FAIL;

	// 직전 최종 Pose를 시작점으로 잡고, 같은 가상 카메라가 공급하는 다음 Section Pose로 전환한다.
	if (fBlendDuration > 0.f)
		Begin_Transition(fBlendDuration);
	else
	{
		m_Transition.bActive = false;
		Mark_CameraDiscontinuity();
	}

	return S_OK;
}

void CCamera_Manager::End_CutSceneCamera(_float fBlendOut)
{
	if (CAMERA_MODE::CUTSCENE != m_CameraMode)
	{
		// CameraManager가 소유하던 참조를 반환한 뒤 멤버 포인터도 반드시 비운다.
		Safe_Release(m_pCutSceneCameraSource);
		m_pCutSceneCameraSource = nullptr;
		return;
	}

	const _bool bRestoreFreeCamera = CAMERA_MODE::FREE == m_eCameraModeBeforeCutScene;
	const _bool bHardCutToFollowCamera = CAMERA_MODE::FOLLOW == m_eCameraModeBeforeCutScene && fBlendOut <= 0.f;

	// 에디터 FREE는 컷신 시작 전 메인 카메라 Pose로 즉시 복구한다.
	if (bRestoreFreeCamera)
	{
		m_BasePose = m_PreCutScenePose;
		m_pCamTransform->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat4(&m_PreCutScenePose.vPosition), 1.f));
		m_pCamTransform->Set_Quat(XMLoadFloat4(&m_PreCutScenePose.vQuat));
		m_pMainCamera->Set_Fovy(m_PreCutScenePose.fFovy);
	}

	// CameraManager가 소유하던 참조를 반환한 뒤 해제된 포인터가 남지 않게 한다.
	Safe_Release(m_pCutSceneCameraSource);
	m_pCutSceneCameraSource = nullptr;

	// Set_CameraMode(FREE)는 CAMERA_ACTION_BEGIN을 다시 발생시키므로 직접 모드를 복원한다.
	m_CameraMode = m_eCameraModeBeforeCutScene;
	m_bLockOn = CAMERA_MODE::FOLLOW == m_CameraMode ? m_bLockOnBeforeCutScene : false;
	m_pGameInstance->Broadcast(EVENT_TYPE::CAMERA_ACTION_END, nullptr);

	// FREE 복귀는 항상 즉시 이동하고, FOLLOW도 BlendOut이 없으면 이전 Hi-Z를 사용할 수 없는 하드컷이다.
	if (bRestoreFreeCamera || bHardCutToFollowCamera)
		Mark_CameraDiscontinuity();

	// FOLLOW 복귀는 마지막 컷신 Pose에서 SpringArm Pose로 부드럽게 전환한다.
	if (CAMERA_MODE::FOLLOW == m_CameraMode && fBlendOut > 0.f)
		Begin_Transition(fBlendOut);
}

void CCamera_Manager::Mark_CameraDiscontinuity()
{
	// 0은 초기 상태로 남겨 두고, 오버플로 시에도 실제 변경을 나타내는 값으로 진행한다.
	++m_iCameraDiscontinuityRevision;

	if (0 == m_iCameraDiscontinuityRevision)
		++m_iCameraDiscontinuityRevision;
}

_float CCamera_Manager::Get_MainCameraFovy() const
{
	return nullptr != m_pMainCamera ? m_pMainCamera->Get_Fovy() : 0.f;
}


#ifdef _DEBUG


void CCamera_Manager::Free_Move(_float fTimeDelta)
{
	_long iWheel = m_pGameInstance->Get_DIMouseMove(DIMM::WHEEL);
	if (iWheel)
	{
		m_fMoveSpeed += (static_cast<_float>(iWheel / 120) / 10.f); // 한번굴리면 +-120
		if (m_fMoveSpeed < 0) m_fMoveSpeed = 0.1f;
	}

	if (m_pGameInstance->Key_Pressing(DIK_W))
	{
		m_pCamTransform->Go_Straight(fTimeDelta * m_fMoveSpeed);
	}
	if (m_pGameInstance->Key_Pressing(DIK_S))
	{
		m_pCamTransform->Go_Backward(fTimeDelta * m_fMoveSpeed);
	}
	if (m_pGameInstance->Key_Pressing(DIK_A))
	{
		m_pCamTransform->Go_Left(fTimeDelta * m_fMoveSpeed);
	}
	if (m_pGameInstance->Key_Pressing(DIK_D))
	{
		m_pCamTransform->Go_Right(fTimeDelta * m_fMoveSpeed);
	}

	_long       MouseMove = {};
	if (m_pGameInstance->Mouse_Pressing(DIMB::RBUTTON))
	{
		if (MouseMove = m_pGameInstance->Get_DIMouseMove(DIMM::X))
		{
			m_pCamTransform->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), MouseMove * m_fDebugMouseSensor * fTimeDelta);
		}

		if (MouseMove = m_pGameInstance->Get_DIMouseMove(DIMM::Y))
		{
			m_pCamTransform->Turn(m_pCamTransform->Get_State(STATE::RIGHT), MouseMove * m_fDebugMouseSensor * fTimeDelta);
		}
	}

}

void CCamera_Manager::Set_CameraPosition(_fvector vPosition)
{
	m_pCamTransform->Set_State(STATE::POSITION, vPosition);
}
#endif // _DEBUG

CCamera_Manager* CCamera_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCamera_Manager* pInstance = new CCamera_Manager(pDevice, pContext);
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CCamera_Manager");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CCamera_Manager::Free()
{
	__super::Free();

	Safe_Release(m_pCutSceneCameraSource);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
	Safe_Release(m_pMainCamera);

	m_ShakeDescs.clear();
	m_SceneDescs.clear();
	Release_WorkTarget();
}
