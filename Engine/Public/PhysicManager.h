#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class CPXDebugRenderer;

/* Simulation Event Callback */
class CSimulationCallback : public PxSimulationEventCallback
{
public:
    void Bind_TriggerCallback(std::function<void(PxTriggerPair*, PxU32)> func) { m_TriggerCallback = func; }
    void Bind_ContactCallback(std::function<void(const PxContactPairHeader&, const PxContactPair*, PxU32 )> func) { m_ContactCallback = func; }
    
    //트리거 충돌 발생
    void onTrigger(PxTriggerPair* pairs, PxU32 count) override
    {
        if (m_TriggerCallback) m_TriggerCallback(pairs, count);
    }
    //트리거 외 강체끼리 충돌 발생 
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 iCount) override
    {
        if (m_ContactCallback) m_ContactCallback(pairHeader, pairs, iCount);
    }

    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
    void onWake(PxActor** actors, PxU32 count) override {}
    void onSleep(PxActor** actors, PxU32 count) override {}
    void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
    
protected:
    function<void(PxTriggerPair*, PxU32)> m_TriggerCallback = { nullptr };
    function<void(const PxContactPairHeader&, const PxContactPair*, PxU32)> m_ContactCallback = { nullptr };
};

/* CCT QueryFilter */
class CCTQueryFilter : public PxQueryFilterCallback
{
public:
    // 충돌 정밀 검사 이전. filterData (주체의 쿼리 필터), Shape(대상)
    virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* pShape, const PxRigidActor* pActor, PxHitFlags& queryFlags) override
    {
        //트리거면 무시 
        if (pShape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE)) return PxQueryHitType::eNONE;

        const PxFilterData otherFilter = pShape->getQueryFilterData();


        if ((filterData.word1 & otherFilter.word0) || (filterData.word0 & otherFilter.word1))
        {
            return PxQueryHitType::eBLOCK;
        }
        
        return PxQueryHitType::eNONE;
    }

    // postFilter는 정밀검사 이후. 부딪힌 결과를 자세하게 알수있음. 무거움
    virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor)
    {
        return PxQueryHitType::eBLOCK;
    }
};

/* CCT BehaviorCallBack  */
 //CCT가 충돌했을때의 처리를 정의하는 콜백함수. 
class CCTBehaviorCallBack : public PxControllerBehaviorCallback
{
public:
	//Static이나 RidigDynamic에 충돌했을때
    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor) override
    {
        //디폴트 플래그 
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }
	//CCT와 CCT끼리 충돌했을때
    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxController& controller) override
    {
        return PxControllerBehaviorFlags(0);
    }
	//CCT와 Obstacle 충돌했을때
    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle& obstacle) override
    {
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }
};


/* Error Callback */
class CErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

/* Physic Manager */ 
class CPhysicManager : public CBase
{
    CPhysicManager();
	~CPhysicManager() = default;

public:
    PxScene*                Get_CurrentScene()              { return m_pCurrentScene; }

	PxCpuDispatcher*        Get_Default_CpuDispatcher()     { return m_pDefault_CpuDispatcher; }   
	PxTolerancesScale&      Get_Default_TolerancesScale()   { return m_Default_TolerancesScale; }  
	PxMaterial*             Get_Default_Material()          { return m_pDefaultMaterial; }

public:
    static void             Bind_TriggerCallback(function<void(PxTriggerPair*, PxU32)> func);
    static void             Bind_ContactCallback(function<void(const PxContactPairHeader&, const PxContactPair*, PxU32)> func);

    static PxQueryFilterCallback*  Get_CCTQueryFilter() { return &m_gCCTQueryFilter; }
	static PxControllerBehaviorCallback* Get_CCTBehaviorCallBack() { return &m_gCCTBehaviorCallBack; }

private:
    static                  CSimulationCallback m_gSimulationCallback;
    static                  CCTQueryFilter m_gCCTQueryFilter;
	static 				    CCTBehaviorCallBack m_gCCTBehaviorCallBack;
public:
    //PxDefaultMemoryOutputStream 
    bool                    Cook_TriangleMesh(PxTriangleMeshDesc& meshDesc, PxDefaultMemoryOutputStream& OutWriteBuffer);

    //필터링 수정했다면 해당 Actor에 대해 필터링 설정 reset해줘야함. 
    void                    Reset_Filtering(PxActor* pActor);
    //전체 컨트롤러 캐시 무효화 (맵 전환시 호출하여 Raw포인터 캐싱한것을 비워야함)
	void                    Invalidate_AllControllerCache();
public:
    //Filter Shader
    static PxFilterFlags    CustomFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
                                        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		                                PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);
