#include "GrappleGun.h"
#include "Player.h"
#include "PartsModel.h"
#include "GameInstance.h"
#include "AnimDriver.h"
#include "SpringArm.h"
#include "Status.h"
#include "Transform.h"
#include "MasterModel.h"
#include "AIObject.h"
#include "Movement.h"
#include "AimComponent.h"

CGrappleGun::CGrappleGun(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CLegionArm{ pDevice, pContext }
{
}

HRESULT CGrappleGun::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CGrappleGun : Arg is nullptr");
		return E_FAIL;
	}

	CLegionArm::LEGIONARM_DESC* pDesc = static_cast<CLegionArm::LEGIONARM_DESC*>(pArg);
	pDesc->strModelTag = L"Prototype_Component_Model_Player_GrappleGun";

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_eType = LegionArmType::Grapple;
	m_pWire = CGrappleWire::Create(m_pDevice, m_pContext, CGrappleWire::WIRE_DESC{});

	if (m_pWire == nullptr)
	{
		MSG_BOX("CGrappleGun : Wire Create Failed");
		return E_FAIL;
	}

	m_pOwnerRootMat = m_pMasterModel->Get_Bone_CombinedMatPtr(ROOT_BONE_NAME);
	m_pMuzzleBoneMat = m_pMasterModel->Get_Bone_CombinedMatPtr(WIRE_START_BONE);

	if (m_pOwnerRootMat == nullptr || m_pMuzzleBoneMat == nullptr)
	{
		MSG_BOX("CGrappleGun : Bone Combined Matrix is nullptr");
		return E_FAIL;
	}

	CAimBoneChain::AIM_PROFILE AimProfile = {};
	AimProfile.Bones =
	{
		{ "Bip001-Spine", 0.20f },
		{ "Bip001-Spine1", 0.30f },
		{ "Bip001-Spine2", 0.50f }
	};
	AimProfile.fMaxYaw = AimProfile.fMaxPitchUp = AimProfile.fMaxPitchDown = XMConvertToRadians(AIM_CHAIN_MAX_ANGLE);

	m_iAimProfile = m_pAimCom->Register_AimProfile(AimProfile);
	if (m_iAimProfile == CAimComponent::INVALID_PROFILE)
		return E_FAIL;

	return S_OK;
}

