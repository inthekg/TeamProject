#include "Effect_Apply.h"
#include "Effect_Emitter.h"

NS_BEGIN(Engine)

_bool Is_Emitting(const MainModuleDesc& tMain, float fEffectTime)
{
	float fLocal = fEffectTime - tMain.fStartDelay;
	if (fLocal < 0.f)
		return false;
	if (tMain.bLoop)
		return true;
	return (fLocal + tMain.fLifeMax) <= tMain.fDuration;
}

void Apply_Desc(CEffect_Emitter* pEmitter, const EmitterDesc& e, _bool bEmitting, float fPlayTime)
{
	Engine::EFFECT_EMITTER_DESC d{};
	d.fRate				= (e.tMain.bRateEnabled && bEmitting) ? e.tMain.fRateOverTime : 0.f;
	d.fLifeMin			= e.tMain.fLifeMin;
	d.fLifeMax			= e.tMain.fLifeMax;
	d.fSpeedMin			= e.tMain.fStartSpeedMin;
	d.fSpeedMax			= e.tMain.fStartSpeedMax;
	d.fSizeMin			= e.tMain.fStartSizeMin;
	d.fSizeMax			= e.tMain.fStartSizeMax;
	d.fStartRotMin		= e.tMain.fStartRotMin;
	d.fStartRotMax		= e.tMain.fStartRotMax;
	d.fShapeRadius		= e.tShape.fRadius;
	d.iShapeType		= static_cast<_uint>(e.tShape.iShapeType);
	d.vBiasDir			= _float3(e.tShape.fBiasDir[0], e.tShape.fBiasDir[1], e.tShape.fBiasDir[2]);
	d.fBiasWeight		= e.tShape.fBiasWeight;
	d.bEdgeEmit			= e.tShape.bEdgeEmit;
	d.bSurfaceRelease	= e.tShape.bSurfaceRelease;
	d.vGravity			= _float3(0.f, 1.f, 0.f);
	d.fGravStart		= e.tPhysics.bEnabled ? e.tPhysics.fGravityStart : 0.f;
	d.fGravEnd			= e.tPhysics.bEnabled ? e.tPhysics.fGravityEnd : 0.f;
	d.fDrag				= e.tPhysics.bEnabled ? e.tPhysics.fDrag : 0.f;
	d.vEmitPos			= _float3(e.tTransform.fPosition[0], e.tTransform.fPosition[1], e.tTransform.fPosition[2]);
	d.vEmitRot			= _float3(e.tTransform.fRotation[0], e.tTransform.fRotation[1], e.tTransform.fRotation[2]);
	d.iMaxParticles		= static_cast<_uint>(e.tMain.iMaxParticles > 0 ? e.tMain.iMaxParticles : 1);
	d.iSimSpace			= (e.tMain.iSimSpace == 0) ? 1 : 0;
	d.bInterpSpawn		= e.tMain.bInterpSpawn;
	d.bSplineSpawn		= e.tMain.bSplineSpawn;
	d.bInfinite			= e.tMain.bInfinite;
	d.iColorCurveMode	= e.tMain.iColorCurveMode;
	d.iSizeCurveMode	= e.tMain.iSizeCurveMode;
	d.iRotCurveMode		= e.tMain.iRotCurveMode;
	d.iSpeedCurveMode	= e.tMain.iSpeedCurveMode;

	if (e.tPreview.bEnabled)
	{
		const PreviewMotionDesc& pv = e.tPreview;
		float fAng = fPlayTime * pv.fSpeed;
		_vector vAxis = XMVector3Normalize(XMVectorSet(pv.fAxis[0], pv.fAxis[1], pv.fAxis[2], 0.f));
		_vector vUp = (fabsf(XMVectorGetY(vAxis)) < 0.99f) ? XMVectorSet(0.f, 1.f, 0.f, 0.f) : XMVectorSet(1.f, 0.f, 0.f, 0.f);
		_vector vT = XMVector3Normalize(XMVector3Cross(vUp, vAxis));
		_vector vB = XMVector3Cross(vAxis, vT);
		_vector vOff = XMVectorScale(XMVectorAdd(XMVectorScale(vT, cosf(fAng)), XMVectorScale(vB, sinf(fAng))), pv.fRadius);
		_float3 vPos;
		XMStoreFloat3(&vPos, XMVectorAdd(XMVectorSet(pv.fCenter[0], pv.fCenter[1], pv.fCenter[2], 0.f), vOff));
		d.vEmitPos = vPos;
	}

	d.bNoise				= e.tNoise.bEnabled;
	d.iNoiseType			= static_cast<_uint>(e.tNoise.iNoiseType);
	d.fNoiseStrength		= e.tNoise.fStrength;
	d.fNoiseFrequency		= e.tNoise.fFrequency;
	d.fNoiseDamping			= e.tNoise.fDamping;
	d.fNoiseScroll			= e.tNoise.fScrollSpeed;
	d.fNoiseOffset			= e.tNoise.fOffset;
	d.bSizeMultiply			= e.tSize.bMultiply;

	d.bUVDistort			= e.tUVDistort.bEnabled;
	d.vUVPanA				= _float2(e.tUVDistort.fPanningA[0], e.tUVDistort.fPanningA[1]);
	d.vUVPanB				= _float2(e.tUVDistort.fPanningB[0], e.tUVDistort.fPanningB[1]);
	d.vNoiseTileA			= _float2(e.tUVDistort.fTilingA[0], e.tUVDistort.fTilingA[1]);
	d.vNoiseTileB			= _float2(e.tUVDistort.fTilingB[0], e.tUVDistort.fTilingB[1]);
	d.fUVDistortStr			= e.tUVDistort.bEnabled ? e.tUVDistort.fStrength : 0.f;

	d.bDistort				= e.tDistortion.bEnabled;
	d.fDistortStr			= e.tDistortion.fStrength;
	d.iDistortMode			= e.tDistortion.iMode;
	d.bDistortDepthMask		= e.tDistortion.bDepthMask;
	d.vDistortPan			= _float2(e.tDistortion.fPanning[0], e.tDistortion.fPanning[1]);
	d.vDistortTile			= _float2(e.tDistortion.fTiling[0], e.tDistortion.fTiling[1]);

	d.bEmissive				= e.tEmissive.bEnabled;
	d.vEmisTileA			= _float2(e.tEmissive.fTilingA[0], e.tEmissive.fTilingA[1]);
	d.vEmisPanA				= _float2(e.tEmissive.fPanningA[0], e.tEmissive.fPanningA[1]);
	d.vEmisTileB			= _float2(e.tEmissive.fTilingB[0], e.tEmissive.fTilingB[1]);
	d.vEmisPanB				= _float2(e.tEmissive.fPanningB[0], e.tEmissive.fPanningB[1]);
	d.fEmisStrA				= e.tEmissive.fStrA;
	d.fEmisStr				= e.tEmissive.fStr;
	d.fEmisPower			= e.tEmissive.fPower;
	d.vEmisTint				= _float3(e.tEmissive.fTint[0], e.tEmissive.fTint[1], e.tEmissive.fTint[2]);
	d.fEmisLerp				= e.tEmissive.fLerp;
	d.fEmisUVNoiseStr		= e.tEmissive.fUVNoiseStr;
	d.iEmisBlendMode		= static_cast<_uint>(e.tEmissive.iBlendMode);

	d.bMask					= e.tMask.bEnabled;
	d.bMaskClamp			= e.tMask.bClamp;
	d.fMaskPower			= e.tMask.fPower;
	d.vMaskTiling			= _float2(e.tMask.fTiling[0], e.tMask.fTiling[1]);
	d.vMaskOffset			= _float2(e.tMask.fOffset[0], e.tMask.fOffset[1]);
	
	d.bLinearOpacity		= e.tMaterial.bLinearOpacity;
	d.fBaseDensity			= e.tMaterial.fBaseDensity;
	d.fBasePower			= e.tMaterial.fBasePower;

	d.bDissolve				= e.tDissolve.bEnabled;
	d.vDissolveTiling		= _float2(e.tDissolve.fTiling[0], e.tDissolve.fTiling[1]);
	d.fDissolveZoom			= e.tDissolve.fZoom;
	d.fDissolvePower		= e.tDissolve.fPower;
	d.fDissolveEdge			= e.tDissolve.fEdge;
	d.vDissolveEdgeColor	= _float3(e.tDissolve.fEdgeColor[0], e.tDissolve.fEdgeColor[1], e.tDissolve.fEdgeColor[2]);
	d.fDissolveEdgeInt		= e.tDissolve.fEdgeIntensity;
	d.fDissolveThreshold	= e.tDissolve.fThreshold;
	d.bDissolveUseAge		= e.tDissolve.bUseAge;

	d.bFresnel				= e.tFresnel.bEnabled;
	d.fFresPower			= e.tFresnel.fOutlinePower;
	d.fFresOutInt			= e.tFresnel.fOutlineIntensity;
	d.fFresInInt			= e.tFresnel.fInsideIntensity;
	d.fFresInverseStr		= e.tFresnel.fInverseStr;
	d.vFresColor			= _float3(e.tFresnel.fColor[0], e.tFresnel.fColor[1], e.tFresnel.fColor[2]);

	d.bScale3D				= e.tScale3D.bEnabled;

	d.bFlipbook				= e.TextureSheet.bEnabled;
	d.iFlipCols				= static_cast<_uint>(e.TextureSheet.iCols > 0 ? e.TextureSheet.iCols : 1);
	d.iFlipRows				= static_cast<_uint>(e.TextureSheet.iRows > 0 ? e.TextureSheet.iRows : 1);
	d.fFlipFrameRate		= e.TextureSheet.fFrameRate;
	d.bFlipSeedBased		= e.TextureSheet.bSeedBased;
	d.iFlipPlayMode			= static_cast<_uint>(e.TextureSheet.iPlayMode);
	d.iFlipFixedFrame		= static_cast<_uint>(e.TextureSheet.iFixedFrame > 0 ? e.TextureSheet.iFixedFrame : 0);

	d.bSoftParticle			= e.tRenderer.bSoftParticle;
	d.fSoftFade				= e.tRenderer.fSoftFadeDistance;
	d.iAlignment			= static_cast<_uint>(e.tRenderer.iAlignment);
	d.fStretchScale			= e.tRenderer.fStretchScale;
	d.vMeshScale			= _float3(e.tTransform.fScale[0], e.tTransform.fScale[1], e.tTransform.fScale[2]);
	d.iSpinMode				= static_cast<_uint>(e.tRenderer.iSpinMode);
	d.vSpinAxis				= _float3(e.tRenderer.fSpinAxis[0], e.tRenderer.fSpinAxis[1], e.tRenderer.fSpinAxis[2]);
	d.vPivot				= _float2(e.tRenderer.fPivot[0], e.tRenderer.fPivot[1]);
	d.fEmiIntensity			= e.tRenderer.fEmiIntensity;
	d.fSphereHardness		= e.tRenderer.fSphereHardness;
	d.fDiffuseSharpness		= e.tRenderer.fDiffuseSharpness;

	d.bVelocity				= e.tVelocity.bEnabled;
	d.vLinearVel			= _float3(e.tVelocity.fLinearVelocity[0], e.tVelocity.fLinearVelocity[1], e.tVelocity.fLinearVelocity[2]);
	d.fOrbit				= e.tVelocity.fOrbitVelocity;
	d.vOrbitCenter			= _float3(e.tVelocity.fOrbitCenter[0], e.tVelocity.fOrbitCenter[1], e.tVelocity.fOrbitCenter[2]);
	d.vOrbitAxis			= _float3(e.tVelocity.fOrbitAxis[0], e.tVelocity.fOrbitAxis[1], e.tVelocity.fOrbitAxis[2]);
	d.fVelSpeedMin			= e.tVelocity.fSpeedMin;
	d.fVelSpeedMax			= e.tVelocity.fSpeedMax;
	d.fSpeedByAngle			= e.tVelocity.fSpeedByAngle;
	d.bSpiral				= e.tVelocity.bSpiral;
	d.bConverge				= e.tVelocity.bConverge;
	d.fArrivalRadius		= e.tVelocity.fArivalRadius;

	if (e.tColor.bEnabled && !e.tColor.vKeyFrames.empty())
		d.vColor = _float4(e.tColor.vKeyFrames[0].fColor[0], e.tColor.vKeyFrames[0].fColor[1],
			e.tColor.vKeyFrames[0].fColor[2], e.tColor.vKeyFrames[0].fColor[3]);
	else
		d.vColor = _float4(e.fDotColor[0], e.fDotColor[1], e.fDotColor[2], e.fDotColor[3]);

	pEmitter->Set_Desc(d);


	if (pEmitter->Curves_Changed_Update(
		e.tColor.vKeyFrames, e.tColor.bEnabled,
		e.tSize.vKeyframes, e.tSize.bEnabled,
		e.tRotOverLife.vKeyFrames, e.tRotOverLife.bEnabled,
		e.tScale3D.vKeyX, e.tScale3D.vKeyY, e.tScale3D.vKeyZ, e.tScale3D.bEnabled,
		e.tSpeedOverLife.vKeyFrames, e.tSpeedOverLife.bEnabled))
	{
		_float4 colorLUT[Engine::CEffect_Emitter::LUT_SIZE];
		_float sizeLUT[Engine::CEffect_Emitter::LUT_SIZE];
		_float rotLUT[Engine::CEffect_Emitter::LUT_SIZE];
		_float speedLUT[Engine::CEffect_Emitter::LUT_SIZE];

		for (_uint i = 0; i < Engine::CEffect_Emitter::LUT_SIZE; ++i)
		{
			float t = i / static_cast<float>(Engine::CEffect_Emitter::LUT_SIZE - 1);
			float c[4];
			EvalColor(e.tColor.vKeyFrames, t, c);
			colorLUT[i] = _float4(c[0], c[1], c[2], c[3]);
			sizeLUT[i] = EvalFloat(e.tSize.vKeyframes, t);
			rotLUT[i] = EvalFloat(e.tRotOverLife.vKeyFrames, t);
			speedLUT[i] = EvalFloat(e.tSpeedOverLife.vKeyFrames, t);
		}
		pEmitter->Set_ColorCurve(colorLUT, e.tColor.bEnabled);
		pEmitter->Set_SizeCurve(sizeLUT, e.tSize.bEnabled);
		pEmitter->Set_RotCurve(rotLUT, e.tRotOverLife.bEnabled);
		pEmitter->Set_SpeedCurve(speedLUT, e.tSpeedOverLife.bEnabled);

		_float3 scale3DLUT[CEffect_Emitter::LUT_SIZE];
		for (_uint i = 0; i < CEffect_Emitter::LUT_SIZE; ++i)
		{
			float t = i / static_cast<float>(CEffect_Emitter::LUT_SIZE - 1);
			scale3DLUT[i] = _float3(EvalFloat(e.tScale3D.vKeyX, t), EvalFloat(e.tScale3D.vKeyY, t), EvalFloat(e.tScale3D.vKeyZ, t));
		}
		pEmitter->Set_Scale3DCurve(scale3DLUT, e.tScale3D.bEnabled);
	}

	pEmitter->Set_Material(static_cast<_uint>(e.tMaterial.iBlendMode), e.tMaterial.bZWrite, e.tMaterial.bZTest, e.tMaterial.strDiffusePath);

	pEmitter->Set_UVDistort(e.tUVDistort.strNoiseAPath, e.tUVDistort.strNoiseBPath);
	pEmitter->Set_Emissive(e.tEmissive.strEmissiveAPath, e.tEmissive.strEmissiveBPath);
	pEmitter->Set_Mask(e.tMask.strMaskPath);
	pEmitter->Set_Dissolve(e.tDissolve.strDissolvePath);
	pEmitter->Set_Fresnel(e.tFresnel.strCrackPath);

	if (e.tRenderer.iAlignment == 2)
		pEmitter->Set_Mesh(e.tRenderer.strMeshPath, static_cast<_uint>(e.tRenderer.iMeshIndex < 0 ? 0 : e.tRenderer.iMeshIndex));

	const _uint iEmitMeshIdx = static_cast<_uint>(e.tShape.iEmitMeshIndex < 0 ? 0 : e.tShape.iEmitMeshIndex);
	const _bool bMeshShape = (e.tShape.iShapeType == 5);
	const _bool bSkelPinned = (bMeshShape && e.tShape.iEmitMeshSource == 1);

	pEmitter->Set_EmitMesh(e.tShape.strEmitMeshPath, iEmitMeshIdx,
		e.tShape.fEmitMeshScale, bMeshShape && !bSkelPinned);
	pEmitter->Set_EmitSkinnedMesh(e.tShape.strEmitMeshPath, e.tShape.iEmitMeshIndex, bSkelPinned);

	pEmitter->Set_VectorField(e.tVectorField.strFieldPath, e.tVectorField.bEnabled,
		e.tVectorField.fIntensity, e.tVectorField.fTightness,
		_float3(e.tVectorField.fTileSize[0], e.tVectorField.fTileSize[1], e.tVectorField.fTileSize[2]));
}

NS_END