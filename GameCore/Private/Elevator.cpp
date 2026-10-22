#include "Elevator.h"

#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "MapActivator.h"
#include "Player.h"
#include "Transform.h"

CElevator::CElevator(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMapGameplayObject{ pDevice, pContext }
{
}

CElevator::CElevator(const CElevator& Prototype)
    : CMapGameplayObject{ Prototype }
{
}

HRESULT CElevator::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CElevator::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    ELEVATOR_DESC Desc = *static_cast<ELEVATOR_DESC*>(pArg);
    Ready_DefaultDesc(&Desc);

    if (false == std::isfinite(Desc.fMoveSpeed) || Desc.fMoveSpeed <= 0.f)
        return E_FAIL;

    if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&Desc.vMoveOffset))) <= FLT_EPSILON)
        return E_FAIL;

    m_vMoveOffset = Desc.vMoveOffset;
    m_fMoveSpeed = Desc.fMoveSpeed;
    m_bStartAtEnd = Desc.bStartAtEnd;

    const _float3 vAbsScale = _float3(fabsf(Desc.vScale.x), fabsf(Desc.vScale.y), fabsf(Desc.vScale.z));

    // PhysX 발판에 실제 적용되는 크기와 오프셋을 보관한다.
    m_vPlatformHalfExtents = _float3(Desc.vPlatformHalfExtents.x * vAbsScale.x, Desc.vPlatformHalfExtents.y * vAbsScale.y, Desc.vPlatformHalfExtents.z * vAbsScale.z);
    m_vPlatformLocalOffset = _float3(Desc.vPlatformLocalOffset.x * vAbsScale.x, Desc.vPlatformLocalOffset.y * vAbsScale.y, Desc.vPlatformLocalOffset.z * vAbsScale.z);
    m_vTransferLandingLocalOffset = _float3(Desc.vTransferLandingLocalOffset.x * vAbsScale.x, Desc.vTransferLandingLocalOffset.y * vAbsScale.y, Desc.vTransferLandingLocalOffset.z * vAbsScale.z);

    // 부모의 StaticRigidBody는 생성하지 않는다.
    ELEVATOR_DESC BaseDesc = Desc;
    BaseDesc.bUsePhysics = false;

    if (FAILED(__super::Initialize(&BaseDesc)))
        return E_FAIL;

    Apply_InitialPosition();
    Update_TargetPosition();

    if (true == Desc.bPhysicsStartRegistered && FAILED(Ready_DynamicPhysics(Desc)))
        return E_FAIL;

    Reset_VisualInterpolation();

    m_eState = ELEVATOR_STATE::READY;

    return S_OK;
}

void CElevator::Physical_Update(_float fFixedTime)
{
    __super::Physical_Update(fFixedTime);

    if (ELEVATOR_STATE::MOVING != m_eState || nullptr == m_pTransformCom || nullptr == m_pDynamicRigidBody)
        return;

    if (false == m_bPhysicsPositionReady)
        Reset_VisualInterpolation();

    _vector vCurrentPosition = XMLoadFloat3(&m_vPhysicsPosition);
    _vector vTargetPosition = XMLoadFloat3(&m_vTargetPosition);
    _vector vToTarget = vTargetPosition - vCurrentPosition;

    const _float fRemainDistance = XMVectorGetX(XMVector3Length(vToTarget));

    if (fRemainDistance <= FLT_EPSILON)
    {
        Finish_Moving();
        return;
    }

    const _float fMoveDistance = min(m_fMoveSpeed * fFixedTime, fRemainDistance);
    _vector vDisplacement = XMVector3Normalize(vToTarget) * fMoveDistance;
    _vector vNextPosition = vCurrentPosition + vDisplacement;
    _vector vQuaternion = m_pTransformCom->Get_Quat();

    // PhysX Target 설정에 실패하면 모델과 탑승자를 먼저 이동시키지 않는다.
    if (FAILED(m_pDynamicRigidBody->Set_KinematicTarget(vNextPosition, vQuaternion)))
        return;

    XMStoreFloat3(&m_vPhysicsPosition, vNextPosition);

    // 물리 발판과 렌더 모델이 동일한 위치를 사용하도록 맞춘다.
    m_pTransformCom->Set_State(STATE::POSITION, vNextPosition);

    // 수직 이동은 PhysX의 자동 Ride 처리에 의존하지 않고 탑승자를 직접 운반한다.
    if (true == Is_PassengerValid())
        m_pPassenger->Apply_ExternalDisplacement(vDisplacement);

    // Controller 모델만 발판과 함께 이동하고 위/아래 외부 Trigger Box는 월드 위치에 고정한다.
    if (nullptr != m_pAttachedController && false == m_pAttachedController->Is_InActive())
        m_pAttachedController->Apply_AttachedDisplacement(vDisplacement);

    if (fMoveDistance >= fRemainDistance)
        Finish_Moving();
}