void CGrappleGun::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	Update_AimPose();

	//공중 불발 + 착지하면 종료
	if (m_bResumeAim && m_pStatusCom->Is_Grounded())
	{
		End_Grapple();
		return;
	}

	if (m_eState == GrappleState::Idle)
		return;

	//타겟이 무효화되면 타겟만 해제하고 연출은 진행
	if(m_pGrappleTarget && (m_pGrappleTarget->Is_InActive() || m_pGrappleTarget->Is_Dead()))
	{
		if (m_bTargetGrabbed)
			m_pGrappleTarget->End_GrappleGrab();
		m_bTargetGrabbed = false;
		m_bPullDone = true;

		Safe_Release(m_pGrappleTarget);
		m_pGrappleTarget = nullptr;
	}

	// 노말버전 - 적 당기기  
	if (m_eState == GrappleState::Pull)
	{
		if (m_pGrappleTarget && !m_bPullDone && m_fStateTime >= GRAPPLE_PULL_DELAY)
		{
			if (m_bPullStarted == false)
			{
				m_bPullStarted = true;
				m_pGameInstance->Play_Shake(CAM_SHAKE_PULLING);
				m_pGrappleTarget->On_GrapplePullStart();

				m_pSpringArm->End_Adjust();

				//당기기 시작할 때 타깃 목적지 설정
				_float fMinDist = Get_MinDistToTarget(GRAPPLE_PULL_DEST_GAP);
				_vector vLookXZ = XMVector3Normalize(XMVectorSetY(m_pOwnerTransform->Get_State(STATE::LOOK), 0.f));
				XMStoreFloat3(&m_vPullDestPos, m_pOwnerTransform->Get_State(STATE::POSITION) + vLookXZ * fMinDist);
			}

			_float fPullRatio = min((m_fStateTime - GRAPPLE_PULL_DELAY) / GRAPPLE_PULL_TIME, 1.f);
			_vector vTargetPos = XMVectorLerp(XMLoadFloat3(&m_vPullStartPos), XMLoadFloat3(&m_vPullDestPos), Ease(EASE_TYPE::EASE_IN, fPullRatio));

			m_pGrappleTarget->On_GrapplePulled(vTargetPos);

			if (fPullRatio >= 1.f)
				m_bPullDone = true;
		}
	}
	// 강화버전 - 추적
	else if (m_eState == GrappleState::Chase)
	{
		_bool bCanSmash = false;
		if (m_pGrappleTarget)
		{
			_vector vPos = m_pOwnerTransform->Get_State(STATE::POSITION);
			_vector vTargetPos = m_pGrappleTarget->Get_Transform()->Get_State(STATE::POSITION);
			_float fDist = XMVectorGetX(XMVector3Length(vPos - vTargetPos));

			if (fDist <= Get_MinDistToTarget(SMASH_EXTRA))
				bCanSmash = true;
		}
		
		if (bCanSmash)
		{
			Enter_Smash();
		}
		//목적지에 도착했는데 스메시 거리가 아니라면 종료 
		else if (m_pOwner->Get_MovementComponent()->Is_MoveReached())
		{
			End_Grapple();
			return;
		}
	}
	//입체기동 - 추적
	else if (m_eState == GrappleState::Zip)
	{
		if (m_pOwner->Get_MovementComponent()->Is_MoveReached())
		{
			Release_Zip();
			return;
		}
	}
	else if (m_eState == GrappleState::Aim)
	{
		Update_Aim(fTimeDelta);
		return;
	}
	else if (m_eState == GrappleState::Smash)
	{
		//스냅 값 기준 y축 차이값을 SpringArm에 보내 루트모션이 아닌 애니메이션에도 카메라가 따라오게 함
		m_pSpringArm->Set_PivotExtraY(max(m_pOwnerRootMat->m[3][1] - m_fSmashBaseBoneY, 0.f));
	}

	m_fStateTime += fTimeDelta;
	const _bool bGrappleAnimPlay = m_pAnimDriver->Is_GrappleActive();

	if (!m_bAnimEntered && bGrappleAnimPlay)
		m_bAnimEntered = true;
	
	// 그래플 애니메이션 종료 
	if ((m_bAnimEntered && !bGrappleAnimPlay) )
	{
		_bool bAirAimResume = m_bResumeAim && !m_pStatusCom->Is_Grounded();

		End_Grapple();

		if (bAirAimResume)
			Request_Aim();
		return;
	}
	
	// 수락 후 일정 시간 동안 그래플 애니메이션에 진입 못하면 종료 
	if (!m_bAnimEntered && m_fStateTime > 1.f)
	{
		End_Grapple();
		return;
	}
}

HRESULT CGrappleGun::Render()
{
	if (FAILED(__super::Render()))
		return E_FAIL;

	if (m_pWire && m_eState != GrappleState::Idle)
		m_pWire->Render();

	return S_OK;
}

//와이어 시작점은 이번 프레임의 최종 위치가 필요하므로 본 갱신 이후 연산
void CGrappleGun::Post_BoneUpdate(_float fTimeDelta)
{
	if (m_eState == GrappleState::Idle || m_eState == GrappleState::Aim)
		return;

	Update_Wire(fTimeDelta);
}

_float CGrappleGun::Get_MinDistToTarget(_float fGap) const
{
	_float fDist = m_pOwner->Get_CCTRadius() + fGap;

	if (m_pGrappleTarget)
		fDist += m_pGrappleTarget->Get_CCTRadius();

	return fDist;
}

