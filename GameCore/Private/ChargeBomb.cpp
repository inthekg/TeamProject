#include "ChargeBomb.h"
#include "AIObject.h"
#include "GameInstance.h"
#include "Transform.h"
#include "HitUtil.h"
#include "Effect.h"
#include "Model.h"
#include "Engine_Utility.h"
#include "Player.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif


CChargeBomb::CChargeBomb(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CPoolableObject{ pDevice, pContext }
{
}

HRESULT CChargeBomb::Initialize_Prototype()
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	Set_InActive();

	return S_OK;
}

HRESULT CChargeBomb::Initialize(void* pArg)
{
	return S_OK;
}

void CChargeBomb::Update(_float fTimeDelta)
{
}

void CChargeBomb::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	//Delay Time만으로 동작하지만, 그 외의 상황이 발생한다면 비활성화
	m_fLifeTime -= fTimeDelta;
	if (m_fLifeTime <= 0.f)
	{
		if (!m_bExploded) Explode();
		else InActivate();
		return;
	}

	Update_Attach(fTimeDelta);
	Update_BeepSoundPos();

#ifdef _DEBUG
	if (m_pGameInstance->Get_DebugFlag())
	{
		if(m_pDebugCapsule == nullptr)
		{
			CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
			DbgDesc.fRadius = m_fRadius;
			DbgDesc.fHalfHeight = 0.f;
			DbgDesc.iSegment = 12;
			DbgDesc.vColor = _float4{ 1.f, 0.5f, 0.2f, 1.f };

			m_pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);
			if (m_pDebugCapsule == nullptr)
			{
				MSG_BOX("CChargeBomb::Late_Update : Failed to Create Debug Capsule");
				return;
			}
		}
		if (m_pDebugCapsule != nullptr)
		{
			m_pDebugCapsule->Update_Pose(m_pTransformCom->Get_State(STATE::POSITION), XMQuaternionIdentity());
			m_pDebugCapsule->Late_Update(fTimeDelta);
		}

	}
#endif // _DEBUG


	if (m_bExploded == false)
	{
		m_fDelay -= fTimeDelta;
		if (m_fDelay <= 0.f)
		{
			Explode();
		}
	}
}

HRESULT CChargeBomb::Render()
{
	return S_OK;
}

void CChargeBomb::Activate(void* pArg)
{
	__super::Activate(pArg);

	auto pDesc = static_cast<CHARGEBOMB_DESC*>(pArg);
	if (nullptr == pDesc)
		return;

	m_pOwner = pDesc->pOwner;
	m_bExploded = false;
	m_fDelay = pDesc->fDelay;
	m_fRadius = pDesc->fRadius;
	m_fDamage = pDesc->fDamage;	
	m_eHitType = pDesc->eHitType;
	m_bUpgrade = pDesc->bUpgrade;	

	//충돌 방향의 반대로 이펙트 
	_vector vTailDir = XMVector3Normalize(-XMLoadFloat3(&pDesc->vHitDir));
	XMStoreFloat3(&m_vTailDirWorld, vTailDir);

	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&pDesc->vPosition),1.f));
	
	m_pAttachTarget = nullptr;
	m_iAttachBoneIndex = -1;	
	m_fLifeTime = pDesc->fDelay + MAX_LIFETIME_OFFSET;

	if(pDesc->pAttachTarget && !pDesc->pAttachTarget->Is_InActive() && !pDesc->pAttachTarget->Is_Dead())
	{
		//Sweep 충돌위치에서 제일 가까운 본에 붙임
		Attach_ToTarget(pDesc);
	}

	Stop_TailEffect();
	if(m_fDelay > 0.f)
	{
		Start_BeepSound();

		Update_TailEffectMatrix(m_pTransformCom->Get_State(STATE::POSITION), vTailDir);
		m_pTailFX = m_pGameInstance->Play_Effect(CHARGEBOMB_ATTACH_EFFECT, &m_TailMatrix, nullptr);
	}
}

void CChargeBomb::InActivate()
{
	__super::InActivate();
	Stop_TailEffect();
	Stop_BeepSound();
	Detach();
}