void CElevator::Late_Update(_float fTimeDelta)
{
    if (nullptr != m_pTransformCom && true == m_bPhysicsPositionReady)
    {
        _vector vRenderPosition = m_pTransformCom->Get_State(STATE::POSITION);
        _vector vPhysicsPosition = XMLoadFloat3(&m_vPhysicsPosition);
        _vector vDifference = vPhysicsPosition - vRenderPosition;

        _float fSafeTimeDelta = fTimeDelta > 0.f ? fTimeDelta : 0.f;
        _float fInterpolationRatio = 1.f - expf(-m_fVisualInterpolationSpeed * fSafeTimeDelta);

        if (XMVectorGetX(XMVector3LengthSq(vDifference)) <= 0.000001f)
            m_pTransformCom->Set_State(STATE::POSITION, vPhysicsPosition);
        else
            m_pTransformCom->Set_State(STATE::POSITION, XMVectorLerp(vRenderPosition, vPhysicsPosition, fInterpolationRatio));
    }

    __super::Late_Update(fTimeDelta);
}



void CElevator::Ready_DefaultDesc(ELEVATOR_DESC* pDesc)
{
    if (nullptr == pDesc)
        return;

    pDesc->eVisualType = VISUAL_TYPE::STATIC;
    pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Elevator") : pDesc->strModelPrototypeTag;
    pDesc->bUsePhysics = false;
}

HRESULT CElevator::Ready_DynamicPhysics(const ELEVATOR_DESC& Desc)
{
    if (nullptr != m_pDynamicRigidBody)
        return S_OK;

    if (nullptr == m_pTransformCom)
        return E_FAIL;

    m_pDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

    if (nullptr == m_pDynamicRigidBody)
        return E_FAIL;

    PHYSIC_DESC PhysicDesc{};
    PhysicDesc.pOwner = this;
    PhysicDesc.bIsKinematic = true;
    PhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::GROUND);
    PhysicDesc.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

    XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
    XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

    const _float3 vAbsScale = _float3(fabsf(Desc.vScale.x), fabsf(Desc.vScale.y), fabsf(Desc.vScale.z));

    SHAPE_DESC PlatformShape{};
    PlatformShape.eType = GEOMETRY_TYPE::BOX;
    PlatformShape.vHalfExtents = m_vPlatformHalfExtents;
    PlatformShape.vLocalOffset = m_vPlatformLocalOffset;
    PlatformShape.bTrigger = false;
    PhysicDesc.Shapes.push_back(PlatformShape);

    SHAPE_DESC PassengerShape{};
    PassengerShape.eType = GEOMETRY_TYPE::BOX;
    PassengerShape.vHalfExtents = _float3(Desc.vPassengerHalfExtents.x * vAbsScale.x, Desc.vPassengerHalfExtents.y * vAbsScale.y, Desc.vPassengerHalfExtents.z * vAbsScale.z);
    PassengerShape.vLocalOffset = _float3(Desc.vPassengerLocalOffset.x * vAbsScale.x, Desc.vPassengerLocalOffset.y * vAbsScale.y, Desc.vPassengerLocalOffset.z * vAbsScale.z);
    PassengerShape.bTrigger = true;
    PhysicDesc.Shapes.push_back(PassengerShape);

    if (FAILED(m_pDynamicRigidBody->Initialize(&PhysicDesc)))
    {
        Safe_Release(m_pDynamicRigidBody);
        return E_FAIL;
    }
    return S_OK;
}