void CGrappleGun::On_AnimEvent(LegionArmEvent eEvent)
{
	if (m_eState == GrappleState::Idle)
	{
		if (eEvent == LegionArmEvent::Grapple_Accepted)
			Begin_Grapple();
		else if (eEvent == LegionArmEvent::Grapple_AimAccepted)
			Enter_Aim();

		return;
	}

	// 노티파이 이벤트 
	switch (eEvent)
	{
	case LegionArmEvent::Grapple_WireLaunch:
		m_bWireLaunched = true;
		m_fStateTime = 0.f;
		break;

	case LegionArmEvent::Grapple_Pull:
		Enter_Pull();
		break;

	case LegionArmEvent::Grapple_Fail:
		Enter_Fail();
		break;

	case LegionArmEvent::Grapple_Chase:
	{
		//타겟이 정적 타깃 = 입체기동
		if (m_bPendingTarget_Static)
			Enter_Zip();
		else
			Enter_Chase();
		break;
	}
	case LegionArmEvent::Grapple_Smash_Strike:
	case LegionArmEvent::Grapple_Smash_Jump:
		Smash_Attack(eEvent);
		break;

	default:
		break;
	}
}

void CGrappleGun::Update_Wire(_float fTimeDelta)
{
	CGrappleWire::WIRE_PROFILE Profile = WIRE_PROFILES[ETOUI(m_eState)];

	switch (m_eState)
	{
	case GrappleState::Fire:
	{
		//0 -> 1
		m_fWireProgress = !m_bWireLaunched ? 0.f : min(m_fStateTime / WIRE_FIRE_TIME, 1.f);
		break;
	}
	case GrappleState::Pull:
	{
		m_fWireProgress = 1.f;
		break;
	}
	case GrappleState::Fail:
	{
		//1 -> 0
		m_fWireProgress = max(1.f - m_fStateTime / WIRE_REWIND_TIME, 0.f);
		//시간이 지날수록 팽팽하게 
		Profile.fSlack = 1.f + (WIRE_PROFILES[ETOUI(GrappleState::Fail)].fSlack - 1.f) * m_fWireProgress;
		break;
	}
	case GrappleState::Chase:
	{
		m_fWireProgress = 1.f;
		break;
	}
	case GrappleState::Smash:
	{
		m_fWireProgress = 0.f;
		break;
	}
	case GrappleState::Zip:
	{
		m_fWireProgress = 1.f;
		break;
	}
	default:
		break;
	}

	if (m_pWire == nullptr)
		return;

	if (m_pGrappleTarget)
	{
		_vector vTargetPos = {};
		if(m_pGrappleTarget->Get_LockOnWorldPosition(vTargetPos))
			XMStoreFloat3(&m_vWireEndPos, vTargetPos);
	}

	//줄이 끝까지 발사되면 이펙트 재생 
	if (!m_bWireHited && m_bPendingTarget && m_fWireProgress >= 1.f)
	{
		m_bWireHited = true;
		m_pGameInstance->Play_Effect(WIRE_HIT_EFFECT, m_vWireEndPos);
	}

	if (m_fWireProgress <= 0.f)
	{
		m_pWire->Hide();
		return;
	}

	_matrix MuzzleWorld = XMLoadFloat4x4(m_pMuzzleBoneMat) * XMLoadFloat4x4(m_pParentMatrix);
	_vector vMuzzlePos = MuzzleWorld.r[3];
	_vector vEndPos = XMVectorLerp(vMuzzlePos, XMLoadFloat3(&m_vWireEndPos), SmoothStep(m_fWireProgress));

	if (!m_pWire->Is_Visible())
		m_pWire->Activate(vMuzzlePos, vEndPos);
	else
		m_pWire->Set_Endpoints(vMuzzlePos, vEndPos);

	m_pWire->Set_Profile(Profile);
	m_pWire->Update(fTimeDelta);

}


