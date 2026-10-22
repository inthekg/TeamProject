#include "PinoLamp.h"
#include "GameInstance.h"
#include "Player.h"
#include "Model.h"
#include "Material.h"
#include "DynamicRigidBody.h"

CPinoLamp::CPinoLamp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CPartObject{ pDevice, pContext }
{
}

HRESULT CPinoLamp::Initialize(void* pArg)
{
	PINOLAMP_DESC* pDesc = static_cast<PINOLAMP_DESC*>(pArg);

	if (pDesc == nullptr || pDesc->pOwner == nullptr || pDesc->pSocketBoneMatrix == nullptr)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pOwner = pDesc->pOwner;
	m_pSocketBoneMatrix = pDesc->pSocketBoneMatrix;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (FAILED(Ready_Physics()))
		return E_FAIL;

	if (FAILED(Ready_PlayerLight()))
		return E_FAIL;

	return S_OK;
}

HRESULT CPinoLamp::Ready_Components()
{
	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), L"Prototype_Component_Model_Pino_Lamp",
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	m_pModelCom->Update_Bones();

	return S_OK;
}

HRESULT CPinoLamp::Ready_Physics()
{
	//Pino 본에 제약을 걸 앵커 
	m_pAnchorBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);
	if (m_pAnchorBody == nullptr) return E_FAIL;

	PHYSIC_DESC AnchorDesc{};
	AnchorDesc.pOwner = m_pOwner;
	AnchorDesc.bIsKinematic = true;
	AnchorDesc.iFilterWord0 = 0;
	AnchorDesc.iFilterWord1 = 0;

	SHAPE_DESC AnchorShapeDesc{};
	AnchorShapeDesc.eType = GEOMETRY_TYPE::SPHERE;
	AnchorShapeDesc.fRadius = 0.02f;
	AnchorShapeDesc.bTrigger = false;

	AnchorDesc.Shapes.push_back(AnchorShapeDesc);

	if (FAILED(m_pAnchorBody->Initialize(&AnchorDesc)))
		return E_FAIL;

	//m_pAnchorBody->Attach_ToBone(m_pSocketBoneMatrix, XMMatrixTranslation(0.f, 0.f, 0.f));

	/* Lamp 본체 */
	m_pLampBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);
	if (m_pLampBody == nullptr) return E_FAIL;

	PHYSIC_DESC LampDesc{};
	LampDesc.pOwner = m_pOwner;
	LampDesc.bIsKinematic = false;
	LampDesc.fMass = LAMP_MASS;
	LampDesc.iFilterWord0 = 0; 
	LampDesc.iFilterWord1 = 0;

	SHAPE_DESC LampShape{};
	LampShape.eType = GEOMETRY_TYPE::SPHERE;
	LampShape.fRadius = LAMP_RADIUS;
	LampShape.bTrigger = false;
	LampDesc.Shapes.push_back(LampShape);
	
	if (FAILED(m_pLampBody->Initialize(&LampDesc)))
		return E_FAIL;

	PxRigidDynamic* pLampActor = static_cast<PxRigidDynamic*>(m_pLampBody->Get_RigidActor());
	
	//감쇠 
	pLampActor->setLinearDamping(0.4f);
	//회전 감쇠
	pLampActor->setAngularDamping(5.f);//2

	/* 구면 Joint */
	//m_pJoint = PxSphericalJointCreate(PxGetPhysics(), m_pAnchorBody->Get_RigidActor(), PxTransform(PxIdentity), pLampActor, PxTransform(PxVec3(0.f, PIVOT_LOCAL_Y, 0.f)));
	//if (m_pJoint == nullptr)
	//{
	//	MSG_BOX("Pino Lamp : Failed to create spherical joint.");
	//	return E_FAIL;
	//}

	//아래setLimitCone는 X축 기준 제한임. 근데 램프의 경우 Y축이 기준이되어야함. 떄문에 기준축을 회전시킨다 . 
	// Z축 90도회전 시키켜서 X축이 Y축이 되도록 한다.
	//PxQuat qRot = PxQuat(XM_PIDIV2, PxVec3(0.f, 0.f, 1.f));
	//m_pJoint = PxSphericalJointCreate(PxGetPhysics(), m_pAnchorBody->Get_RigidActor(), PxTransform(PxVec3(0.f), qRot), pLampActor, PxTransform(PxVec3(0.f, PIVOT_LOCAL_Y, 0.f)));
	//if (m_pJoint == nullptr)
	//{
	//	MSG_BOX("Pino Lamp : Failed to create spherical joint.");
	//	return E_FAIL;
	//}
	//// 원뿔 제한 설정 . 이 이내로만 회전하도록 
	//m_pJoint->setLimitCone(PxJointLimitCone(PxReal(CONE_LIMIT_RAD), PxReal(CONE_LIMIT_RAD)));
	//// 제한 활성화
	//m_pJoint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
	PxQuat qRot = PxQuat(XM_PIDIV2, PxVec3(0.f, 0.f, 1.f));
	PxD6Joint* pD6Joint = PxD6JointCreate(PxGetPhysics(), m_pAnchorBody->Get_RigidActor(), PxTransform(PxVec3(0.f), qRot), pLampActor, PxTransform(PxVec3(0.f, PIVOT_LOCAL_Y, 0.f), qRot));
	pD6Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
	pD6Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
	pD6Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

	//Y축 회전 제한
	pD6Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
	//Z축 회전 제한
	pD6Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
	pD6Joint->setSwingLimit(PxJointLimitCone(PxReal(CONE_LIMIT_RAD), PxReal(CONE_LIMIT_RAD)));
	//x축 Twist 회전 제한(빙글빙글 도는거 방지) 
	pD6Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
	return S_OK;
}