public:
    PxScene*                Create_PxScene(const PxSceneDesc& desc);
    PxShape*                Create_PxShape(const PxGeometry& geometry, PxMaterial* material, bool isExclusive = false);
    PxMaterial*             Create_PxMaterial(float staticFriction, float dynamicFriction, float restitution);
    PxRigidDynamic*         Create_RigidDynamic(_fvector vInitPos);
    PxTriangleMesh*         Create_TriangleMesh(PxInputStream& stream);
    PxRigidStatic*          Create_RigidStatic(_fvector vInitPos, _fvector vInitQuat);
    PxController*           Create_CapsuleController(PxCapsuleControllerDesc& desc)
    {
        return m_pCurrentControllerManager->createController(desc);
    }
    PxController*           Create_BoxController(PxBoxControllerDesc& desc)
    {
        return m_pCurrentControllerManager->createController(desc);
    }

public:
    const _float            Get_StepTime() const { return m_fStepTime; }
    void                    Accumulate_Time(_float fTimeDelta);
    _bool                   Is_AccumulatedTime_Enough() const;
    void                    Start_Simulation();
    void                    Finish_Simulation();

    //현재 고정DT을 필요하는 개체가 없어서 한번만 시뮬 돌리게 변경함
    // TODO : 레그돌이나 움직이는 리지드바디가 생기면 다시 돌리기
	void                    Simulate_OneStep(_float fTimeDelta);


public:
    void                    Add_ActorToScene(PxActor* pActor);
    void                    Remove_ActorFromScene(PxActor* pActor);
    _bool                   Raycast(const XMFLOAT3& vRayOrigin, const XMFLOAT3& vRayDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
    _bool                   Sweep_Sphere(_float fRadius, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult , PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
    _bool                   Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
    //반지름, Half높이, 월드 시작위치, 회전, 방향, 거리, 필터링 그룹, 결과
    _bool                   Sweep_Capsule(_float fRadius,_float fHalfHeight, _fvector vOrigin, _fvector vQuat, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);
	_bool                   Overlap_Sphere(_float fRadius, _fvector vOrigin, FILTER_GROUP eTargetFilter, vector<CGameObject*>& OutResults, PxQueryFlags QueryFlags = PxQueryFlag::eDYNAMIC);

#ifdef _DEBUG
    //디버그용 시각화 데이터(콜라이더) 업데이트 
    HRESULT                 Create_DebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

    _bool                   Is_VisualDebugger_Enable() const { return m_pDebugRenderer != nullptr; }
    void                    Toggle_VisualDebugger();
    void                    VisualDebugger_Update();

    const PxRenderBuffer&   Get_RenderBuffer()const;

    void                    Apply_DebugVisible(_bool bVisible);
private:
    void                    On_VisualDebugger();
    void                    Off_VisualDebugger();
#endif //_DEBUG

private:
    HRESULT                 Initialize();

    //사용가능한 스레드 코어수를 반환함
    PxU32                   Get_OptimalPhysicsThreadCount();

private:
    PxFoundation*           m_pFoundation                   = { nullptr };
    PxPhysics*              m_pPhysics                      = { nullptr };
    PxScene*                m_pCurrentScene                 = { nullptr };
    PxControllerManager*    m_pCurrentControllerManager     = { nullptr };
    PxDefaultAllocator      m_DefaultAllocator              = {};
    
	PxMaterial*             m_pDefaultMaterial              = { nullptr };

private:
    CPXDebugRenderer*       m_pDebugRenderer                = { nullptr };   
    _bool                   m_bIsVisualDebuggerOn           = { false };

	_float                  m_fAccumulatedTime              = { };   

	_uint                   m_iStepCount                    = { 0 };   
	//const _uint             m_iMaxStepCount                 = { 5 };
    const _uint             m_iMaxStepCount                 = { 2 };
    const _float            m_fStepTime                     = { 1.f / 60.f };

    PxCpuDispatcher*        m_pDefault_CpuDispatcher        = { nullptr };
    PxTolerancesScale       m_Default_TolerancesScale;
    //누적시간 상한선
    const _float            m_fAccumulatedMaxTime           = { 2 * (1.f / 60.f) }; //5*

    //디버그 콜라이더 선 두께. 반드시 1이상주기
	const _float            m_fColliderLineWidth            = { 50.f };
public:
    static CPhysicManager* Create();
    virtual void           Free() override;

};

NS_END