void CGrappleGun::Update_YawFollow(_float fTimeDelta)
{
	_float fCamYaw = m_pSpringArm->Get_HorizonValue();
	_vector vTargetQuat = XMQuaternionRotationRollPitchYaw(0.f, fCamYaw, 0.f);

	_float fRatio = 1.f - expf(-AIM_YAW_FOLLOW_RATE * fTimeDelta);
	m_pOwnerTransform->Smooth_Turn(vTargetQuat, fRatio);
}

void CGrappleGun::Update_AimPose()
{
	if (m_eState == GrappleState::Aim)
		m_pAimCom->Request_AimPoseToCamera(m_iAimProfile);
	else if (m_eState == GrappleState::Fire)
		m_pAimCom->Request_AimPoseToPoint(m_iAimProfile, XMLoadFloat3(&m_vWireEndPos));
}

void CGrappleGun::Use()
{
	if (m_eState != GrappleState::Idle)
		return;

	//그래플 관련 애니메이션 플레이 중인가
	if (m_pAnimDriver->Is_GrappleActive())
		return;

	if (!m_pStatusCom->Is_Grounded())
		return;

	Fire_QuickShot(false);
}

void CGrappleGun::Use_Upgrade()
{
	if (m_eState == GrappleState::Aim)
	{
		Fire_AimedShot();
		return;
	}

	if (m_eState != GrappleState::Idle || m_pAnimDriver->Is_GrappleActive())
		return;

	if (m_pStatusCom->Is_Grounded())
	{
		Fire_QuickShot(true);
		return;
	}

	//입체기동 해금 + 공중일때만 조준모드 발동
	if(m_bMobilityUnlocked)
		Request_Aim();
}

void CGrappleGun::Reset_Pending()
{
	Safe_Release(m_pGrappleTarget);
	m_pGrappleTarget = nullptr;

	m_bPendingTarget = false;
	m_bPendingUpgrade = false;
	m_bPendingTarget_Static = false;
}

void CGrappleGun::Cancel()
{
	if (m_eState == GrappleState::Idle)
	{
		Set_AimSlow(false);
		m_pSpringArm->End_Adjust();
		Set_CrosshairVisible(false);
		Reset_Pending();
		return;
	}
	else if (m_eState == GrappleState::Aim)
	{
		Exit_Aim();
		return;
	}
	Clear_Grapple(false);
}


//Reset - Sweep 탐색 - 타겟 확정 - 애니 요청 순서를 보장 
void CGrappleGun::Fire_QuickShot(_bool bUpgrade)
{
	_vector vStart = {}, vDir = {};

	Reset_Pending();
	m_bPendingUpgrade = bUpgrade;
	
	CAIObject* pTarget = m_pStatusCom->Get_LockedTarget();
	
	if (pTarget)
		Compute_RayToTarget(pTarget, XMVectorSet(SWEEP_OFFSET.x, SWEEP_OFFSET.y, SWEEP_OFFSET.z, 0.f), vStart, vDir);
	else 
		Compute_CameraRay(vStart, vDir);

	m_bPendingTarget = Commit_Preview(Sweep_GrappleRay(vStart, vDir));
	Play_GrappleAction();
}

void CGrappleGun::Fire_AimedShot()
{
	_vector vStart = {}, vDir = {};
	Compute_CameraRay(vStart, vDir);

	SHOT_RESULT shotResult = Sweep_GrappleRay(vStart, vDir);

	_bool bGrounded = m_pStatusCom->Is_Grounded();
	_bool bAirMiss = !bGrounded && !shotResult.bHit;

	if (bAirMiss)
	{
		//불발되면 다시 Aim으로 복귀
		m_pAnimDriver->Set_Param(PARAM_GRAPPLE_AIMING, false);
		m_eState = GrappleState::Idle;
		m_bAnimEntered = false;
	}
	else
	{
		Exit_Aim();
	}

	Reset_Pending();

	m_bPendingTarget = Commit_Preview(shotResult);
	//타겟이 몬스터면 강화버전으로 진행 
	if (m_bPendingTarget && !m_bPendingTarget_Static)
		m_bPendingUpgrade = true;

	m_bResumeAim = bAirMiss;

	Play_GrappleAction();
}