void CElevator::Apply_InitialPosition()
{
    if (false == m_bStartAtEnd || nullptr == m_pTransformCom)
        return;

    _vector vStartPosition = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vEndPosition = vStartPosition + XMLoadFloat3(&m_vMoveOffset);

    m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vEndPosition, 1.f));
}

void CElevator::Update_TargetPosition()
{
    if (nullptr == m_pTransformCom)
        return;

    _vector vCurrentPosition = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vMoveOffset = XMLoadFloat3(&m_vMoveOffset);
    _vector vStartPosition = true == m_bStartAtEnd ? vCurrentPosition - vMoveOffset : vCurrentPosition;
    _vector vEndPosition = vStartPosition + vMoveOffset;

    // 왕복할 두 끝점을 고정해서 반복 이동 시 오차 누적을 방지한다.
    XMStoreFloat3(&m_vStartPosition, vStartPosition);
    XMStoreFloat3(&m_vEndPosition, vEndPosition);

    // 끝에서 시작하면 첫 목적지는 시작점이고, 시작점에서 시작하면 첫 목적지는 끝점이다.
    m_bMovingToEnd = false == m_bStartAtEnd;
    m_vTargetPosition = true == m_bMovingToEnd ? m_vEndPosition : m_vStartPosition;
}

void CElevator::Reset_VisualInterpolation()
{
    if (nullptr == m_pTransformCom)
    {
        m_bPhysicsPositionReady = false;
        return;
    }

    // 현재 Transform 위치를 최초 물리 위치로 사용합니다.
    XMStoreFloat3(&m_vPhysicsPosition, m_pTransformCom->Get_State(STATE::POSITION));
    m_bPhysicsPositionReady = true;
}

void CElevator::Finish_Moving()
{
    m_eState = ELEVATOR_STATE::ARRIVED;

    // 도착한 뒤 단일 엘리베이터 컨트롤러를 다시 사용할 수 있도록 초기 상태로 돌린다.
    if (nullptr != m_pAttachedController && false == m_pAttachedController->Is_InActive())
        m_pAttachedController->Reset_ForReuse();
}

HRESULT CElevator::Start_MovingFromHeight(_float fControllerHeight)
{
    if (false == std::isfinite(fControllerHeight))
        return E_FAIL;

    // 이동 중 입력은 목적지를 바꾸지 않고, 도착 시 단일 컨트롤러를 초기화한다.
    if (ELEVATOR_STATE::MOVING == m_eState)
        return S_OK;

    if (ELEVATOR_STATE::READY != m_eState && ELEVATOR_STATE::ARRIVED != m_eState)
        return S_OK;

    if (nullptr == m_pDynamicRigidBody || nullptr == m_pDynamicRigidBody->Get_RigidActor() || nullptr == m_pDynamicRigidBody->Get_RigidActor()->getScene())
        return E_FAIL;

    const _float fDistanceToStart = fabsf(fControllerHeight - m_vStartPosition.y);
    const _float fDistanceToEnd = fabsf(fControllerHeight - m_vEndPosition.y);

    // 아래쪽 Trigger는 End(위), 위쪽 Trigger는 Start(아래)를 목적지로 선택한다.
    m_bMovingToEnd = fDistanceToStart <= fDistanceToEnd;
    m_vTargetPosition = true == m_bMovingToEnd ? m_vEndPosition : m_vStartPosition;

    const _vector vCurrentPosition = XMLoadFloat3(&m_vPhysicsPosition);
    const _vector vTargetPosition = XMLoadFloat3(&m_vTargetPosition);

    // 이미 호출한 층에 도착해 있으면 이동 없이 컨트롤러 애니메이션만 원상 복구한다.
    if (XMVectorGetX(XMVector3LengthSq(vTargetPosition - vCurrentPosition)) <= 0.000001f)
    {
        if (nullptr != m_pAttachedController && false == m_pAttachedController->Is_InActive())
            m_pAttachedController->Reset_ForReuse();

        return S_OK;
    }

    m_eState = ELEVATOR_STATE::MOVING;
    return S_OK;
}

