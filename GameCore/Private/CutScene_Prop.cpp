#include "CutScene_Prop.h"

#include "CutScene_Actor.h"
#include "GameInstance.h"
#include "Material.h"
#include "RenderStruct.h"

static _matrix CutScene_PropBlendWorldMatrix(_fmatrix StartMatrix, _fmatrix TargetMatrix, _float fWeight)
{
	_vector vStartScale = {};
	_vector vStartRotation = {};
	_vector vStartTranslation = {};
	_vector vTargetScale = {};
	_vector vTargetRotation = {};
	_vector vTargetTranslation = {};

	if (false == XMMatrixDecompose(&vStartScale, &vStartRotation, &vStartTranslation, StartMatrix) || false == XMMatrixDecompose(&vTargetScale, &vTargetRotation, &vTargetTranslation, TargetMatrix))
		return TargetMatrix;

	const _float fClampedWeight = max(0.f, min(fWeight, 1.f));
	const _vector vScale = XMVectorLerp(vStartScale, vTargetScale, fClampedWeight);
	const _vector vRotation = XMQuaternionSlerp(XMQuaternionNormalize(vStartRotation), XMQuaternionNormalize(vTargetRotation), fClampedWeight);
	const _vector vTranslation = XMVectorLerp(vStartTranslation, vTargetTranslation, fClampedWeight);
	return XMMatrixScalingFromVector(vScale) * XMMatrixRotationQuaternion(vRotation) * XMMatrixTranslationFromVector(vTranslation);
}

CCutScene_Prop::CCutScene_Prop(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapObject{ pDevice, pContext }
{
	XMStoreFloat4x4(&m_LocalWorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_AttachmentBlendStartWorld, XMMatrixIdentity());
}

HRESULT CCutScene_Prop::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	CUTSCENE_PROP_DESC* pDesc = static_cast<CUTSCENE_PROP_DESC*>(pArg);

	if (pDesc->strMeshName.empty() || pDesc->strMaterialName.empty())
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Set_Mesh(pDesc->strMeshName)))
		return E_FAIL;

	if (FAILED(Set_Material(pDesc->strMaterialName)))
		return E_FAIL;

	Set_CutSceneTransform(XMLoadFloat4x4(&pDesc->WorldMatrix));

	Set_EmissiveAmount(pDesc->fEmissiveAmount);

	return S_OK;
}

void CCutScene_Prop::Late_Update(_float fTimeDelta)
{
	if (m_bCutSceneAttachmentActive && nullptr != m_pParentActor && false == m_strAttachBoneName.empty())
	{
		_float4x4 ParentBoneWorld = {};

		if (m_pParentActor->Get_CutSceneBoneWorld(m_strAttachBoneName, &ParentBoneWorld))
		{
			const _matrix TargetWorldMatrix = XMLoadFloat4x4(&m_LocalWorldMatrix) * XMLoadFloat4x4(&ParentBoneWorld);

			if (m_bAttachmentBlendActive)
			{
				Set_Transform(CutScene_PropBlendWorldMatrix(XMLoadFloat4x4(&m_AttachmentBlendStartWorld), TargetWorldMatrix, m_fAttachmentBlendWeight));

				if (m_fAttachmentBlendWeight >= 1.f)
					m_bAttachmentBlendActive = false;
			}
			else
				Set_Transform(TargetWorldMatrix);
		}
	}

	__super::Late_Update(fTimeDelta);

	if (Is_InActive() || false == m_bCutSceneVisible || nullptr == m_pMaterialcom)
		return;

	// 독립 컷신 소품은 자신의 Material 렌더 그룹으로 제출한다.
	const RENDERID eRenderGroup = RENDERID::END == m_pMaterialcom->Get_RenderGroup() ? RENDERID::NONBLEND : m_pMaterialcom->Get_RenderGroup();
	m_pGameInstance->Add_RenderGroup(eRenderGroup, this);
}

