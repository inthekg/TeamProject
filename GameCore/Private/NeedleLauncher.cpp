#include "NeedleLauncher.h"

#include "NeedleProjectile.h"
#include "GameInstance.h"
#include "Transform.h"
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderStruct.h"

CNeedleLauncher::CNeedleLauncher(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CNeedleLauncher::CNeedleLauncher(const CNeedleLauncher& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CNeedleLauncher::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CNeedleLauncher::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	NEEDLE_LAUNCHER_DESC* pDesc = static_cast<NEEDLE_LAUNCHER_DESC*>(pArg);

	Ready_DefaultDesc(pDesc);

	m_eLayerLevel = pDesc->eLayerLevel;

	m_iBurstCount = max(pDesc->iBurstCount, 1u);
	m_fShotInterval = max(pDesc->fShotInterval, 0.01f);

	m_fNeedleSpeed = max(pDesc->fNeedleSpeed, 0.01f);
	m_fNeedleRange = max(pDesc->fNeedleRange, 0.01f);
	m_fNeedleRadius = max(pDesc->fNeedleRadius, 0.01f);
	m_fNeedleDamage = max(pDesc->fNeedleDamage, 0.f);

	m_HoleLocalOffsets = pDesc->HoleLocalOffsets;
	m_fMuzzleForwardOffset = max(pDesc->fMuzzleForwardOffset, 0.f);

	// 현재 NeedleLauncher는 정확히 3개의 NeedleHole을 하나의 그룹으로 관리한다.
	if (3 != m_HoleLocalOffsets.size())
		return E_FAIL;

	for (const _float3& vHoleLocalOffset : m_HoleLocalOffsets)
	{
		// 비정상 좌표가 렌더링과 Projectile 위치에 전달되는 것을 막는다.
		if (false == std::isfinite(vHoleLocalOffset.x) ||
			false == std::isfinite(vHoleLocalOffset.y) ||
			false == std::isfinite(vHoleLocalOffset.z))
		{
			return E_FAIL;
		}
	}

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	m_eState = LAUNCHER_STATE::READY;
	m_iFiredCount = 0;
	m_fShotTimer = 0.f;

	return S_OK;
}

HRESULT CNeedleLauncher::Render()
{
	// 일반 렌더링에서는 현재 Material Pass와 일치하는 Mesh만 제출한다.
	return Render_LauncherGroup(true);
}

HRESULT CNeedleLauncher::Render_Shadow()
{
	// Shadow 렌더링에서는 Material Pass 검사를 생략한다.
	return Render_LauncherGroup(false);
}

HRESULT CNeedleLauncher::Render_LauncherGroup(_bool bCheckMaterialPass)
{
	if (nullptr == m_pGameInstance || nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return S_OK;

	const auto& Units = m_pModelCom->Get_RenderUnits();
	const RENDERID eCurrentPass = m_pGameInstance->Get_CurRenderPass();
	const _matrix PreTransformMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix());
	const _matrix LauncherWorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());

	// 하나의 CModel을 세 개의 로컬 위치에 각각 제출한다.
	for (const _float3& vHoleLocalOffset : m_HoleLocalOffsets)
	{
		const _matrix HoleLocalMatrix = XMMatrixTranslation(vHoleLocalOffset.x, vHoleLocalOffset.y, vHoleLocalOffset.z);
		const _matrix HoleWorldMatrix = PreTransformMatrix * HoleLocalMatrix * LauncherWorldMatrix;

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

			XMStoreFloat4x4(&Command.worldMatrix, HoleWorldMatrix);

			m_pGameInstance->Submit_Static_Resolved(Command);
		}
	}

	return S_OK;
}

void CNeedleLauncher::Ready_DefaultDesc(NEEDLE_LAUNCHER_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::STATIC;

	if (true == pDesc->strModelPrototypeTag.empty())
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NeedleHole");

	// 발사구는 충돌체 없이 시각 모델만 사용한다.
	pDesc->bUsePhysics = false;
}

void CNeedleLauncher::On_LinkedActivate()
{
	// 발사 중 중복 활성화는 무시한다.
	if (LAUNCHER_STATE::READY != m_eState)
		return;

	m_eState = LAUNCHER_STATE::FIRING;
	m_iFiredCount = 0;
	m_fShotTimer = 0.f;

	// 첫 번째 화살은 발판을 밟는 즉시 발사한다.
	Fire_Needle();
	++m_iFiredCount;

	if (m_iFiredCount >= m_iBurstCount)
		Finish_Burst();
}

void CNeedleLauncher::Update_MapGameplayObject(_float fTimeDelta)
{
	if (LAUNCHER_STATE::FIRING != m_eState)
		return;

	m_fShotTimer += fTimeDelta;

	// 프레임이 밀려도 발사 횟수가 누락되지 않게 while로 처리한다.
	while (m_fShotTimer >= m_fShotInterval && m_iFiredCount < m_iBurstCount)
	{
		m_fShotTimer -= m_fShotInterval;

		Fire_Needle();
		++m_iFiredCount;
	}

	if (m_iFiredCount >= m_iBurstCount)
		Finish_Burst();
}

void CNeedleLauncher::Fire_Needle()
{
	if (nullptr == m_pGameInstance || nullptr == m_pTransformCom)
		return;

	const _matrix WorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	const _vector vFireDirection = XMVector3Normalize(XMVectorNegate(m_pTransformCom->Get_State(STATE::LOOK)));

	// 한 번의 발사 타이밍에 NeedleHole 3개가 동시에 화살을 발사한다.
	for (const _float3& vHoleLocalOffset : m_HoleLocalOffsets)
	{
		_float3 vMuzzleLocalPosition = vHoleLocalOffset;

		// NeedleHole 중심보다 로컬 +Z 방향으로 조금 앞에서 생성한다.
		vMuzzleLocalPosition.z -= m_fMuzzleForwardOffset;

		const _vector vLocalMuzzlePosition = XMLoadFloat3(&vMuzzleLocalPosition);
		const _vector vMuzzleWorldPosition = XMVector3TransformCoord(vLocalMuzzlePosition, WorldMatrix);

		CNeedleProjectile::FIRE_DESC FireDesc{};

		XMStoreFloat3(&FireDesc.vOrigin, vMuzzleWorldPosition);
		XMStoreFloat3(&FireDesc.vDirection, vFireDirection);

		FireDesc.fSpeed = m_fNeedleSpeed;
		FireDesc.fRange = m_fNeedleRange;
		FireDesc.fRadius = m_fNeedleRadius;
		FireDesc.fDamage = m_fNeedleDamage;

		CNeedleProjectile* pNeedle = m_pGameInstance->Get_PoolObject<CNeedleProjectile>(&FireDesc);

		// 풀이 부족하면 해당 발사구만 건너뛴다.
		if (nullptr == pNeedle)
			continue;

		if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eLayerLevel), TEXT("Layer_TrapProjectile"), pNeedle)))
			Safe_Release(pNeedle);
	}
}

void CNeedleLauncher::Finish_Burst()
{
	m_eState = LAUNCHER_STATE::READY;
	m_iFiredCount = 0;
	m_fShotTimer = 0.f;
}

CNeedleLauncher* CNeedleLauncher::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNeedleLauncher* pInstance = new CNeedleLauncher(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNeedleLauncher");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CNeedleLauncher::Clone(void* pArg)
{
	CNeedleLauncher* pInstance = new CNeedleLauncher(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNeedleLauncher");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNeedleLauncher::Free()
{
	__super::Free();
}