HRESULT CPinoLamp::Ready_PlayerLight()
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	// 피노 램프 전용 포인트 라이트를 하나 추가해두고 위치만 매 프레임 따라가게 한다.
	m_iPlayerLightIndex = m_pGameInstance->Get_LightCount(LIGHT::POINT);
	m_bPlayerLightEnable = false;

	// 맵 라이트의 Intensity와 동일하게 색상에 광량을 곱해서 적용한다.
	const _float fIntensity = 3.f;

	m_PlayerLightDesc = {};
	m_PlayerLightDesc.eType = LIGHT::POINT;
	m_PlayerLightDesc.vDiffuse = _float4(1.f * fIntensity, 0.78f * fIntensity, 0.45f * fIntensity, 1.f);
	m_PlayerLightDesc.vAmbient = _float4(0.f, 0.f, 0.f, 0.15f);
	m_PlayerLightDesc.vSpecular = _float4(1.f * fIntensity, 0.78f * fIntensity, 0.45f * fIntensity, 1.f);
	m_PlayerLightDesc.vDirection = _float4(0.f, -1.f, 0.f, 0.f);
	m_PlayerLightDesc.vPosition = _float4(0.f, 0.f, 0.f, 1.f);
	m_PlayerLightDesc.fRange = 12.f;
	m_PlayerLightDesc.fInnerCos = cosf(XMConvertToRadians(20.f));
	m_PlayerLightDesc.fOuterCos = cosf(XMConvertToRadians(30.f));
	m_PlayerLightDesc.bShadow = false;
	m_PlayerLightDesc.bGodRay = false;
	m_PlayerLightDesc.fGodRayIntensity = 0.f;

	if (FAILED(m_pGameInstance->Add_Light(LIGHT::POINT, m_PlayerLightDesc)))
	{
		m_iPlayerLightIndex = UINT_MAX;
		return E_FAIL;
	}

	if (FAILED(m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPlayerLightIndex, false)))
		return E_FAIL;

	return S_OK;
}