CGrappleGun::SHOT_RESULT CGrappleGun::Sweep_GrappleRay(_fvector vStart, _fvector vDir) const
{
	SHOT_RESULT outResult = {};
	FILTER_GROUP eFilter = static_cast<FILTER_GROUP>
		( 
		ETOUI(FILTER_GROUP::MONSTER) 
		| ETOUI(FILTER_GROUP::MONSTEROBJECT) 
		| ETOUI(FILTER_GROUP::GROUND) 
		| ETOUI(FILTER_GROUP::OBJECT)
		);
	
	CollisionResult colResult{};
	if (false == m_pGameInstance->Sweep_Sphere(SWEEP_RADIUS, vStart, vDir, MAX_DISTANCE, eFilter, colResult, PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC))
	{
		//잡기 실패하면 최대사거리 반환
		XMStoreFloat3(&outResult.vHitPos, vStart + vDir * MAX_DISTANCE);
		return outResult;
	}

	_uint iTargetFilter = 0;
	if (colResult.tSweepData.pHitShape)
		iTargetFilter = colResult.tSweepData.pHitShape->getQueryFilterData().word0;

	//Static Target이면 
	if (iTargetFilter & ETOUI(FILTER_GROUP::GROUND) || iTargetFilter & ETOUI(FILTER_GROUP::OBJECT))
	{
		outResult.bHit = true;
		//입체기동이 켜져있어야 Static에 유효값 전달함
		outResult.bStatic = m_bMobilityUnlocked;
		outResult.vHitPos = colResult.tSweepData.vHitPosition;
		outResult.pTarget = nullptr; 
	}
	//Dynamic Target(몬스터)이면
	else 
	{
		CAIObject* pAIObject = dynamic_cast<CAIObject*>(colResult.pOtherActor);
		if (pAIObject != nullptr)
		{
			outResult.bHit = true;
			outResult.bStatic = false;

			_vector vHitPos = {};
			if (pAIObject->Get_LockOnWorldPosition(vHitPos))
				XMStoreFloat3(&outResult.vHitPos, vHitPos);
			else
				outResult.vHitPos = colResult.tSweepData.vHitPosition;

			if (pAIObject->Can_GrappleGrab() && !pAIObject->Is_Dead() && !pAIObject->Is_InActive())
				outResult.pTarget = pAIObject;
		}
	}

	//그 외에는 최대 사거리 (줄이 원점으로 날아가는 것을 방지) 
	if (!outResult.bHit)
		XMStoreFloat3(&outResult.vHitPos, vStart + vDir * MAX_DISTANCE);

	return outResult;
}

_bool CGrappleGun::Commit_Preview(const SHOT_RESULT& result)
{
	m_vHitPosition = result.vHitPos;
	m_vWireEndPos = result.vHitPos;
	m_bPendingTarget_Static = result.bStatic;

	if (result.bStatic)
		return true;

	if (result.pTarget && !result.pTarget->Is_Dead() && !result.pTarget->Is_InActive())
	{
		m_pGrappleTarget = result.pTarget;
		Safe_AddRef(m_pGrappleTarget);
		return true;
	}

	return false;
}

