#include "MapGameplayObject.h"

#include "Core_Struct.h"
#include "GameInstance.h"
#include "Animator.h"
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "Bone.h"
#include "RenderStruct.h"
#include "StaticRigidBody.h"

CMapGameplayObject::CMapGameplayObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CMapGameplayObject::CMapGameplayObject(const CMapGameplayObject& Prototype)
	: CGameObject{ Prototype }
	, m_bLoopDefaultClip{ Prototype.m_bLoopDefaultClip }
	, m_bInteractionEnabled{ Prototype.m_bInteractionEnabled }
	, m_strLoopDefaultClip{ Prototype.m_strLoopDefaultClip }
{
}

HRESULT CMapGameplayObject::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CMapGameplayObject::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMapGameplayObject");
		return E_FAIL;
	}

	MAP_GAMEPLAY_OBJECT_DESC* pDesc = static_cast<MAP_GAMEPLAY_OBJECT_DESC*>(pArg);

	if (VISUAL_TYPE::END == pDesc->eVisualType)
		return E_FAIL;

	m_eVisualType = pDesc->eVisualType;
	m_bApplyRootMotion = pDesc->bApplyRootMotion;
	m_bLoopDefaultClip = pDesc->bLoopDefaultClip;
	m_bInteractionEnabled = pDesc->bInteractionEnabled;
	m_strLoopDefaultClip = pDesc->bLoopDefaultClip ? pDesc->strDefaultClip : TEXT("");

	// Manager가 액터별로 지정한 상호작용 로컬 위치와 고정 회전값을 보관한다.
	m_bUseInteractPosition = pDesc->bUseInteractPosition;
	m_vInteractLocalOffsetXZ = pDesc->vInteractLocalOffsetXZ;
	m_fInteractLocalYaw = pDesc->fInteractLocalYaw;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Visual(*pDesc)))
		return E_FAIL;

	if (FAILED(Ready_Physics(*pDesc)))
		return E_FAIL;

	if (VISUAL_TYPE::SKINNED == m_eVisualType && nullptr != m_pAnimator)
	{
		m_pAnimator->Set_ControllerEnabled(pDesc->bControllerEnabled);

		if (true == pDesc->bPlayDefault && FAILED(Play_InitialAnimation(*pDesc)))
			return E_FAIL;
	}

	return S_OK;
}

void CMapGameplayObject::Deactivate_LinkedTriggers()
{
	if (m_Func_DeactivateLinkedTriggers)
		m_Func_DeactivateLinkedTriggers();
}

void CMapGameplayObject::Configure_InteractionEvent(EVENT_TYPE_MAP_OBJECT eEventType, const string& strAssetName, _bool bPlayOnce)
{
	m_eInteractionEvent = eEventType;
	m_strInteractionCutSceneAssetName = strAssetName;
	m_bInteractionEventPlayOnce = bPlayOnce;
	m_bInteractionEventConsumed = false;
}

void CMapGameplayObject::Dispatch_InteractionEvent()
{
	if (nullptr == m_pGameInstance || EVENT_TYPE_MAP_OBJECT::END == m_eInteractionEvent)
		return;

	if (true == m_bInteractionEventPlayOnce && true == m_bInteractionEventConsumed)
		return;

	CUTSCENE_REQUEST Request{};
	Request.iTargetSpawnID = m_iSpawnID;
	Request.strAssetName = m_strInteractionCutSceneAssetName;
	Request.bPlayOnce = m_bInteractionEventPlayOnce;
	Request.hResult = E_FAIL;

	// Player가 Interact 성공을 확인한 뒤 호출하며 EventBus 수신자가 같은 프레임에 재생 결과를 기록한다.
	m_pGameInstance->Broadcast(m_eInteractionEvent, &Request);

	if (SUCCEEDED(Request.hResult) && true == m_bInteractionEventPlayOnce)
		m_bInteractionEventConsumed = true;
}

