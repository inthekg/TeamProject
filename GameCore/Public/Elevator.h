#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class CMapActivator;
class CPlayer;

class GAMECORE_DLL CElevator  final : public CMapGameplayObject
{
public :
	enum class ELEVATOR_STATE
	{
		READY,
		MOVING,
		ARRIVED,
		END
	};

    typedef struct tagElevatorDesc : public MAP_GAMEPLAY_OBJECT_DESC
    {
        // ElevatorSetting 파일에서 전달받는 인스턴스별 설정이다.
        _float3 vMoveOffset = { 0.f, 20.f, 0.f };
        _float fMoveSpeed = 2.f;
        _bool bStartAtEnd = false;

        // 모델 기준 발판 Collider다.
        _float3 vPlatformHalfExtents = { 2.f, 0.25f, 2.f };
        _float3 vPlatformLocalOffset = { 0.f, 0.f, 0.f };

        // 순간이동 시 발판 중심에서 적용할 모델 기준 착지 오프셋이다.
        _float3 vTransferLandingLocalOffset = { -0.0595316f, 0.f, -0.3439657f };

        // 발판 탑승 여부 확인용 Trigger다.
        _float3 vPassengerHalfExtents = { 1.8f, 1.5f, 1.8f };
        _float3 vPassengerLocalOffset = { 0.f, 1.5f, 0.f };
    } ELEVATOR_DESC;

private:
    CElevator(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CElevator(const CElevator& Prototype);
    virtual ~CElevator() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void Physical_Update(_float fFixedTime) override;
    virtual void Late_Update(_float fTimeDelta) override;

    virtual void Begin_Overlap(CGameObject* pOther) override;
    virtual void End_Overlap(CGameObject* pOther) override;

    // 탑승자 감지용 Trigger가 엘리베이터 자체를 상호작용 대상으로 만들지 않도록 막는다.
    virtual _bool Can_Interact(CGameObject* pPlayer) const override;

    virtual void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition) override;
public:
    HRESULT Start_MovingFromHeight(_float fControllerHeight);

    // 현재 엘리베이터 발판 위의 순간이동 위치와 현재 Yaw를 반환한다.
    HRESULT Get_TransferLandingPose(_float3* pOutPosition, _float* pOutYaw) const;

    void Attach_Controller(CMapActivator* pController);

    ELEVATOR_STATE Get_ElevatorState() const { return m_eState; }

private:
    void Ready_DefaultDesc(ELEVATOR_DESC* pDesc);
    HRESULT Ready_DynamicPhysics(const ELEVATOR_DESC& Desc);
    void Apply_InitialPosition();
    void Update_TargetPosition();
    void Reset_VisualInterpolation();
    void Finish_Moving();

    _bool Is_PassengerValid() const;

private:
    Engine::CDynamicRigidBody*          m_pDynamicRigidBody = { nullptr };
    CMapActivator*                      m_pAttachedController = { nullptr };
    CPlayer*                            m_pPassenger = { nullptr };

    _float3                             m_vMoveOffset = {};

    // 실제 PhysX 발판에 적용된 Scale 반영 크기와 오프셋이다.
    _float3                             m_vPlatformHalfExtents = {};
    _float3                             m_vPlatformLocalOffset = {};
    _float3                             m_vTransferLandingLocalOffset = {};

    _float3                             m_vStartPosition = {};
    _float3                             m_vEndPosition = {};
    _float3                             m_vTargetPosition = {};
    _float3                             m_vPhysicsPosition = {};
    _float                              m_fMoveSpeed = { 0.f };
    _float                              m_fVisualInterpolationSpeed = {18.f};
    _bool                               m_bStartAtEnd = { false };
    _bool                               m_bMovingToEnd = { false };
    _bool                               m_bPhysicsPositionReady = { false };

    ELEVATOR_STATE                      m_eState = { ELEVATOR_STATE::END };

public:
    static CElevator* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