// inputLock 초기화 여부를 전달받음 
// 맵 전환 등 InputLock 하면서 그래플을 취소할경우, 기존 InputLock 플래그를 건들지않음 
void CGrappleGun::Clear_Grapple(_bool bRestoreInputLock)
{
	if (m_pSpringArm)
	{
		m_pSpringArm->End_Adjust();
		m_pSpringArm->Set_PivotExtraY(0.f);
	}

	if (m_bTargetGrabbed && m_pGrappleTarget)
		m_pGrappleTarget->End_GrappleGrab();


	if (bRestoreInputLock)
		m_pStatusCom->Set_InputLock(false);

	Set_CrosshairVisible(false);

	m_eState = GrappleState::Idle;
	m_bAnimEntered = false;
	m_bTargetGrabbed = false;
	m_bPullDone = false;
	m_bPullStarted = false;
	m_bResumeAim = false;

	m_fWireProgress = 0.f;
	m_bWireLaunched = false;
	m_bWireHited = false;
	m_bPendingTarget = false;
	m_bPendingTarget_Static = false;

	Safe_Release(m_pGrappleTarget);
	m_pGrappleTarget = nullptr;

	m_pOwner->Get_MovementComponent()->Stop_MoveTo();
	m_pStatusCom->Set_GravityDisabled(false);
	m_pGameInstance->Stop_CameraWork();

	Set_AimSlow(false);
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_AIMING, false);

	if (m_pWire)
	{
		m_pWire->Hide();
	}
}

void CGrappleGun::Play_GrappleAction()
{
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_HIT, m_bPendingTarget && !m_bPendingTarget_Static);
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_UPGRADE, m_bPendingUpgrade);
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_NEAR, false);
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_ZIP, m_bPendingTarget_Static);

	if (!m_bPendingTarget_Static && m_bPendingUpgrade && m_bPendingTarget)
		m_pAnimDriver->Play_Action(AnimAction::Grapple_Upgrade);
	else
		m_pAnimDriver->Play_Action(AnimAction::Grapple);
}

void CGrappleGun::Begin_Grapple()
{
	m_eState = GrappleState::Fire;

	m_pStatusCom->Set_InputLock(true);
	m_bAnimEntered = false;
	m_fStateTime = 0.f;
	m_bWireLaunched = false;

	//줄이 나가는 방향으로 회전시킴
	_vector vPlayerPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	_vector vDirXZ = XMVectorSetY(XMLoadFloat3(&m_vWireEndPos) - vPlayerPos, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vDirXZ)), 0.f) == false)
	{
		_float fYaw = atan2f(XMVectorGetX(vDirXZ), XMVectorGetZ(vDirXZ));
		_vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f);

		m_pOwnerTransform->Set_Quat(vQuat);
		//루트모션 회전에 덮어씌움
		m_pOwner->Get_MovementComponent()->Set_RootMotionQuat(vQuat);
	}

}

void CGrappleGun::Enter_Pull()
{
	m_eState = GrappleState::Pull;
	m_fStateTime = 0.f;

	if (m_pGrappleTarget && !m_bTargetGrabbed)
	{
		m_bTargetGrabbed = true;
		m_pGrappleTarget->Begin_GrappleGrab();
		XMStoreFloat3(&m_vPullStartPos, m_pGrappleTarget->Get_Transform()->Get_State(STATE::POSITION));
	}
	
	Begin_ChangeView();
}

void CGrappleGun::Enter_Fail()
{
	m_eState = GrappleState::Fail;
	m_fStateTime = 0.f;
}

void CGrappleGun::Enter_Chase()
{
	if (m_pGrappleTarget == nullptr) return;

	m_eState = GrappleState::Chase;
	m_fStateTime = 0.f;

	_vector vPlayerPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	_vector vTargetPos = m_pGrappleTarget->Get_Transform()->Get_State(STATE::POSITION);

	_vector vDir = vTargetPos - vPlayerPos;
	_float fDist = XMVectorGetX(XMVector3Length(vDir));
	vDir = XMVector3Normalize(vDir);

	_float fMoveDist = max(fDist - Get_MinDistToTarget(CHASE_STOP_EXTRA), 0.f);
	_vector vDestPos = vPlayerPos + vDir * fMoveDist;

	//Begin_Grapple에서 줄 끝 방향으로 이미 회전시켰으므로 현재 Quat 그대로 사용 
	_vector vQuat = m_pOwnerTransform->Get_Quat();

	m_pOwner->Get_MovementComponent()->Begin_MoveTo(vDestPos, vQuat, fMoveDist / CHASE_SPEED, EASE_TYPE::EASE_IN, true);
	m_pStatusCom->Set_GravityDisabled(true);

}