void CChargeBomb::Explode()
{
	Stop_TailEffect();
	Stop_BeepSound();

	_float3 vPos = { };
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(SOUND_EXPLO, CHANNEL_ID::SOUND_SFX, vPos);

	m_bExploded = true;
	m_pGameInstance->Play_Effect(CHARGEBOMB_EXPLODE_EFFECT, vPos);

	HitInfo hitInfo{};
	hitInfo.fDamage = (m_bUpgrade ? m_fUpgradeDamage : m_fDamage);
	hitInfo.eHitType = m_eHitType;
	hitInfo.pOtherActor = nullptr;
	hitInfo.pDamangeCauser = this;
	hitInfo.vHitPosition = vPos;

	unordered_set<CGameObject*> Targets;

	//데미지 확정하기 위해 명시적 데미지 적용
	if (m_pAttachTarget && !m_pAttachTarget->Is_InActive() && !m_pAttachTarget->Is_Dead())
	{
		m_pAttachTarget->Take_Damage(hitInfo);
		Targets.insert(m_pAttachTarget);
	}

	//범위 내 모든 오브젝트에게 데미지 적용
	CHitUtil::Overlap_Sphere(m_pGameInstance, m_fRadius, m_pTransformCom->Get_State(STATE::POSITION),
		hitInfo, Targets, static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::MONSTER)));

	//강화버전 플레이어는 별도로 Launch 처리
	if (m_bUpgrade)
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
		if (pPlayer)
		{
			_vector vBombPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vPlayerPos = pPlayer->Get_Transform()->Get_State(STATE::POSITION);

			_float fDist = XMVectorGetX(XMVector3Length(vPlayerPos - vBombPos));
			if (fDist <= 10.f) // m_fRadius 로 변경하기 
			{
				pPlayer->Launch_Explosion(m_fUpgradeDamage, vBombPos, PLAYER_LAUNCH_POWER, PLAYER_LAUNCH_UP);
			}
		}
	}

	Detach();
	InActivate();
}

void CChargeBomb::Attach_ToTarget(const CHARGEBOMB_DESC* pDesc)
{
	CAIObject* pTarget = pDesc->pAttachTarget;
	CModel* pModel = pTarget->Get_Model();
	if (pModel == nullptr) return;

	const _float4x4* pTargetWorld = pTarget->Get_Transform()->Get_WorldMatrixPtr();
	_matrix TargetWorld = XMLoadFloat4x4(pTargetWorld);
	_vector vHitPos = XMVectorSetW(XMLoadFloat3(&pDesc->vPosition), 1.f);

	//제일 가까운 본 탐색 
	_int iBestBoneIndex = -1;
	_float fBestDistSQ = FLT_MAX;
	_vector vBestBonePos = XMVectorZero();

	const _uint iBoneCount = pModel->Get_NumBones();
	for (_uint i = 0; i < iBoneCount; i++)
	{
		const _float4x4* pCombined = pModel->Get_Bone_CombinedMatPtr(i);
		if (pCombined == nullptr) continue;

		_vector vBoneWorldPos = XMVector3TransformCoord(
			XMVectorSetW(XMLoadFloat4x4(pCombined).r[3], 1.f), TargetWorld);

		_float fDistSQ = XMVectorGetX(XMVector3LengthSq(vBoneWorldPos - vHitPos));
		if(fDistSQ < fBestDistSQ)
		{
			fBestDistSQ = fDistSQ;
			iBestBoneIndex = i;
			vBestBonePos = vBoneWorldPos;
		}

	}

	if (iBestBoneIndex < 0) return;

	//캡슐 표면 Hit라서 본 방향으로 좀 당김
	_float fDist = sqrtf(fBestDistSQ);
	
	if (!Engine::IsEqual(fDist, 0.f))
	{
		_vector vDir = XMVector3Normalize(vBestBonePos - vHitPos);
		vHitPos = vHitPos + vDir * min(BOMB_ATTACH_OFFSET, fDist);
	}

	//소켓 본 기준 로컬 오프셋 계산. 즉, 본 위치를 기준으로 한 로컬 좌표계에서의 상대 위치
	_matrix SocketWorld = Engine::Make_BoneSocketWorld(pTarget->Get_Bone_CombinedMatPtr(iBestBoneIndex), pTargetWorld);
	_matrix SocketWorldInv = XMMatrixInverse(nullptr, SocketWorld);

	XMStoreFloat3(&m_vAttachLocal, XMVector3TransformCoord(vHitPos, SocketWorldInv));
	XMStoreFloat3(&m_vAttachTailDirLocal, XMVector3Normalize(
		XMVector3TransformNormal(-XMLoadFloat3(&pDesc->vHitDir), SocketWorldInv)));

	m_iAttachBoneIndex = iBestBoneIndex;
	m_pAttachTarget = pTarget;
	Safe_AddRef(m_pAttachTarget);

	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vHitPos,1.f));

}