void CCutScene_Prop::Set_CutSceneTransform(_fmatrix Transform)
{
	XMStoreFloat4x4(&m_LocalWorldMatrix, Transform);

	if (false == m_bCutSceneAttachmentActive || nullptr == m_pParentActor)
		Set_Transform(Transform);
}

void CCutScene_Prop::Set_CutSceneAttachment(CCutScene_Actor* pParentActor, const string& strBoneName, _float fBlendWeight)
{
	const _bool bWasAttached = m_bCutSceneAttachmentActive && nullptr != m_pParentActor && false == m_strAttachBoneName.empty();
	const _bool bAttachmentChanged = m_pParentActor != pParentActor || m_strAttachBoneName != strBoneName;

	if (bAttachmentChanged && bWasAttached && nullptr != m_pTransformCom && fBlendWeight < 1.f)
	{
		// Bone이 바뀌는 첫 프레임의 World를 보존해 새 Bone 위치로 순간 이동하지 않게 한다.
		m_AttachmentBlendStartWorld = *m_pTransformCom->Get_WorldMatrixPtr();
		m_bAttachmentBlendActive = true;
	}
	else if (bAttachmentChanged)
		m_bAttachmentBlendActive = false;

	m_pParentActor = pParentActor;
	m_strAttachBoneName = strBoneName;
	m_bCutSceneAttachmentActive = nullptr != m_pParentActor && false == m_strAttachBoneName.empty();
	m_fAttachmentBlendWeight = max(0.f, min(fBlendWeight, 1.f));
}

void CCutScene_Prop::Set_CutSceneAttachmentActive(_bool bActive)
{
	m_bCutSceneAttachmentActive = bActive && nullptr != m_pParentActor && false == m_strAttachBoneName.empty();

	if (false == m_bCutSceneAttachmentActive)
	{
		m_bAttachmentBlendActive = false;
		m_fAttachmentBlendWeight = 1.f;
		// 분리된 소품은 Track Transform을 월드 행렬로 사용한다.
		Set_Transform(XMLoadFloat4x4(&m_LocalWorldMatrix));
	}
}

void CCutScene_Prop::Clear_CutSceneAttachment()
{
	m_pParentActor = nullptr;
	m_strAttachBoneName.clear();
	m_bCutSceneAttachmentActive = false;
	m_bAttachmentBlendActive = false;
	m_fAttachmentBlendWeight = 1.f;
	Set_Transform(XMLoadFloat4x4(&m_LocalWorldMatrix));
}

HRESULT CCutScene_Prop::Render()
{
	if (nullptr == m_pMaterialcom || nullptr == m_pMeshCom || nullptr == m_pTransformCom)
		return E_FAIL;

	StaticCommand Command{};
	Command.header.etype = RENDER_TYPE::STATIC;
	Command.header.iCommandSize = sizeof(StaticCommand);
	Command.pVIBuffer = m_pMeshCom;
	Command.pMaterial = m_pMaterialcom;
	Command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();
	Command.fEmissiveAmount = m_fEmissiveAmount;
	Command.fDissolveAmount = m_fDissolveAmount;
	Command.vDissolveColor = m_vDissolveColor;

	// 공유 Material은 수정하지 않고 오브젝트별 렌더 명령에 이미시브와 디졸브 값을 기록한다.
	m_pGameInstance->Submit_Static_Resolved(Command);

	return S_OK;
}

CCutScene_Prop* CCutScene_Prop::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CUTSCENE_PROP_DESC& Desc)
{
	CCutScene_Prop* pInstance = new CCutScene_Prop(pDevice, pContext);

	if (FAILED(pInstance->Initialize(const_cast<CUTSCENE_PROP_DESC*>(&Desc))))
	{
		MSG_BOX("Failed to Create : CCutScene_Prop");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCutScene_Prop::Free()
{
	Clear_CutSceneAttachment();
	__super::Free();
}