void CMapGameplayObject::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CMapGameplayObject::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	// 게임플레이 로직은 Static/Skinned 여부와 관계없이 실행한다.
	Update_MapGameplayObject(fTimeDelta);

	// 범용 장식물은 별도 파생 클래스 없이 기본 Clip만 반복할 수 있다.
	if (true == m_bLoopDefaultClip && false == m_strLoopDefaultClip.empty() && true == Is_AnimationFinished())
		Play_AnimClip(m_strLoopDefaultClip);

	if (VISUAL_TYPE::SKINNED == m_eVisualType)
		Drive_Animator(fTimeDelta);
}

void CMapGameplayObject::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMapGameplayObject::Late_Update(_float fTimeDelta)
{
	// Skinned에서만 현재 Pose를 Bone에 반영한다.
	if (VISUAL_TYPE::SKINNED == m_eVisualType && nullptr != m_pModelCom && nullptr != m_pAnimator)
		m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	__super::Late_Update(fTimeDelta);

	if (VISUAL_TYPE::NONE != m_eVisualType && nullptr != m_pModelCom)
		m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CMapGameplayObject::Render()
{
	switch (m_eVisualType)
	{
	case VISUAL_TYPE::NONE:
		return S_OK;

	case VISUAL_TYPE::STATIC:
		return Render_Static(true);

	case VISUAL_TYPE::SKINNED:
		return Render_Skinned(true);

	default:
		return E_FAIL;
	}
}

HRESULT CMapGameplayObject::Render_Shadow()
{
	switch (m_eVisualType)
	{
	case VISUAL_TYPE::NONE:
		return S_OK;

	case VISUAL_TYPE::STATIC:
		return Render_Static(false);

	case VISUAL_TYPE::SKINNED:
		return Render_Skinned(false);

	default:
		return E_FAIL;
	}
}

HRESULT CMapGameplayObject::Render_Static(_bool bCheckMaterialPass)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return S_OK;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	const RENDERID eCurrentPass = m_pGameInstance->Get_CurRenderPass();

	// Static 모델은 PreTransform을 World 앞에 적용한다.
	_matrix WorldMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix()) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());

	for (_uint i = 0; i < static_cast<_uint>(Units.size()); ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterial = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterial)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

		if (true == bCheckMaterialPass && pMaterial->Get_RenderGroup() != eCurrentPass)
			continue;

		StaticCommand Command{};
		Command.header.etype = RENDER_TYPE::STATIC;
		Command.header.iCommandSize = sizeof(StaticCommand);
		Command.pVIBuffer = pMesh;
		Command.pMaterial = pMaterial;

		XMStoreFloat4x4(&Command.worldMatrix, WorldMatrix);

		// 파생 객체가 디졸브처럼 객체별 렌더 값을 추가할 수 있도록 제출 직전에 호출한다.
		Configure_RenderCommand(&Command);

		m_pGameInstance->Submit_Static_Resolved(Command);
	}

	return S_OK;
}

HRESULT CMapGameplayObject::Render_Skinned(_bool bCheckMaterialPass)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return S_OK;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	const RENDERID eCurrentPass = m_pGameInstance->Get_CurRenderPass();
	const _float4x4* pWorldMatrix = m_pTransformCom->Get_WorldMatrixPtr();

	for (_uint i = 0; i < static_cast<_uint>(Units.size()); ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterial = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterial)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

		if (true == bCheckMaterialPass && pMaterial->Get_RenderGroup() != eCurrentPass)
			continue;

		SkinnedCommand Command{};
		Command.header.etype = RENDER_TYPE::SKINNED;
		Command.header.iCommandSize = sizeof(SkinnedCommand);
		Command.iBoneCount = pMesh->Get_BoneCount();
		Command.pVIBuffer = pMesh;
		Command.pMaterial = pMaterial;
		Command.worldMatrix = *pWorldMatrix;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(Command, i)))
			continue;

		// Static/Skinned가 동일한 객체 렌더 설정 흐름을 사용한다.
		Configure_RenderCommand(&Command);

		m_pGameInstance->Submit_Skinned_Resolved(Command);
	}

	return S_OK;
}

void CMapGameplayObject::Configure_RenderCommand(RenderCommand* pCommand) const
{
	if (nullptr == pCommand)
		return;

	// 모든 MapGameplayObject가 사용하는 공통 객체 렌더 값을 한 곳에서 설정한다.
	pCommand->iAlphaClip = 0;
	pCommand->fEmissiveAmount = m_fEmissiveAmount;
	pCommand->iRimProfile = m_iRimProfile;
	pCommand->fRimAmount = m_fRimAmount;
}