void CGrappleGun::Enter_Smash()
{
	m_eState = GrappleState::Smash;
	m_fStateTime = 0.f;
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_NEAR, true); 
	m_pAnimDriver->Play_Action(AnimAction::Grapple_Upgrade);

	m_fSmashBaseBoneY = m_pOwnerRootMat->m[3][1];
	m_pGameInstance->Stop_CameraWork();
}


void CGrappleGun::End_Grapple()
{
	Clear_Grapple(true);
}

void CGrappleGun::Enter_Zip()
{
	m_eState = GrappleState::Zip;
	m_fStateTime = 0.f;

	_vector vPlayerPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	_vector vDir = XMLoadFloat3(&m_vHitPosition) - vPlayerPos;
	_float fDist = XMVectorGetX(XMVector3Length(vDir));
	vDir = XMVector3Normalize(vDir);

	_float fDistToDest = max(fDist - (m_pOwner->Get_CCTRadius() + ZIP_STOP_EXTRA), 0.f);
	_vector vDestPos = vPlayerPos + vDir * fDistToDest;
	_vector vDestQuat = m_pOwnerTransform->Get_Quat();

	m_pOwner->Get_MovementComponent()->Begin_MoveTo(vDestPos, vDestQuat, fDistToDest / CHASE_SPEED, EASE_TYPE::EASE_IN, true);
	m_pStatusCom->Set_GravityDisabled(true);
}

void CGrappleGun::Release_Zip()
{
	CMovement* pMovement = m_pOwner->Get_MovementComponent();
	if (pMovement)
	{
		_float fPlayerY = XMVectorGetY(m_pOwnerTransform->Get_State(STATE::POSITION));
		_float fRiseHeight = max(m_vHitPosition.y - fPlayerY, 0.f) + RELEASE_CLEAR_HEIGHT;
		_float fGravity = fabsf(pMovement->Get_FallGravity());

		//에너지 보존 법칙 vel = sqrt(2 * g * h) 
		//목표 높이만큼 올라가는데 필요한 속도 
		_float fUpSpeed = min(sqrtf(2.f * fGravity * fRiseHeight), RELEASE_UP_MAX_SPEED);

		m_pAnimDriver->Set_Param(PARAM_GRAPPLE_ZIPFALL, true);
		m_pAnimDriver->Set_Param(PARAM_GRAPPLE_ZIP, false);

		pMovement->Launch(XMVectorSet(0.f, fUpSpeed, 0.f, 0.f));
	}

	End_Grapple();
	m_pGameInstance->Play_Shake(CAM_SHAKE_ZIP_RELEASE);
}

void CGrappleGun::Smash_Attack(LegionArmEvent eEvent)
{
	if (m_pGrappleTarget == nullptr || m_pGrappleTarget->Is_Dead() || m_pGrappleTarget->Is_InActive())
		return;

	_vector vPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	_vector vTargetPos = m_pGrappleTarget->Get_Transform()->Get_State(STATE::POSITION);

	_float fDist = XMVectorGetX(XMVector3Length(vPos - vTargetPos));
	if (fDist > Get_MinDistToTarget(SMASH_EXTRA)) return;

	HitInfo hitInfo{};
	hitInfo.pOtherActor = m_pOwner;
	hitInfo.pDamangeCauser = this;
	if (eEvent == LegionArmEvent::Grapple_Smash_Strike)
	{
		hitInfo.eHitType = HIT_TYPE::Normal;
		hitInfo.fDamage = SMASH_STRIKE_DAMAGE;
	}
	else if (eEvent == LegionArmEvent::Grapple_Smash_Jump)
	{
		hitInfo.eHitType = HIT_TYPE::KnockBack;
		hitInfo.fGroggy = SMASH_GROGGY;
		hitInfo.fDamage = SMASH_JUMP_DAMAGE;
	}
	else return;

	m_pGrappleTarget->Take_Damage(hitInfo);
}

