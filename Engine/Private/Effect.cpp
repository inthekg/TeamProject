#include "Effect.h"
#include "GameInstance.h"
#include "Effect_Emitter.h"
#include "Effect_Trail.h"
#include "Effect_Apply.h"
#include "Decal.h"
#include "Model.h"

namespace
{
	_vector Screw_Point(const _vector& A, const _vector& B, const _vector& vAxis, _float fAngle, _float t)
	{
		_vector vBA = XMVectorSubtract(B, A);
		const _float d = XMVectorGetX(XMVector3Dot(vBA, vAxis));
		_vector vBp = XMVectorSubtract(B, XMVectorScale(vAxis, d));

		_vector vMid = XMVectorScale(XMVectorAdd(A, vBp), 0.5f);
		_vector vChord = XMVectorSubtract(vBp, A);
		const _float fTanHalf = tanf(fAngle * 0.5f);
		_vector vPerp = XMVector3Cross(vAxis, vChord);
		_vector c = XMVectorAdd(vMid, XMVectorScale(vPerp, 1.f / (2.f * fTanHalf)));

		_vector qf = XMQuaternionRotationAxis(vAxis, fAngle * t);
		_vector vRot = XMVector3Rotate(XMVectorSubtract(A, c), qf);
		return XMVectorAdd(XMVectorAdd(c, vRot), XMVectorScale(vAxis, t * d));
	}
}