void CMapGameplayObject::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
	if (nullptr == m_pTransformCom)
		return;

	m_pTransformCom->Set_Scale(vScale);

	_vector vRotationQuaternion = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(vRotationDegree.x), XMConvertToRadians(vRotationDegree.y), XMConvertToRadians(vRotationDegree.z));

	m_pTransformCom->Set_Quat(vRotationQuaternion);
	m_pTransformCom->Translate(XMVectorSet(vPosition.x, vPosition.y, vPosition.z, 1.f));
}

HRESULT CMapGameplayObject::Play_AnimClip(const _wstring& strClipName)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator || strClipName.empty())
		return E_FAIL;

	return m_pAnimator->Play_Clip(Hash_String(strClipName.c_str()));
}

HRESULT CMapGameplayObject::Play_AnimSequence(const _wstring& strSequenceName)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator || strSequenceName.empty())
		return E_FAIL;

	return m_pAnimator->Play_Sequence(Hash_String(strSequenceName.c_str()));
}

void CMapGameplayObject::Request_AnimState(const _wstring& strStateName)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Request_State(strStateName);
}

void CMapGameplayObject::Request_AnimAction(const _wstring& strActionName)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Request_Action(strActionName);
}

void CMapGameplayObject::Request_AnimSequenceEnd()
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Request_SequenceEnd();
}

void CMapGameplayObject::Set_AnimBool(const _wstring& strKey, _bool bValue)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Set_Bool(strKey, bValue);
}

void CMapGameplayObject::Set_AnimInt(const _wstring& strKey, _int iValue)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Set_Int(strKey, iValue);
}

void CMapGameplayObject::Set_AnimFloat(const _wstring& strKey, _float fValue)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Set_Float(strKey, fValue);
}

void CMapGameplayObject::Set_AnimTrigger(const _wstring& strKey)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Set_Trigger(strKey);
}

_bool CMapGameplayObject::Is_AnimationFinished() const
{
	// Static은 애니메이션이 없으므로 완료된 것으로 취급한다.
	if (VISUAL_TYPE::SKINNED != m_eVisualType)
		return true;

	return nullptr == m_pAnimator ? true : m_pAnimator->Is_Ended();
}

_bool CMapGameplayObject::Is_AnimInState(const _wstring& strStateName) const
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return false;

	const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();

	if (nullptr == pState)
		return false;

	return pState->iNameHash == Hash_String(strStateName.c_str());
}

_bool CMapGameplayObject::Was_AnimStateChanged() const
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return false;

	return m_pAnimator->Was_StateChangedThisFrame();
}

_bool CMapGameplayObject::Set_MeshVisible(_uint iMeshIndex, _bool bVisible)
{
	if (iMeshIndex >= m_MeshVisibles.size())
		return false;

	m_MeshVisibles[iMeshIndex] = bVisible;

	return true;
}

HRESULT CMapGameplayObject::Change_Visual(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (VISUAL_TYPE::END == Desc.eVisualType)
		return E_FAIL;

	// 기존 Animator를 정리한다.
	Safe_Release(m_pAnimator);

	// Component Map이 가진 Model 참조를 정리한다.
	auto Iter = m_Components.find(TEXT("Com_Model"));

	if (m_Components.end() != Iter)
	{
		Safe_Release(Iter->second);
		m_Components.erase(Iter);
	}

	// 멤버가 가진 Model 참조를 정리한다.
	Safe_Release(m_pModelCom);

	m_MeshVisibles.clear();

	m_eVisualType = Desc.eVisualType;
	m_bApplyRootMotion = Desc.bApplyRootMotion;

	if (FAILED(Ready_Visual(Desc)))
		return E_FAIL;

	if (VISUAL_TYPE::SKINNED == m_eVisualType && nullptr != m_pAnimator)
	{
		m_pAnimator->Set_ControllerEnabled(Desc.bControllerEnabled);

		if (true == Desc.bPlayDefault)
		{
			if (FAILED(Play_InitialAnimation(Desc)))
				return E_FAIL;
		}
		else if (true == Desc.bControllerEnabled)
		{
			// Controller Entry를 한 번 처리한다.
			m_pAnimator->Update(0.f);
		}
	}

	return S_OK;
}