void CGrappleGun::Request_Aim()
{
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_AIMING, true);
	m_pAnimDriver->Play_Action(AnimAction::Grapple_Aim);
}

void CGrappleGun::Enter_Aim()
{
	//애니 수락전에 착지했으면 조준모드 들어가지않음 
	if (m_pStatusCom->Is_Grounded())
	{
		m_pAnimDriver->Set_Param(PARAM_GRAPPLE_AIMING, false);
		return;
	}

	m_eState = GrappleState::Aim;
	m_fStateTime = 0.f;
	m_bAnimEntered = false;

	Set_CrosshairVisible(true);
	Set_AimSlow(true);

	_float fCamYaw = m_pSpringArm->Get_HorizonValue();
	_vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, fCamYaw, 0.f);

	m_pOwnerTransform->Set_Quat(vQuat);
	m_pOwner->Get_MovementComponent()->Set_RootMotionQuat(vQuat);

	Begin_ChangeView();

}

// Aim은 End_Grapple이 아닌 Exit_Aim로 종료함
void CGrappleGun::Update_Aim(_float fTimeDelta)
{
	m_fStateTime += fTimeDelta;

	//공중 조준 중 착지하면 종료
	if (m_pStatusCom->Is_Grounded())
	{
		Exit_Aim();
		return;
	}

	const _bool bAimAnimPlay = m_pAnimDriver->Is_ActionActive(AnimAction::Grapple_Aim);

	if (!m_bAnimEntered && bAimAnimPlay)
		m_bAnimEntered = true;

	if ((m_bAnimEntered && !bAimAnimPlay) || (!m_bAnimEntered && m_fStateTime > 1.f))
	{
		Exit_Aim();
		return;
	}

	Update_YawFollow(fTimeDelta);
}

void CGrappleGun::Exit_Aim()
{
	Set_AimSlow(false);
	m_pAnimDriver->Set_Param(PARAM_GRAPPLE_AIMING, false);

	Set_CrosshairVisible(false);

	m_pSpringArm->End_Adjust();
	m_eState = GrappleState::Idle;
}

void CGrappleGun::Set_AimSlow(_bool bOn)
{
	if (m_bSlowActive == bOn)
		return;

	m_pGameInstance->Set_MainTimeScale(bOn ? AIM_SLOW_SCALE : 1.f);
	//타임 스케일 무효화 
	m_pOwner->Set_GlobalTimeScale_Offset(bOn ? 1.f / AIM_SLOW_SCALE : 1.f);
	//낙하에만 적용함 
	m_pOwner->Get_MovementComponent()->Set_AirTimeScale(bOn ? AIM_SLOW_SCALE : 1.f);

	m_bSlowActive = bOn;
}

void CGrappleGun::Begin_ChangeView()
{
	CSpringArm::SPRING_ADJUST_DESC AdjustDesc = {};
	AdjustDesc.vLocalOffset = _float3(CAM_SWING_RIGHT_OFFSET, CAM_SWING_UP_OFFSET, 0.f);
	AdjustDesc.fBlendInTime = CAM_BLEND_IN_TIME;
	AdjustDesc.fBlendOutTime = CAM_BLEND_OUT_TIME;
	AdjustDesc.fArmLength = CAM_ARM_LENGTH;

	m_pSpringArm->Begin_Adjust(AdjustDesc);
}

CGrappleGun* CGrappleGun::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CGrappleGun* pInstance = new CGrappleGun(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CGrappleGun");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CGrappleGun::Free()
{
	__super::Free();
	Safe_Release(m_pGrappleTarget);
	m_pGrappleTarget = nullptr;
	Safe_Release(m_pWire);
}