void CPinoLamp::Update_PlayerLight(_fvector vLightPosition)
{
	if (nullptr == m_pGameInstance || UINT_MAX == m_iPlayerLightIndex || false == m_bPlayerLightEnable)
		return;

	// 피노의 등까지 밝히도록 램프 기준점에서 위로 올리고 바라보는 방향의 반대쪽으로 이동한다.
	const _vector vPlayerLook = XMVector3Normalize(m_pOwner->Get_Transform()->Get_State(STATE::LOOK));
	const _vector vLightOffset = XMVectorSet(0.f, PLAYER_LIGHT_UP_OFFSET, 0.f, 0.f) - vPlayerLook * PLAYER_LIGHT_BACK_OFFSET;
	const _vector vAdjustedLightPosition = XMVectorSetW(vLightPosition + vLightOffset, 1.f);

	XMStoreFloat4(&m_PlayerLightDesc.vPosition, vAdjustedLightPosition);
	m_pGameInstance->Set_LightDesc(LIGHT::POINT, m_iPlayerLightIndex, m_PlayerLightDesc);
}


void CPinoLamp::Set_LightEnable(_bool bEnable)
{
	if (nullptr == m_pGameInstance || UINT_MAX == m_iPlayerLightIndex)
		return;

	if (m_bPlayerLightEnable == bEnable)
		return;

	m_bPlayerLightEnable = bEnable;

	_matrix BoneWorld = XMLoadFloat4x4(m_pSocketBoneMatrix) * XMLoadFloat4x4(m_pOwner->Get_Transform()->Get_WorldMatrixPtr());
	_float3 vLightPos{};
	XMStoreFloat3(&vLightPos, BoneWorld.r[3]);
	m_pGameInstance->Play_Effect(EFFECT_LAMP, vLightPos);
	m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPlayerLightIndex, m_bPlayerLightEnable);

	if (m_iSoundHandle != -1)
	{
		m_pGameInstance->Stop_SoundSource(m_iSoundHandle);
		m_iSoundHandle = -1;
	}

	m_iSoundHandle = m_pGameInstance->Play_Cue(SOUND_LAMP, CHANNEL_ID::SOUND_PLAYER, vLightPos);
}

void CPinoLamp::Physical_Update(_float fFixedTime)
{
	if (!m_pAnchorBody || !m_pSocketBoneMatrix || !m_pOwner) return;

	//_matrix vTranlation = XMMatrixTranslation(1.f, -7.f, -3.f); // Local Offset
	_matrix BoneWorld = XMLoadFloat4x4(m_pSocketBoneMatrix) * XMLoadFloat4x4(m_pOwner->Get_Transform()->Get_WorldMatrixPtr());
	// 본 위치만 적용함
	_vector vBonePos = BoneWorld.r[3];
	m_pAnchorBody->Set_KinematicTarget(vBonePos, XMQuaternionIdentity());
}


