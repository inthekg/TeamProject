#include "NeedleProjectile.h"

#include "GameInstance.h"
#include "Transform.h"
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderStruct.h"
#include "HitUtil.h"

CNeedleProjectile::CNeedleProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject{ pDevice, pContext }
{
}

HRESULT CNeedleProjectile::Initialize_Prototype()
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);

	if (nullptr == m_pTransformCom)
		return E_FAIL;

	if (FAILED(Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Needle"), TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	Set_InActive();

	return S_OK;
}

HRESULT CNeedleProjectile::Initialize(void* pArg)
{
	return S_OK;
}

void CNeedleProjectile::Update(_float fTimeDelta)
{
	if (true == Is_InActive() || nullptr == m_pTransformCom)
		return;

	if (fTimeDelta <= 0.f)
		return;

	const _vector vDirection = XMLoadFloat3(&m_vDirection);
	const _vector vPreviousPosition = m_pTransformCom->Get_State(STATE::POSITION);

	const _float fMoveDistance = min(m_fSpeed * fTimeDelta, m_fRemainRange);

	if (fMoveDistance <= FLT_EPSILON)
	{
		InActivate();
		return;
	}

	const _vector vCurrentPosition = vPreviousPosition + vDirection * fMoveDistance;

	// 같은 이동 구간에서 벽 뒤 플레이어를 먼저 맞히지 않도록
	// 가장 가까운 환경 충돌 거리를 먼저 구한다.
	_float fAllowedDistance = fMoveDistance;
	_bool bEnvironmentBlocked = false;

	CollisionResult GroundCollision{};

	if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPreviousPosition, vDirection, fMoveDistance, FILTER_GROUP::GROUND, GroundCollision, PxQueryFlag::eSTATIC))
	{
		fAllowedDistance = GroundCollision.tSweepData.fDistance;
		bEnvironmentBlocked = true;
	}

	CollisionResult ObjectCollision{};

	// 일반 오브젝트는 Static과 Dynamic을 모두 검사한다.
	const PxQueryFlags ObjectQueryFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;

	if (true == m_pGameInstance->Sweep_Sphere(m_fRadius, vPreviousPosition, vDirection, fMoveDistance, FILTER_GROUP::OBJECT, ObjectCollision, ObjectQueryFlags))
	{
		if (false == bEnvironmentBlocked || ObjectCollision.tSweepData.fDistance < fAllowedDistance)
			fAllowedDistance = ObjectCollision.tSweepData.fDistance;

		bEnvironmentBlocked = true;
	}

	// 플레이어 검사는 가장 가까운 환경 충돌 지점 앞까지만 수행한다.
	const _vector vAllowedPosition = vPreviousPosition + vDirection * fAllowedDistance;

	HitInfo Hit{};
	Hit.pOtherActor = this;
	Hit.pDamangeCauser = this;
	Hit.fDamage = m_fDamage;
	Hit.eHitType = HIT_TYPE::Normal;

	if (true == CHitUtil::Sweep_Sphere(m_pGameInstance, m_fRadius, vPreviousPosition, vAllowedPosition, Hit, m_HitTargets, FILTER_GROUP::PLAYER))
	{
		InActivate();
		return;
	}

	if (true == bEnvironmentBlocked)
	{
		InActivate();
		return;
	}

	m_pTransformCom->Set_State(STATE::POSITION, vCurrentPosition);

	m_fRemainRange -= fMoveDistance;

	if (m_fRemainRange <= FLT_EPSILON)
		InActivate();
}

void CNeedleProjectile::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CNeedleProjectile::Render()
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return S_OK;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	const RENDERID eCurrentPass = m_pGameInstance->Get_CurRenderPass();

	// Static 모델은 PreTransform을 World 앞에 적용한다.
	const _matrix WorldMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix()) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());

	for (_uint i = 0; i < static_cast<_uint>(Units.size()); ++i)
	{
		CMesh* pMesh = Units[i].pMesh;
		CMaterial* pMaterial = Units[i].pMaterial;

		if (nullptr == pMesh || nullptr == pMaterial)
			continue;

		if (pMaterial->Get_RenderGroup() != eCurrentPass)
			continue;

		StaticCommand Command{};
		Command.header.etype = RENDER_TYPE::STATIC;
		Command.header.iCommandSize = sizeof(StaticCommand);
		Command.pVIBuffer = pMesh;
		Command.pMaterial = pMaterial;

		XMStoreFloat4x4(&Command.worldMatrix, WorldMatrix);

		m_pGameInstance->Submit_Static_Resolved(Command);
	}

	return S_OK;
}

void CNeedleProjectile::Activate(void* pArg)
{
	FIRE_DESC* pDesc = static_cast<FIRE_DESC*>(pArg);

	if (nullptr == pDesc)
	{
		InActivate();
		return;
	}

	const _vector vDirection = XMLoadFloat3(&pDesc->vDirection);

	if (XMVectorGetX(XMVector3LengthSq(vDirection)) <= FLT_EPSILON)
	{
		InActivate();
		return;
	}

	m_fSpeed = max(pDesc->fSpeed, 0.f);
	m_fRemainRange = max(pDesc->fRange, 0.f);
	m_fRadius = max(pDesc->fRadius, 0.01f);
	m_fDamage = max(pDesc->fDamage, 0.f);

	if (m_fSpeed <= FLT_EPSILON || m_fRemainRange <= FLT_EPSILON)
	{
		InActivate();
		return;
	}

	__super::Activate(pArg);

	XMStoreFloat3(&m_vDirection, XMVector3Normalize(vDirection));

	m_HitTargets.clear();

	const _vector vLook = XMLoadFloat3(&m_vDirection);
	const _vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_vector vRight = XMVector3Cross(vWorldUp, vLook);

	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= FLT_EPSILON)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

	// Needle 모델의 길이 방향인 로컬 +Z를 발사 방향에 맞춘다.
	m_pTransformCom->Set_State(STATE::RIGHT, vRight);
	m_pTransformCom->Set_State(STATE::UP, vUp);
	m_pTransformCom->Set_State(STATE::LOOK, vLook);
	m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat3(&pDesc->vOrigin));
}

void CNeedleProjectile::InActivate()
{
	m_HitTargets.clear();

	__super::InActivate();
}

CNeedleProjectile* CNeedleProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNeedleProjectile* pInstance = new CNeedleProjectile(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNeedleProjectile");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CNeedleProjectile::Clone(void* pArg)
{
	return nullptr;
}

void CNeedleProjectile::Free()
{
	m_HitTargets.clear();

	Safe_Release(m_pModelCom);

	__super::Free();
}