CEffect::CEffect(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject { pDevice, pContext }
{

}

HRESULT CEffect::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CEffect::Activate(void* pArg)
{
	Set_Active();
	m_fPlayTime = 0.f;
	m_bFinished = false;

	const EFFECT_PLAY_DESC* pPlay = static_cast<const EFFECT_PLAY_DESC*>(pArg);

	if (pPlay == nullptr || pPlay->pDesc == nullptr || pPlay->pDesc->Emitters.empty())
	{
		m_bFinished = true;
		return;
	}

	m_pDesc = pPlay->pDesc;
	m_vWorldPos = pPlay->vWorldPos;
	m_pShaderBundle = pPlay->pShaderBundle;

	m_pFollowBoneMat = pPlay->pFollowBoneMat;
	m_pFollowBoneMat2 = pPlay->pFollowBoneMat2;
	m_pOwnerWorld = pPlay->pOwnerWorld;
	m_vFollowOffset = pPlay->vFollowOffset;
	m_vFollowAngle = pPlay->vFollowAngle;
	m_bFollow = (m_pFollowBoneMat != nullptr || m_pOwnerWorld != nullptr);
	m_bPositionOnly = m_pDesc->bPositionOnly;
	m_pFollowModel = pPlay->pFollowModel;

	Update_Root();
	m_matRootSpawn = m_matRoot;
	m_bHasPrevRoot = false;
	m_bHasPrevRoot2 = false;

	m_fLoopPeriod = 0.f;
	m_fTotalDuration = 0.f;
	m_bLoop = false;
	m_bStopping = false;
	m_fMaxLife = 0.f;
	m_fMaxTrailLife = 0.f;

	for (const auto& tEmitter : m_pDesc->Emitters)
	{
		_float fWindow = tEmitter.tMain.fStartDelay + tEmitter.tMain.fDuration;
		if (fWindow > m_fLoopPeriod)
			m_fLoopPeriod = fWindow;
		_float fEnd = fWindow + tEmitter.tMain.fLifeMax;
		if (fEnd > m_fTotalDuration)
			m_fTotalDuration = fEnd;
		if (tEmitter.tMain.fLifeMax > m_fMaxLife)
			m_fMaxLife = tEmitter.tMain.fLifeMax;
		if (tEmitter.tMain.bLoop)
			m_bLoop = true;
		if (tEmitter.tTrail.bEnabled && tEmitter.tTrail.fTrailLife > m_fMaxTrailLife)
			m_fMaxTrailLife = tEmitter.tTrail.fTrailLife;
	}

	if (m_bLoop)
		m_fTotalDuration = FLT_MAX;

	for (size_t i = 0; i < m_pDesc->Emitters.size(); ++i)
	{
		CEffect_Emitter* pEmitter = m_pGameInstance->Get_PoolObject<CEffect_Emitter>();
		if (pEmitter == nullptr)
			break;
		pEmitter->Bind_SharedShaders(m_pShaderBundle);
		m_Emitters.push_back(pEmitter);
	}

	for (size_t i = 0; i < m_pDesc->Emitters.size(); ++i)
	{
		if (!m_pDesc->Emitters[i].tTrail.bEnabled)
		{
			m_Trails.push_back(nullptr);
			continue;
		}
		CEffect_Trail* pTrail = m_pGameInstance->Get_PoolObject<CEffect_Trail>();
		if (pTrail != nullptr)
			pTrail->Bind_SharedShaders(m_pShaderBundle);
		m_Trails.push_back(pTrail);
	}

	for (size_t i = 0; i < m_pDesc->Decals.size(); ++i)
	{
		CDecal* pDecal = m_pGameInstance->Get_PoolObject<CDecal>();
		if (pDecal == nullptr)
			break;
		pDecal->Bind_SharedShaders(m_pShaderBundle);
		m_Decals.push_back(pDecal);
	}

	m_PrevLocalTime.assign(m_Emitters.size(), FLT_MAX);
	m_EmitterBurstDone.assign(m_Emitters.size(), false);
	m_EmitterBonePtrs.assign(m_Emitters.size(), {});

	auto ModelMeshPath = [](CModel* pModel) -> string
		{
			string strOut;
			if (pModel == nullptr)
				return strOut;
			const _wstring& strWPath = pModel->Get_MeshBoneFilePath();
			if (strWPath.empty())
				return strOut;
			int iLen = WideCharToMultiByte(CP_ACP, 0, strWPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (iLen > 0)
			{
				strOut.resize(iLen - 1);
				WideCharToMultiByte(CP_ACP, 0, strWPath.c_str(), -1, &strOut[0], iLen, nullptr, nullptr);
			}
			return strOut;
		};

	m_strFollowMeshPath = ModelMeshPath(m_pFollowModel);

	m_FollowParts.clear();
	m_FollowPartMeshPaths.clear();

	if (pPlay->pFollowParts != nullptr)
	{
		for (CModel* pPart : *pPlay->pFollowParts)
		{
			if (pPart == nullptr)
				continue;
			const string strPath = ModelMeshPath(pPart);
			if (strPath.empty())
				continue;
			m_FollowParts.push_back(pPart);
			m_FollowPartMeshPaths.push_back(strPath);
		}
	}

	if (m_pFollowModel != nullptr)
	{
		for (size_t i = 0; i < m_Emitters.size(); ++i)
		{
			const SkeletalEmitDesc& tSkeletal = m_pDesc->Emitters[i].tSkeletal;
			if (!tSkeletal.bEnabled)
				continue;

			for (const string& strBone : tSkeletal.BoneName)
			{
				const _float4x4* pBoneMat = m_pFollowModel->Get_Bone_CombinedMatPtr(strBone.c_str());
				if (pBoneMat != nullptr)
					m_EmitterBonePtrs[i].push_back(pBoneMat);
			}
		}
	}
}

void CEffect::Update(_float fTimeDelta)
{
	if (m_bFinished || m_pDesc == nullptr)
		return;

	m_fPlayTime += fTimeDelta;

	if (m_bLoop && m_fLoopPeriod > 0.f && m_fPlayTime >= m_fLoopPeriod)
		m_fPlayTime -= m_fLoopPeriod;

	if (m_bFollow)
		Update_Root();

	for (size_t i = 0; i < m_Emitters.size(); ++i)
	{
		const EmitterDesc& tEmitter = m_pDesc->Emitters[i];
		if (!tEmitter.bActive)
			continue;

		const _float fLocal = m_fPlayTime - tEmitter.tMain.fStartDelay;

		if (!m_bStopping && tEmitter.tMain.bBurstEnabled)
		{
			const _bool bEnterWindow = (m_PrevLocalTime[i] < 0.f && fLocal >= 0.f);
			const _bool bWrapped = (fLocal < m_PrevLocalTime[i] && fLocal >= 0.f);
			_bool bFire = bEnterWindow || bWrapped;
			if (tEmitter.tMain.bInfinite && m_EmitterBurstDone[i])
				bFire = false;
			if (bFire)
			{
				int iMin = tEmitter.tMain.iBurstMin > 0 ? tEmitter.tMain.iBurstMin : 0;
				int iMax = tEmitter.tMain.iBurstMax > 0 ? tEmitter.tMain.iBurstMax : 0;
				int iCount = (iMax > iMin) ? (iMin + rand() % (iMax - iMin + 1)) : iMin;
				if (iCount > 0)
				{
					m_Emitters[i]->Request_Burst(static_cast<_uint>(iCount));
					m_EmitterBurstDone[i] = true;
				}
			}
		}
		m_PrevLocalTime[i] = fLocal;

		_bool bEmitting = m_bStopping ? false : Is_Emitting(tEmitter.tMain, m_fPlayTime);
		Apply_Desc(m_Emitters[i], tEmitter, bEmitting, m_fPlayTime);

		const float* off = tEmitter.tTransform.fPosition;
		_vector vEmitPos = XMVector3TransformCoord(XMVectorSet(off[0], off[1], off[2], 1.f), XMLoadFloat4x4(&m_matRoot));
		_float3 vOut;
		XMStoreFloat3(&vOut, vEmitPos);

		const _bool bBoneEmit = (i < m_EmitterBonePtrs.size() && !m_EmitterBonePtrs[i].empty());

		if (bBoneEmit)
		{
			const auto& BonePtrs = m_EmitterBonePtrs[i];
			_matrix matOwner = (m_pOwnerWorld != nullptr) ? XMLoadFloat4x4(m_pOwnerWorld) : XMMatrixIdentity();

			m_BoneMatScratch.resize(BonePtrs.size());
			for (size_t b = 0; b < BonePtrs.size(); ++b)
				XMStoreFloat4x4(&m_BoneMatScratch[b], XMLoadFloat4x4(BonePtrs[b]) * matOwner);

			m_Emitters[i]->Set_BoneSources(m_BoneMatScratch.data(),
				static_cast<_uint>(m_BoneMatScratch.size()), tEmitter.tSkeletal.bInheritRotation
				, tEmitter.tSkeletal.bAttach);

			m_Emitters[i]->Set_EmitPos(_float3(off[0], off[1], off[2]));
			_float4x4 matIdentity;
			XMStoreFloat4x4(&matIdentity, XMMatrixIdentity());
			m_Emitters[i]->Set_EmitExtraRot(matIdentity);
		}
		else
		{
			m_Emitters[i]->Set_BoneSources(nullptr, 0, false);
			m_Emitters[i]->Set_EmitPos(vOut);
			m_Emitters[i]->Set_EmitExtraRot(m_matFollowRot);
		}

		const float* oc = tEmitter.tVelocity.fOrbitCenter;
		_matrix matOrbitRoot = XMLoadFloat4x4(&m_matRoot);
		_vector vOrbitCenter = XMVector3TransformCoord(XMVectorSet(oc[0], oc[1], oc[2], 1.f), matOrbitRoot);
		_float3 vOrbitOut;
		XMStoreFloat3(&vOrbitOut, vOrbitCenter);
		m_Emitters[i]->Set_OrbitCenter(vOrbitOut);
		m_Emitters[i]->Set_SceneSRV(tEmitter.tDistortion.bEnabled
			? m_pGameInstance->Get_RenderTargetSRV(TEXT("Target_SceneColor")) : nullptr);

		// 표면 고정: 모델 표면에 파티클을 고정한다.
		const ShapeModuleDesc& tShape = tEmitter.tShape;
		const _bool bWantPin = (tShape.iShapeType == 5 && tShape.iEmitMeshSource == 1);

		// Part_Object -> 모든 파츠가 GPU Bone Range를 가질 경우, 그룹을 고정한다.
		_bool bPartsPinned = false;
		if (bWantPin && !m_FollowParts.empty())
		{
			bPartsPinned = true;
			for (CModel* pPart : m_FollowParts)
			{
				if (pPart == nullptr || !pPart->Has_GPUFinalBoneRange())
				{
					bPartsPinned = false;
					break;
				}
			}
		}

		if (bPartsPinned)
		{
			// 각 파츠 = 한 그룹. 바인드 정점은 파츠 각각 구성, (경로 캐시 후 최초 1회 로드함)
			const _int iSelPart = tShape.iEmitMeshIndex;
			m_PinGroupPathScratch.clear();
			m_PinGroupBaseScratch.clear();
			for (size_t g = 0; g < m_FollowParts.size(); ++g)
			{
				if (iSelPart >= 0 && static_cast<size_t>(iSelPart) != g)
					continue;
				m_PinGroupPathScratch.push_back(m_FollowPartMeshPaths[g]);
				m_PinGroupBaseScratch.push_back(m_FollowParts[g]->Get_GPUFinalBoneOffset());
			}

			if (!m_PinGroupPathScratch.empty())
			{
				m_Emitters[i]->Set_EmitSkinnedMeshGroups(m_PinGroupPathScratch, true);

				ID3D11ShaderResourceView* pPaletteSRV = m_pGameInstance->Get_GlobalFinalBoneMatrixSRV();
				const _uint iCapacity = m_pGameInstance->Get_GlobalFinalBoneMatrixCapacity();

				_float4x4 matWorld;
				if (m_pOwnerWorld != nullptr)
					matWorld = *m_pOwnerWorld;
				else
					XMStoreFloat4x4(&matWorld, XMMatrixIdentity());
				if (pPaletteSRV != nullptr)
					m_Emitters[i]->Set_EmitMeshSkinSource(pPaletteSRV, m_PinGroupBaseScratch.data(),
						static_cast<_uint>(m_PinGroupBaseScratch.size()), iCapacity, matWorld);
			}
		
		}
		else if (bWantPin
			&& m_pFollowModel != nullptr && m_pFollowModel->Has_GPUFinalBoneRange()
			&& !m_strFollowMeshPath.empty())
		{
			m_Emitters[i]->Set_EmitSkinnedMesh(m_strFollowMeshPath, tShape.iEmitMeshIndex, true);

			ID3D11ShaderResourceView* pPaletteSRV = m_pGameInstance->Get_GlobalFinalBoneMatrixSRV();
			const _uint iBoneOffset = m_pFollowModel->Get_GPUFinalBoneOffset();
			const _uint iCapacity = m_pGameInstance->Get_GlobalFinalBoneMatrixCapacity();

			_float4x4 matWorld;
			if (m_pOwnerWorld != nullptr)
				matWorld = *m_pOwnerWorld;
			else
				XMStoreFloat4x4(&matWorld, XMMatrixIdentity());

			if (pPaletteSRV != nullptr)
			{
				const _uint iGroupBase[1] = { iBoneOffset };
				m_Emitters[i]->Set_EmitMeshSkinSource(pPaletteSRV, iGroupBase, 1, iCapacity, matWorld);
			}
		}

		{
			const int iSrc = tEmitter.tShape.iSpawnSourceIndex;
			if (iSrc >= 0 && iSrc < static_cast<int>(m_Emitters.size())
				&& iSrc != static_cast<int>(i) && m_Emitters[iSrc] != nullptr)
			{
				CEffect_Emitter* pSrc = m_Emitters[iSrc];
				m_Emitters[i]->Set_SpawnSource(pSrc->Get_PoolSRV(), pSrc->Get_AliveSRV(), pSrc->Get_AliveCountSRV(), true);
			}
			else
				m_Emitters[i]->Set_SpawnSource(nullptr, nullptr, nullptr, false);
		}

		{
			_bool bArcEmit = false;
			_float3 vArcAxis = {};
			_float fArcAngle = 0.f;

			if (!m_bStopping && tEmitter.tMain.bSplineSpawn
				&& m_pFollowBoneMat2 != nullptr && m_bHasPrevRoot)
			{
				_float3 vTop, vRot, vPrevTop, vPrevRot;
				XMStoreFloat3(&vTop, XMLoadFloat4x4(&m_matRoot).r[3]);
				XMStoreFloat3(&vRot, XMLoadFloat4x4(&m_matRoot2).r[3]);
				XMStoreFloat3(&vPrevTop, XMLoadFloat4x4(&m_matRootPrev).r[3]);
				XMStoreFloat3(&vPrevRot, XMLoadFloat4x4(&m_matRoot2Prev).r[3]);

				_vector vCH = XMVectorScale(XMVectorSubtract(XMLoadFloat3(&vTop), XMLoadFloat3(&vRot)), 0.5f);
				_vector vPH = XMVectorScale(XMVectorSubtract(XMLoadFloat3(&vPrevTop), XMLoadFloat3(&vPrevRot)), 0.5f);

				_vector vDirPrev = XMVector3Normalize(vPH);
				_vector vDirCur = XMVector3Normalize(vCH);
				_vector vAxis = XMVector3Cross(vDirPrev, vDirCur);
				const _float fAxisLenSq = XMVectorGetX(XMVector3LengthSq(vAxis));
				const _float fCosA = XMVectorGetX(XMVector3Dot(vDirPrev, vDirCur));
				const _float fSeg = (fAxisLenSq > 1e-12f) ? atan2f(sqrtf(fAxisLenSq), fCosA) : 0.f;

				// 트레일이 실제로 아크를 그리는 조건(iSteps >= 2)과 동일하게 맞춘다.
				if (fSeg > 0.0087f && fAxisLenSq > 1e-12f)
				{
					const _int iSteps = static_cast<_int>(ceilf(fSeg / 0.035f));
					if (iSteps >= 2)
					{
						XMStoreFloat3(&vArcAxis, XMVector3Normalize(vAxis));
						fArcAngle = fSeg;
						bArcEmit = true;
					}
				}
			}
			m_Emitters[i]->Set_EmitArc(vArcAxis, fArcAngle, bArcEmit);
		}

		m_Emitters[i]->Update(fTimeDelta);
		
		m_pGameInstance->Add_RenderGroup(RENDERID::BLEND, m_Emitters[i]);

		if (i < m_Trails.size() && m_Trails[i] != nullptr)
		{
			const TrailModuleDesc& tTrail = tEmitter.tTrail;
			if (tTrail.bEnabled)
			{
				m_Trails[i]->Set_Params(tTrail);
				m_Trails[i]->Set_Color(_float4(tEmitter.fDotColor[0], tEmitter.fDotColor[1],
					tEmitter.fDotColor[2], tEmitter.fDotColor[3]));

				_float4 colorLUT[CEffect_Trail::LUT_SIZE];
				for (_uint k = 0; k < CEffect_Trail::LUT_SIZE; ++k)
				{
					const float t = k / static_cast<float>(CEffect_Trail::LUT_SIZE - 1);
					float c[4];
					EvalColor(tEmitter.tColor.vKeyFrames, t, c);
					colorLUT[k] = _float4(c[0], c[1], c[2], c[3]);
				}
				m_Trails[i]->Set_ColorCurve(colorLUT, tEmitter.tColor.bEnabled);
				m_Trails[i]->Set_Texture(tTrail.strTexturePath);
				m_Trails[i]->Set_NoiseTexture(tTrail.strNoiseTexPath);
				
				m_Trails[i]->Set_SceneSRV(tTrail.bUseDistortion
					? m_pGameInstance->Get_RenderTargetSRV(TEXT("Target_SceneColor")) : nullptr);

				if (m_bStopping)
				{
					m_Trails[i]->Begin_Fade();
				}
				else if (tTrail.bDualSocket && m_pFollowBoneMat2 != nullptr)
				{
					_float3 vTop, vRot;
					XMStoreFloat3(&vTop, XMLoadFloat4x4(&m_matRoot).r[3]);
					XMStoreFloat3(&vRot, XMLoadFloat4x4(&m_matRoot2).r[3]);

					_bool bArc = false;

					if (m_bHasPrevRoot)
					{
						_float3 vPrevTop, vPrevRot;
						XMStoreFloat3(&vPrevTop, XMLoadFloat4x4(&m_matRootPrev).r[3]);
						XMStoreFloat3(&vPrevRot, XMLoadFloat4x4(&m_matRoot2Prev).r[3]);

						_vector vCC = XMVectorScale(XMVectorAdd(XMLoadFloat3(&vTop), XMLoadFloat3(&vRot)), 0.5f);
						_vector vPC = XMVectorScale(XMVectorAdd(XMLoadFloat3(&vPrevTop), XMLoadFloat3(&vPrevRot)), 0.5f);
						_vector vCH = XMVectorScale(XMVectorSubtract(XMLoadFloat3(&vTop), XMLoadFloat3(&vRot)), 0.5f);
						_vector vPH = XMVectorScale(XMVectorSubtract(XMLoadFloat3(&vPrevTop), XMLoadFloat3(&vPrevRot)), 0.5f);

						_vector vDirPrev = XMVector3Normalize(vPH);
						_vector vDirCur = XMVector3Normalize(vCH);
						_vector vAxis = XMVector3Cross(vDirPrev, vDirCur);
						const _float fAxisLenSq = XMVectorGetX(XMVector3LengthSq(vAxis));
						const _float fCosA = XMVectorGetX(XMVector3Dot(vDirPrev, vDirCur));
						const _float fSeg = (fAxisLenSq > 1e-12f) ? atan2f(sqrtf(fAxisLenSq), fCosA) : 0.f;

						if (fSeg > 0.0087f && fAxisLenSq > 1e-12f)
						{
							vAxis = XMVector3Normalize(vAxis);

							const _float fHalfLenPrev = XMVectorGetX(XMVector3Length(vPH));
							const _float fHalfLenCur = XMVectorGetX(XMVector3Length(vCH));

							_int iSteps = static_cast<_int>(ceilf(fSeg / 0.035f));

							if (iSteps > 128)
								iSteps = 128;

							if (iSteps >= 2)
							{
								vector<_float3> Centers; Centers.reserve(iSteps);
								vector<_float3> Halves; Halves.reserve(iSteps);
								for (_int k = 1; k <= iSteps; ++k)
								{
									const _float t = k / static_cast<_float>(iSteps);
									_float3 oc;
									XMStoreFloat3(&oc, Screw_Point(vPC, vCC, vAxis, fSeg, t));
									_vector qf = XMQuaternionRotationAxis(vAxis, fSeg * t);
									_vector vDir = XMVector3Rotate(vDirPrev, qf);
									const _float fLen = fHalfLenPrev + (fHalfLenCur - fHalfLenPrev) * t;
									_float3 oh;
									XMStoreFloat3(&oh, XMVectorScale(vDir, fLen));
									Centers.push_back(oc);
									Halves.push_back(oh);
								}

								m_Trails[i]->Set_Anchor2_Arc(Centers, Halves);
								bArc = true;
							}
						}
					}
					if (!bArc)
						m_Trails[i]->Set_Anchor2(vTop, vRot);
				}
				else
				{
					m_Trails[i]->Set_Anchor(vOut);
				}
				
				m_Trails[i]->Update(fTimeDelta);
				m_pGameInstance->Add_RenderGroup(RENDERID::BLEND, m_Trails[i]);
			}
			else
			{
				m_Trails[i]->Reset();
			}
		}
	}

	_matrix matDecalRoot = XMLoadFloat4x4(&m_matRootSpawn);

	for (size_t i = 0; i < m_Decals.size(); ++i)
	{
		if (m_Decals[i] == nullptr || i >= m_pDesc->Decals.size())
			continue;

		const DecalDesc& d = m_pDesc->Decals[i];

		_matrix matS = XMMatrixScaling(d.fBoxSize[0], d.fBoxSize[1], d.fBoxSize[2]);
		_matrix matR = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(d.fRotation[0]),
			XMConvertToRadians(d.fRotation[1]),
			XMConvertToRadians(d.fRotation[2]));
		_matrix matT = XMMatrixTranslation(d.fOffset[0], d.fOffset[1], d.fOffset[2]);

		_float4x4 matWorld;
		XMStoreFloat4x4(&matWorld, matS * matR * matT * matDecalRoot);

		_float fAlpha = d.bActive ? 1.f : 0.f;
		if (d.bActive && d.bUseLifetime && d.fLifeTime > 0.f)
		{
			const _float fLocal = m_fPlayTime - d.fStartDelay;
			if (fLocal < 0.f || fLocal > d.fLifeTime)
				fAlpha = 0.f;
			else
				fAlpha = EvalFloat(d.vFadeCurve, fLocal / d.fLifeTime);
		}

		m_Decals[i]->Set_Desc(d);
		m_Decals[i]->Set_World(matWorld);
		m_Decals[i]->Set_GlobalAlpha(fAlpha);

		if (fAlpha > 0.f)
			m_pGameInstance->Add_Decal(m_Decals[i]);
	}

	if (m_bFollow)
	{
		if (m_bHasPrevRoot)
		{
			m_matRootPrev2 = m_matRootPrev;
			m_matRoot2Prev2 = m_matRoot2Prev;
			m_bHasPrevRoot2 = true;
		}
		m_matRootPrev = m_matRoot;
		m_matRoot2Prev = m_matRoot2;
		m_bHasPrevRoot = true;
	}
	else
	{
		m_bHasPrevRoot = false;
		m_bHasPrevRoot2 = false;
	}

	if (m_fPlayTime >= m_fTotalDuration)
	{
		m_bFinished = true;
		InActivate();
	}
}

void CEffect::InActivate()
{
	Release_Emitters();
	Set_InActive();
}

void CEffect::Update_Root()
{
	if (m_bFollow)
	{
		_matrix matRoot = XMMatrixIdentity();
		if (m_pFollowBoneMat != nullptr)
			matRoot = XMLoadFloat4x4(m_pFollowBoneMat);
		if (m_pOwnerWorld != nullptr)
			matRoot = matRoot * XMLoadFloat4x4(m_pOwnerWorld);

		_matrix matLocalOffset = XMMatrixRotationRollPitchYaw(m_vFollowAngle.x, m_vFollowAngle.y, m_vFollowAngle.z) *
			XMMatrixTranslation(m_vFollowOffset.x, m_vFollowOffset.y, m_vFollowOffset.z);

		matRoot = matLocalOffset * matRoot;

		if (m_bPositionOnly)
			matRoot = XMMatrixTranslationFromVector(matRoot.r[3]);

		XMStoreFloat3(&m_vWorldPos, matRoot.r[3]);
		XMStoreFloat4x4(&m_matRoot, matRoot);

		if (m_pFollowBoneMat2 != nullptr)
		{
			_matrix matRoot2 = XMLoadFloat4x4(m_pFollowBoneMat2);
			if (m_pOwnerWorld != nullptr)
				matRoot2 = matRoot2 * XMLoadFloat4x4(m_pOwnerWorld);
			matRoot2 = matLocalOffset * matRoot2;
			if (m_bPositionOnly)
				matRoot2 = XMMatrixTranslationFromVector(matRoot2.r[3]);
			XMStoreFloat4x4(&m_matRoot2, matRoot2);
		}
		else
		{
			m_matRoot2 = m_matRoot;
		}

		if (m_bPositionOnly)
		{
			XMStoreFloat4x4(&m_matFollowRot, XMMatrixIdentity());
			return;
		}

		_matrix matRot = matRoot;
		matRot.r[0] = XMVector3Normalize(matRot.r[0]);
		matRot.r[1] = XMVector3Normalize(matRot.r[1]);
		matRot.r[2] = XMVector3Normalize(matRot.r[2]);
		matRot.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		XMStoreFloat4x4(&m_matFollowRot, matRot);
		return; 
	}

	_matrix matAngle = XMMatrixRotationRollPitchYaw(m_vFollowAngle.x, m_vFollowAngle.y, m_vFollowAngle.z);
	XMStoreFloat4x4(&m_matRoot, matAngle * XMMatrixTranslation(m_vWorldPos.x, m_vWorldPos.y, m_vWorldPos.z));
	m_matRoot2 = m_matRoot;
	XMStoreFloat4x4(&m_matFollowRot, matAngle);
}

void CEffect::Stop_Follow()
{
	m_bFollow = false;
	m_pFollowBoneMat = nullptr;
	m_pFollowBoneMat2 = nullptr;
	m_pOwnerWorld = nullptr;
}

void CEffect::Stop()
{
	if (m_bFinished || m_bStopping)
		return;
	m_bStopping = true;
	m_bLoop = false;
	const _float fTail = (m_fMaxTrailLife > m_fMaxLife) ? m_fMaxTrailLife : m_fMaxLife;
	m_fTotalDuration = m_fPlayTime + fTail;
}

void CEffect::Stop_Immediate()
{
	if (m_bFinished)
		return;
	m_bFinished = true;
	InActivate();
}

void CEffect::Release_Emitters()
{
	for (auto& pEmitter : m_Emitters)
		Safe_Release(pEmitter);

	for (auto& pTrail : m_Trails)
		Safe_Release(pTrail);

	for (auto& pDecal : m_Decals)
		Safe_Release(pDecal);

	m_Emitters.clear();
	m_Trails.clear();
	m_Decals.clear();
	m_PrevLocalTime.clear();
	m_EmitterBurstDone.clear();
	m_EmitterBonePtrs.clear();
	m_FollowParts.clear();
	m_FollowPartMeshPaths.clear();
	m_PinGroupBaseScratch.clear();
	m_PinGroupPathScratch.clear();
	m_pDesc = nullptr;
	m_pShaderBundle = nullptr;
	m_bFollow = false;
	m_bPositionOnly = false;
	m_pFollowBoneMat = nullptr;
	m_pFollowBoneMat2 = nullptr;
	m_pOwnerWorld = nullptr;
	m_pFollowModel = nullptr;
	m_vFollowOffset = {};
	m_vFollowAngle = {};
	m_bStopping = false;
	m_fMaxLife = 0.f;
	m_fMaxTrailLife = 0.f;
}

CEffect* CEffect::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEffect* pInstance = new CEffect(pDevice, pContext);
	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Create : CEffect");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CEffect::Free()
{
	Release_Emitters();
	__super::Free();
}