void CPinoLamp::Late_Update(_float fTimeDelta)
{
	if (m_pLampBody == nullptr || m_pAnchorBody == nullptr)
		return;

	_matrix BoneWorld = XMLoadFloat4x4(m_pSocketBoneMatrix) * XMLoadFloat4x4(m_pOwner->Get_Transform()->Get_WorldMatrixPtr());
	_vector vBonePos = BoneWorld.r[3];

	_vector vAnchorPos = {}, vAnchorQuat = {};
	_vector vLampPos = {}, vLampQuat = {};

	m_pAnchorBody->Get_GlobalTransform(vAnchorPos, vAnchorQuat);
	m_pLampBody->Get_GlobalTransform(vLampPos, vLampQuat);

	_matrix Rot = XMMatrixIdentity();
	_vector vHangDir = XMVectorSubtract(vLampPos, vAnchorPos);
	_float fDist = XMVectorGetX(XMVector3LengthSq(vHangDir));
	//if (fDist > 0.f)
	//{
	//	vHangDir = XMVector3Normalize(vHangDir);
	//	_vector vDown = XMVectorSet(0.f, -1.f, 0.f, 0.f);
	//	_float fDot = max(-1.f, min(1.f, XMVectorGetX(XMVector3Dot(vDown, vHangDir))));
	//	if (fDot < 0.9999f)
	//	{
	//		_vector vAxis = XMVector3Cross(vDown, vHangDir);
	//		if(XMVectorGetX(XMVector3LengthSq(vAxis)) > 0.f)
	//			Rot = XMMatrixRotationAxis(XMVector3Normalize(vAxis), acosf(fDot));
	//	}
	//}

	//두 강체가 차이가 벌어지면 보정 ( Teleport 방지 ) 
	if (fDist > DIST_DIFF_SQRT)
	{
		vLampPos = vAnchorPos - XMVectorSet(0.f, PIVOT_LOCAL_Y, 0.f, 0.f);
		vLampQuat = vAnchorQuat;
		m_pLampBody->Set_GlobalTransform(vLampPos, vLampQuat);

		PxRigidDynamic* pLampActor = static_cast<PxRigidDynamic*>(m_pLampBody->Get_RigidActor());
		pLampActor->setLinearVelocity(PxVec3(0.f));
		pLampActor->setAngularVelocity(PxVec3(0.f));
	}

	//Anchor의 위치 + 램프의 회전으로 월드 셋
	_matrix WorldMatrix = XMMatrixRotationQuaternion(vLampQuat);
	//_matrix WorldMatrix = Rot;
	WorldMatrix.r[3] = XMVectorSetW(vBonePos, 1.f);

	XMStoreFloat4x4(&m_CombinedWorldMatrix, WorldMatrix);
	Update_PlayerLight(vAnchorPos);

	if (m_bVisibility)
	{
		m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
		//m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
	}
	
}

HRESULT CPinoLamp::Render()
{
	if (!m_pModelCom)
	{
		MSG_BOX("CPinoLamp : Render_Anim Failed. Model is nullptr");
		return E_FAIL;
	}

	const auto& units = m_pModelCom->Get_RenderUnits();
	_uint iCount = static_cast<_uint>(units.size());

	_matrix WorldMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix()) * XMLoadFloat4x4(&m_CombinedWorldMatrix);
	for (_uint i = 0; i < iCount; i++)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;

		if (pMesh == nullptr || pMaterialCom == nullptr)
		{
			MSG_BOX("CBodyPart : Render_Anim Failed");
			return E_FAIL;
		}

		StaticCommand command{};
		command.header.etype = RENDER_TYPE::STATIC;
		command.header.iCommandSize = sizeof(StaticCommand);
		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.fEmissiveAmount = 300.f;
		XMStoreFloat4x4(&command.worldMatrix, WorldMatrix);

		command.iAlphaClip = pMaterialCom->Get_OpacityMaskSource();

		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;

		m_pGameInstance->Submit_Static_Resolved(command);

	}

	return S_OK;
}

void CPinoLamp::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	m_fDissolveAmount = fAmount;
	m_vDissolveColor = vColor;
}

CPinoLamp* CPinoLamp::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CPinoLamp* pInstance = new CPinoLamp(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to create Pino Lamp");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CPinoLamp::Free()
{
	__super::Free();

	if (nullptr != m_pGameInstance && UINT_MAX != m_iPlayerLightIndex)
	{
		// 맵 라이트 인덱스가 밀리지 않게 제거하지 않고 비활성화만 한다.
		m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPlayerLightIndex, false);
		m_iPlayerLightIndex = UINT_MAX;
	}

	if (m_pJoint)
	{
		m_pJoint->release();
		m_pJoint = nullptr;
	}

	Safe_Release(m_pAnchorBody);
	Safe_Release(m_pLampBody);
	Safe_Release(m_pModelCom);
}