HRESULT CMapGameplayObject::Ready_Visual(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (VISUAL_TYPE::NONE == Desc.eVisualType)
		return S_OK;

	if (Desc.strModelPrototypeTag.empty())
		return E_FAIL;

	if (FAILED(Ready_Model(Desc)))
		return E_FAIL;

	if (VISUAL_TYPE::STATIC == Desc.eVisualType)
		return S_OK;

	if (VISUAL_TYPE::SKINNED != Desc.eVisualType)
		return E_FAIL;

	return Ready_Animator(Desc);
}

HRESULT CMapGameplayObject::Ready_Model(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (FAILED(__super::Add_Component(Desc.iModelLevel, Desc.strModelPrototypeTag, TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	if (nullptr == m_pModelCom)
		return E_FAIL;

	m_MeshVisibles.assign(m_pModelCom->Get_RenderUnits().size(), true);

	return S_OK;
}

HRESULT CMapGameplayObject::Ready_Animator(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	CAnimator::ANIMATOR_DESC AnimDesc{};

	AnimDesc.BasePose = *m_pModelCom->Get_BindPosePtr();
	AnimDesc.RawAnimationLoadDesc.strFolderName = Desc.strAnimFolderTag;
	AnimDesc.RawAnimationLoadDesc.strAnimationBaseName = Desc.strAnimTag;
	AnimDesc.RawAnimationLoadDesc.eTimeUnit = Desc.eAnimTimeUnit;
	AnimDesc.RawAnimationLoadDesc.bStaticAnimation = Desc.bStaticAnimation;

	const vector<CBone*>& Bones = m_pModelCom->Get_Bones();

	AnimDesc.Bones.clear();
	AnimDesc.Bones.reserve(Bones.size());

	for (_uint i = 0; i < static_cast<_uint>(Bones.size()); ++i)
	{
		CAnimator::ANIMATOR_BONE_DESC BoneDesc{};

		if (nullptr != Bones[i])
		{
			BoneDesc.strName = Bones[i]->Get_Name();

			_wstring strBoneName;
			strBoneName.assign(BoneDesc.strName.begin(), BoneDesc.strName.end());

			BoneDesc.iNameHash = Hash_String(strBoneName.c_str());
			BoneDesc.iParentIndex = Bones[i]->Get_ParentIndex();
		}

		AnimDesc.Bones.push_back(BoneDesc);
	}

	AnimDesc.PreTransformMatrix = m_pModelCom->Get_PreTransformMatrix();

	m_pAnimator = CAnimator::Create(AnimDesc);

	if (nullptr == m_pAnimator)
		return E_FAIL;

	m_pAnimator->Set_RootBoneIndex(m_pModelCom->Get_BoneIndex(Desc.pRootBoneName));

	return S_OK;
}

HRESULT CMapGameplayObject::Play_InitialAnimation(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return E_FAIL;

	if (false == Desc.strDefaultClip.empty())
		return Play_AnimClip(Desc.strDefaultClip);

	if (false == Desc.strDefaultSequence.empty())
		return Play_AnimSequence(Desc.strDefaultSequence);

	return m_pAnimator->Play_Default();
}

void CMapGameplayObject::Drive_Animator(_float fTimeDelta)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pAnimator)
		return;

	m_pAnimator->Update(fTimeDelta);

	for (const ANIM_NOTIFY* pNotify : m_pAnimator->Get_TriggeredNotifies())
	{
		if (nullptr == pNotify)
			continue;

		if (ANIM_NOTIFY_TYPE::CUSTOM == pNotify->eType && false == pNotify->strPayload.empty())
			On_AnimNotify(pNotify->iPayloadHash);
	}

	if (true == m_bApplyRootMotion)
		Apply_RootMotion(m_pAnimator->Get_RootMotionDelta(), fTimeDelta);
}

HRESULT CMapGameplayObject::Ready_Physics(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (false == Desc.bUsePhysics)
		return S_OK;

	if (nullptr != m_pPhysicsRigidBody)
		return S_OK;

	PHYSIC_DESC PhysicDesc{};

	PhysicDesc.pOwner = this;
	PhysicDesc.iFilterWord0 = Desc.iPhysicsFilterWord0;
	PhysicDesc.iFilterWord1 = Desc.iPhysicsFilterWord1;

	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	SHAPE_DESC ShapeDesc{};

	ShapeDesc.eType = GEOMETRY_TYPE::BOX;
	ShapeDesc.vHalfExtents = Desc.vPhysicsHalfExtents;
	ShapeDesc.vLocalOffset = Desc.vPhysicsLocalOffset;
	ShapeDesc.vLocalRot = Desc.vPhysicsLocalQuat;
	ShapeDesc.bTrigger = false;

	PhysicDesc.Shapes.push_back(ShapeDesc);

	if (FAILED(__super::Add_Component(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_StaticRigidBody"), TEXT("Com_MapGameplayPhysics"), reinterpret_cast<CComponent**>(&m_pPhysicsRigidBody), &PhysicDesc)))
		return E_FAIL;

	if (false == Desc.bPhysicsStartRegistered)
		UnRegister_Physics();

	return S_OK;
}

HRESULT CMapGameplayObject::Register_Physics()
{
	if (nullptr == m_pPhysicsRigidBody || nullptr == m_pPhysicsRigidBody->Get_RigidActor())
		return S_OK;

	PxRigidActor* pActor = m_pPhysicsRigidBody->Get_RigidActor();

	if (nullptr == pActor->getScene())
		m_pGameInstance->Add_ActorToScene(pActor);

	return S_OK;
}

void CMapGameplayObject::UnRegister_Physics()
{
	if (nullptr == m_pPhysicsRigidBody || nullptr == m_pPhysicsRigidBody->Get_RigidActor())
		return;

	PxRigidActor* pActor = m_pPhysicsRigidBody->Get_RigidActor();

	if (nullptr != pActor->getScene())
		m_pGameInstance->Remove_ActorFromScene(pActor);
}

_bool CMapGameplayObject::Is_PhysicsRegistered() const
{
	if (nullptr == m_pPhysicsRigidBody || nullptr == m_pPhysicsRigidBody->Get_RigidActor())
		return false;

	return nullptr != m_pPhysicsRigidBody->Get_RigidActor()->getScene();
}

_float4x4 CMapGameplayObject::Get_InteractTransform(CGameObject* pPlayer) const
{
	_float4x4 ResultWorldMatrix{};
	XMStoreFloat4x4(&ResultWorldMatrix, XMMatrixIdentity());

	if (nullptr == pPlayer)
		return ResultWorldMatrix;

	const _float4x4* pPlayerWorld = pPlayer->Get_WorldTransform();

	if (nullptr == pPlayerWorld)
		return ResultWorldMatrix;

	// 보정값이 없는 대상은 플레이어의 현재 월드 행렬을 그대로 반환한다.
	ResultWorldMatrix = *pPlayerWorld;

	if (false == m_bUseInteractPosition || nullptr == m_pTransformCom)
		return ResultWorldMatrix;

	// 액터의 Pitch와 Roll은 제외하고 수평 Right, Look 축으로 로컬 XZ 보정값을 변환한다.
	const _vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector vRight = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::RIGHT), 0.f));
	const _vector vHorizontalLook = XMVector3Cross(vRight, vWorldUp);
	const _vector vWorldOffset = vRight * m_vInteractLocalOffsetXZ.x + vHorizontalLook * m_vInteractLocalOffsetXZ.y;
	const _vector vWorldLook = vRight * sinf(m_fInteractLocalYaw) + vHorizontalLook * cosf(m_fInteractLocalYaw);
	const _vector vObjectPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTargetPosition = vObjectPosition + vWorldOffset;

	// Y 보정은 제공하지 않고 플레이어의 현재 Y를 그대로 유지한다.
	vTargetPosition = XMVectorSetY(vTargetPosition, pPlayerWorld->_42);

	if (XMVectorGetX(XMVector3LengthSq(vWorldLook)) <= 1e-8f)
		return ResultWorldMatrix;

	// 위치와 무관하게 액터에 저장된 고정 로컬 Yaw를 월드 회전으로 변환한다.
	const _vector vNormalizedWorldLook = XMVector3Normalize(vWorldLook);
	const _float fYaw = atan2f(XMVectorGetX(vNormalizedWorldLook), XMVectorGetZ(vNormalizedWorldLook));
	_matrix InteractWorldMatrix = XMMatrixRotationY(fYaw);
	InteractWorldMatrix.r[3] = XMVectorSetW(vTargetPosition, 1.f);
	XMStoreFloat4x4(&ResultWorldMatrix, InteractWorldMatrix);

	return ResultWorldMatrix;
}

_float3 CMapGameplayObject::Get_InteractLook(CGameObject* pPlayer) const
{
	const _float4x4 InteractWorldMatrix = Get_InteractTransform(pPlayer);
	_vector vLook = XMVectorSet(InteractWorldMatrix._31, 0.f, InteractWorldMatrix._33, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= 1e-8f)
		return _float3(0.f, 0.f, 1.f);

	_float3 vInteractLook{};
	XMStoreFloat3(&vInteractLook, XMVector3Normalize(vLook));

	return vInteractLook;
}

void CMapGameplayObject::Broadcast_ObjUI(_bool bVisible, OBJECT_TYPE eObjType)
{
	if (nullptr == m_pGameInstance || OBJECT_TYPE::END == eObjType)
		return;

	OBJ_UI_EVENT_DESC EventDesc{};
	EventDesc.bVisible = bVisible;
	EventDesc.eObjType = eObjType;

	// UI는 전달받은 OBJECT_TYPE에 맞는 이름을 표시한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::OBJ_UI, &EventDesc);
}

void CMapGameplayObject::Apply_RootMotion(const ANIM_ROOT_MOTION_DELTA& Root, _float fTimeDelta)
{
	if (VISUAL_TYPE::SKINNED != m_eVisualType || nullptr == m_pTransformCom)
		return;

	_vector vWorldMove = XMVector3Rotate(XMLoadFloat3(&Root.vTranslation), m_pTransformCom->Get_Quat());

	if (XMVectorGetX(XMVector3LengthSq(vWorldMove)) > 1e-12f)
	{
		_vector vPosition = m_pTransformCom->Get_State(STATE::POSITION);
		m_pTransformCom->Set_State(STATE::POSITION, XMVectorAdd(vPosition, vWorldMove));
	}

	_vector vRootRotation = XMQuaternionNormalize(XMLoadFloat4(&Root.vRotation));
	_vector vAxis = XMVectorZero();
	_float fAngle = 0.f;

	XMQuaternionToAxisAngle(&vAxis, &fAngle, vRootRotation);

	const _float fYaw = fAngle * XMVectorGetY(vAxis);

	if (fabsf(fYaw) > 1e-5f)
	{
		_vector vYawQuaternion = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fYaw);
		_vector vNewQuaternion = XMQuaternionNormalize(XMQuaternionMultiply(vYawQuaternion, m_pTransformCom->Get_Quat()));

		m_pTransformCom->Set_Quat(vNewQuaternion);
	}
}

CMapGameplayObject* CMapGameplayObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMapGameplayObject* pInstance = new CMapGameplayObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMapGameplayObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMapGameplayObject::Clone(void* pArg)
{
	CMapGameplayObject* pInstance = new CMapGameplayObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapGameplayObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapGameplayObject::Free()
{
	m_Func_DeactivateLinkedTriggers = nullptr;

	// Scene에서 이미 빠진 Actor도 RigidActor 해제 경로가 안전하게 처리하도록 기존 Physics 흐름을 유지한다.
	if (nullptr != m_pPhysicsRigidBody && nullptr != m_pPhysicsRigidBody->Get_RigidActor() && nullptr == m_pPhysicsRigidBody->Get_RigidActor()->getScene())
		m_pGameInstance->Add_ActorToScene(m_pPhysicsRigidBody->Get_RigidActor());

	Safe_Release(m_pPhysicsRigidBody);
	Safe_Release(m_pAnimator);
	Safe_Release(m_pModelCom);

	m_MeshVisibles.clear();

	__super::Free();
}