void CChargeBomb::Update_Attach(_float fTimeDelta)
{
	if(m_pAttachTarget == nullptr || m_iAttachBoneIndex < 0)
		return;

	if (m_pAttachTarget->Is_InActive())
	{
		Detach();
		return;
	}

	const _float4x4* pCombined = m_pAttachTarget->Get_Bone_CombinedMatPtr(m_iAttachBoneIndex);
	if(pCombined == nullptr)
	{
		Detach();
		return;
	}

	_matrix SocketWorld = Engine::Make_BoneSocketWorld(pCombined, m_pAttachTarget->Get_Transform()->Get_WorldMatrixPtr());
	//저장해둔 소켓본 기준 상대위치를 타켓의 월드로 변환함 
	_vector vWorldPos = XMVector3TransformCoord(XMLoadFloat3(&m_vAttachLocal), SocketWorld);
	_vector vWorldDir = XMVector3TransformNormal(XMLoadFloat3(&m_vAttachTailDirLocal), SocketWorld);
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vWorldPos, 1.f));

	if(m_pTailFX)
		Update_TailEffectMatrix(vWorldPos, vWorldDir);
}

void CChargeBomb::Detach()
{
	Safe_Release(m_pAttachTarget);
	m_pAttachTarget = nullptr;
	m_iAttachBoneIndex = -1;
}

void CChargeBomb::Update_TailEffectMatrix(_fvector vPos, _fvector vTailDir)
{
	//발사축
	_vector vLook = XMVector3Normalize(vTailDir);
	//월드 Y축과 거의 평행이면 X축을 사용
	_vector vAxis = fabsf(XMVectorGetY(vLook)) > 0.99f ? XMVectorSet(1.f, 0.f, 0.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_vector vRight = XMVector3Normalize(XMVector3Cross(vAxis, vLook));

	_vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));
	_matrix Mat = { vRight, vUp, vLook , XMVectorSetW(vPos, 1.f) };
	XMStoreFloat4x4(&m_TailMatrix, Mat);
}

void CChargeBomb::Stop_TailEffect()
{
	if (m_pTailFX)
	{
		m_pTailFX->Stop_Follow();
		m_pTailFX->Stop();
		m_pTailFX = nullptr;
	}

}

void CChargeBomb::Start_BeepSound()
{
	Stop_BeepSound();

	_float3 vPos = { };
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_iSparkSound = m_pGameInstance->Play_Cue(SOUND_SPARK_LOOP, CHANNEL_ID::SOUND_SFX, vPos, SPARK_VOLUME, true);
	m_iBeepSound = m_pGameInstance->Play_Cue(SOUND_BEEP_LOOP, CHANNEL_ID::SOUND_SFX, vPos, BEEP_VOLUME, true);
}

void CChargeBomb::Update_BeepSoundPos()
{
	if (0 == m_iBeepSound && 0 == m_iSparkSound)
		return;

	_float3 vPos = { };
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

	if (0 != m_iSparkSound)
		m_pGameInstance->Update_SoundSourcePos(m_iSparkSound, vPos);

	if (0 != m_iBeepSound)
		m_pGameInstance->Update_SoundSourcePos(m_iBeepSound, vPos);
}

void CChargeBomb::Stop_BeepSound()
{
	if (0 != m_iSparkSound)
	{
		m_pGameInstance->Stop_SoundSource(m_iSparkSound);
		m_iSparkSound = 0;
	}

	if (0 != m_iBeepSound)
	{
		m_pGameInstance->Stop_SoundSource(m_iBeepSound);
		m_iBeepSound = 0;
	}
}

CChargeBomb* CChargeBomb::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CChargeBomb* pInstance = new CChargeBomb(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CChargeBomb");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CChargeBomb::Free()
{
	Stop_BeepSound();

	__super::Free();

	Detach();

#ifdef  _DEBUG
	Safe_Release(m_pDebugCapsule);
#endif //_DEBUG

}