HRESULT CElevator::Get_TransferLandingPose(_float3* pOutPosition, _float* pOutYaw) const
{
    if (nullptr == pOutPosition || nullptr == pOutYaw || nullptr == m_pTransformCom || nullptr == m_pDynamicRigidBody)
        return E_FAIL;

    PxRigidActor* pRigidActor = m_pDynamicRigidBody->Get_RigidActor();

    if (nullptr == pRigidActor || nullptr == pRigidActor->getScene())
        return E_FAIL;

    // 목적지 Elevator의 현재 자세를 기준으로 지정된 발판 위 착지 지점을 계산한다.
    _vector vLocalLandingPosition = XMVectorSet(m_vPlatformLocalOffset.x + m_vTransferLandingLocalOffset.x, m_vPlatformLocalOffset.y + m_vPlatformHalfExtents.y + m_vTransferLandingLocalOffset.y, m_vPlatformLocalOffset.z + m_vTransferLandingLocalOffset.z, 0.f);
    _vector vWorldLandingPosition = XMVector3Rotate(vLocalLandingPosition, m_pTransformCom->Get_Quat());
    vWorldLandingPosition += m_pTransformCom->Get_State(STATE::POSITION);

    _vector vCurrentLook = m_pTransformCom->Get_State(STATE::LOOK);

    if (XMVectorGetX(XMVector3LengthSq(vCurrentLook)) <= FLT_EPSILON)
        return E_FAIL;

    XMStoreFloat3(pOutPosition, vWorldLandingPosition);

    // Respawn_Point의 Yaw는 Radian이므로 현재 Elevator 방향을 그대로 사용한다.
    *pOutYaw = atan2f(XMVectorGetX(vCurrentLook), XMVectorGetZ(vCurrentLook));

    return S_OK;
}

void CElevator::Attach_Controller(CMapActivator* pController)
{
    if (m_pAttachedController == pController)
        return;

    m_pAttachedController = pController;

    if (true == m_bStartAtEnd && nullptr != m_pAttachedController)
        m_pAttachedController->Apply_AttachedInitialDisplacement(XMLoadFloat3(&m_vMoveOffset));
}



void CElevator::Begin_Overlap(CGameObject* pOther)
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(pOther);

    if (nullptr != pPlayer)
        m_pPassenger = pPlayer;
}

void CElevator::End_Overlap(CGameObject* pOther)
{
    if (pOther == m_pPassenger)
        m_pPassenger = nullptr;
}

_bool CElevator::Can_Interact(CGameObject* pPlayer) const
{
    // 엘리베이터의 Trigger는 탑승자 운반 확인용이며 E 상호작용 대상이 아니다.
    return false;
}

_bool CElevator::Is_PassengerValid() const
{
    return nullptr != m_pPassenger && false == m_pPassenger->Is_InActive() && nullptr != m_pPassenger->Get_CCTComponent();
}

void CElevator::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
    __super::Set_MapTransform(vScale, vRotationDegree, vPosition);

    Apply_InitialPosition();
    Update_TargetPosition();

    if (nullptr != m_pDynamicRigidBody && nullptr != m_pTransformCom)
        m_pDynamicRigidBody->Set_GlobalTransform(m_pTransformCom->Get_State(STATE::POSITION), m_pTransformCom->Get_Quat());

    Reset_VisualInterpolation();

    m_eState = ELEVATOR_STATE::READY;
}

CElevator* CElevator::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CElevator* pInstance = new CElevator(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CElevator");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CElevator::Clone(void* pArg)
{
    CElevator* pInstance = new CElevator(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CElevator");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CElevator::Free()
{
    m_pPassenger = nullptr;
    m_pAttachedController = nullptr;

    Safe_Release(m_pDynamicRigidBody);

    __super::Free();
}
