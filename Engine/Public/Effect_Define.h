#pragma once
#include "Engine_Defines.h"
/* Effect 관련 구조체 정보를 모아놓은 헤더 */

NS_BEGIN(Engine)

struct FloatKeyframe
{
	float	fTime = 0.f;
	float	fValue = 0.f;
	int		iInterp = 0; // 보간 방식 선택, 1. Linear | 2. SmoothStep | 3. EaseIn | 4. EaseOut
};

struct ColorKeyframe
{
	float	fTime = 0.f;
	float	fColor[4] = { 1.f, 1.f, 1.f, 1.f };
	int		iInterp = 0;
};

inline float ApplyInterp(float fAlpha, int iInterp) // 보간 방식을 설정하는 함수
{
	switch (iInterp)
	{
	case 1:
		return fAlpha * fAlpha * (3.f - 2.f * fAlpha);
	case 2:
		return fAlpha * fAlpha;
	case 3:
		return fAlpha * (2.f - fAlpha);
	default:
		return fAlpha;
	}
}

// 키프레임을 통해서 실제 값을 보간하는 함수
inline float EvalFloat(const vector<FloatKeyframe>& kf, float fTime)
{
	if (kf.empty())
		return 0.f;

	if (kf.size() == 1)
		return kf[0].fValue;

	if (fTime <= kf.front().fTime)
		return kf.front().fValue;

	if (fTime >= kf.back().fTime)
		return kf.back().fValue;

	for (size_t i = 0; i < kf.size() - 1; ++i)
	{
		if (fTime >= kf[i].fTime && fTime <= kf[i + 1].fTime)
		{
			float fAlpha = (fTime - kf[i].fTime) / (kf[i + 1].fTime - kf[i].fTime);
			fAlpha = ApplyInterp(fAlpha, kf[i].iInterp);
			return kf[i].fValue + fAlpha * (kf[i + 1].fValue - kf[i].fValue);
		}
	}

	return kf.back().fValue;
}


// 실제 값을 통해서 색을 보간하는 함수
inline void EvalColor(const vector<ColorKeyframe>& kf, float fTime, float fOut[4])
{
	if (kf.empty())
	{
		fOut[0] = fOut[1] = fOut[2] = fOut[3] = 1.f;
		return;
	}

	if (kf.size() == 1)
	{
		memcpy(fOut, kf[0].fColor, sizeof(float) * 4);
		return;
	}

	if (fTime <= kf.front().fTime)
	{
		memcpy(fOut, kf.front().fColor, sizeof(float) * 4);
		return;
	}

	if (fTime >= kf.back().fTime)
	{
		memcpy(fOut, kf.back().fColor, sizeof(float) * 4);
		return;
	}

	for (size_t i = 0; i < kf.size() - 1; ++i)
	{
		if (fTime >= kf[i].fTime && fTime <= kf[i + 1].fTime)
		{
			float fAlpha = (fTime - kf[i].fTime) / (kf[i + 1].fTime - kf[i].fTime);
			fAlpha = ApplyInterp(fAlpha, kf[i].iInterp);
			for (int c = 0; c < 4; c++)
				fOut[c] = kf[i].fColor[c] + fAlpha * (kf[i + 1].fColor[c] - kf[i].fColor[c]);
			return;
		}
	}
	memcpy(fOut, kf.back().fColor, sizeof(float) * 4);
}

struct MainModuleDesc
{
	bool	bRateEnabled = true;	// 시간 당 지속 생성
	float	fRateOverTime = 10.f;	// 지속 생성할 시간
	bool	bBurstEnabled = false;	// 방출 타입 활성화/비활성화 여부 
	int		iBurstMin = 1;			// 생성 수량의 최솟값
	int		iBurstMax = 20;			// 생성 수량의 최댓값

	float	fStartSpeedMin = 1.f;	// 생성 될 때의 최소 속도
	float	fStartSpeedMax = 3.f;	// 생성 될 떄의 최대 속도
	float	fStartSizeMin = 0.1f;	// 생성 될 때의 최소 크기
	float	fStartSizeMax = 0.1f;	// 생성 될 때의 최대 크기
	float	fStartRotMin = 0.f;		// 생성 될 떄의 최소 회전값
	float	fStartRotMax = 0.f;		// 생성 될 떄의 최대 회전값
	int		iMaxParticles = 1000;	// 최대 생성 파티클 수

	float	fLifeMin = 0.5f;		// 파티클 수명의 최솟값
	float	fLifeMax = 1.2f;		// 파티클 수명의 최댓값
	float	fStartDelay = 0.f;		// 이미터의 시작 딜레이
	float	fDuration = 2.f;		// 이미터의 지속 시간
	bool	bLoop = false;			// 반복 여부

	bool	bInfinite = false;
	int		iColorCurveMode = 2;
	int		iSizeCurveMode = 2;
	int		iRotCurveMode = 1;
	int		iSpeedCurveMode = 2;

	int		iSimSpace = 1;			// 시뮬레이션 공간, 1. Local | 2. World
	bool	bInterpSpawn = false;	// 프레임 간 스폰 위치 보간
	bool	bSplineSpawn = false;	// 스폰 위치 곡선 보간
};

struct ShapeModuleDesc
{
	int		iShapeType = 0;						// 파티클 생성 모양,
	// 1. Hemisphere(반구) | 2. Sphere(구) | 3. Cone(원뿔) | 4. Directional(방향) | 5. Mesh (메쉬)
	float	fBiasDir[3] = { 0.f, 1.f, 0.f };	// 방향 가중치 벡터 -> 특정 방향성을 가지게 하고 싶을 때 사용
	float	fBiasWeight = 0.5f;					// 가중치의 강도, 높을 수록 방향성이 강해짐
	float	fRadius = 0.3f;						// 생성 반경
	bool	bEdgeEmit = false;					// 표면에서만 생성할 지, 내부 전체에서 생성할지

	string	strEmitMeshPath;					// 표면 샘플링에 사용할 메쉬 경로
	int		iEmitMeshIndex = 0;					// 다중 Mesh일 경우, 샘플링할 Mesh 인덱스
	float	fEmitMeshScale = 1.f;				// 메쉬 로컬 좌표에 곱할 스케일
	int		iEmitMeshSource = 0;				// 메쉬 표면 소스 0. Static | 1. Skeletal
	bool	bSurfaceRelease = false;			// 표면 방출 여부

	int     iSpawnSourceIndex = -1;				// 다른 이미터의 파티클 위치에서 스폰
};

struct RendererModuleDesc
{
	int		iAlignment = 0;						// 렌더 속성, 
	// 1. Billboard | 2. Velocity-Aligned Billboard(Stretch Billboard) | 3. Mesh | 4. Oriented Quad
	int		iSortMode = 0;						// 정렬 속성, 0. 정렬 없음 | 1. 카메라 거리 기준 정렬
	float	fEmiIntensity = 1.f;				// Emissive 강도
	float	fSphereHardness = 0.5f;				// 구형 마스크 선명도
	float	fDiffuseSharpness = 1.f;
	string	strMeshPath;						// 렌더 속성이 메쉬일 경우, 메쉬의 경로.

	bool	bSoftParticle = false;				// Soft Particle(파티클과 다른 물체 경계 부드럽게 보간)
	float	fSoftFadeDistance = 0.5f;			// Soft Particle의 강도

	float	fStretchScale = 0.1f;				// Stretch 가중치
	int		iMeshIndex = 0;						// 다중 Mesh일 경우, 선택할 Mesh의 인덱스

	int		iSpinMode = 0;						// Mesh 회전 모드
	float	fSpinAxis[3] = { 0.f, 1.f, 0.f };	// Mesh Spin 축
	float	fPivot[2] = { 0.5f, 0.5f };
};

struct VelocityModuleDesc
{
	bool	bEnabled = false;						// 활성/비활성화
	float	fSpeedMin = 1.f;						// 최소 속도
	float	fSpeedMax = 3.f;						// 최대 속도
	float	fSpeedByAngle = 0.f;					// 진행 방향에 따른 속도값
	float	fOrbitVelocity = 0.f;					// 중심축을 선회할 때 속도
	float	fLinearVelocity[3] = { 0.f, 0.f, 0.f }; // X/Y/Z 속도
	float	fOrbitCenter[3] = { 0.f, 0.f, 0.f };	// 공전 중심
	float	fOrbitAxis[3] = { 0.f, 1.f, 0.f };		// 공전 축
	bool	bSpiral = false;						// 나선형 움직임 On/Off
	bool	bConverge = false;						// 외곽 생성 후 중심으로 수렴
	float	fArivalRadius = 0.1f;					// 중심 도달 시 소멸 반경
};

struct PhysicsModuleDesc
{
	bool	bEnabled = false;
	float	fGravityStart = 0.f;	// 중력 시작값
	float	fGravityEnd = -9.8f;	// 중력 종료값
	float	fDrag = 0.05f;			// 공기 저항 계수 (Velocity *= (1 - Drag) * DeltaTime)
};

struct ColorModuleDesc
{
	bool	bEnabled = false;
	vector<ColorKeyframe> vKeyFrames =
	{
		{ 0.f, { 1.f, 1.f, 1.f, 1.f }, 0 },
		{ 1.f, { 1.f, 1.f, 1.f, 0.f }, 0 }
	};
};

struct SizeModuleDesc
{
	bool	bEnabled = false;
	bool	bMultiply = false;
	vector<FloatKeyframe> vKeyframes =
	{
		{ 0.f, 0.1f, 0 },
		{ 1.f, 0.f, 0 }
	};
};

struct SpeedOverLifeModuleDesc
{
	bool	bEnabled = false;
	vector<FloatKeyframe> vKeyFrames =
	{
		{ 0.f, 1.f, 0 },
		{ 1.f, 1.f, 0 }
	};
};

struct NoiseModuleDesc
{
	bool	bEnabled = false;
	int		iNoiseType = 0;		// 노이즈 타입, 0. Perlin(불규칙적) | 1. Sin(규칙적)
	float	fStrength = 1.f;	// 노이즈 강도
	float	fFrequency = 1.f;	// 노이즈 주파수
	float	fDamping = 0.f;		// 속력에 반비례한 강도
	float	fOffset = 0.f;		// 파티클별 랜덤 오프셋
	float	fScrollSpeed = 0.f;	// 노이즈 필드 이동 속도
};

struct VectorFieldModuleDesc
{
	bool	bEnabled = false;
	string	strFieldPath;
	float	fIntensity = 20.f;
	float	fTightness = 0.f;
	float	fTileSize[3] = { 1.f, 1.f, 1.f };
};

struct TextureSheetModuleDesc
{
	bool	bEnabled = false;
	int		iRows = 4;			// 스프라이트 시트 행 수
	int		iCols = 4;			// 스프라이트 시트 열 수
	float	fFrameRate = 16.f;	// 초 당 프레임 수
	bool	bSeedBased = false;	// SV_InstanceID 기반 파티클 별 시작 프레임 분산
	int		iPlayMode = 0;		// 재생 모드, 1. Loop | 2. Once | 3. Fixed
	int		iFixedFrame = 0;	// Fixed에서 고정할 프레임 번호
};

struct TrailModuleDesc
{
	bool	bEnabled = false;
	float	fWidth = 0.1f;			// 트레일 폭
	float	fMaxLength = 2.f;		// 트레일의 최대 길이
	float	fTailWidthScale = 1.f; // 꼬리 폭 비율 (1이면 비율 줄이지 않음)
	float	fTrailLife = 0.3f;		// 점 수명 (0이면 무한)
	float	fPointInterval = 0.05f; // 점 추가 간격 
	bool	bSpline = true;			// 곡선 보간
	float	fSinSpeed = 0.f;		// 일렁거림 속도
	float	fWaveAmplitude = 0.f;	// 일렁거림 강도
	bool	bFadeOut = true;		// 꼬리로 갈 수록 투명하게 처리
	int		iTexMode = 0;			// 텍스쳐 모드, 0. Stretch | 1. Tile
	string	strTexturePath;			// 텍스쳐 경로
	bool	bDualSocket = false;	// 2소켓 모드 활성화/비활성화

	string	strNoiseTexPath;		// Noise 텍스쳐 경로
	float	fNoiseTileU = 0.2f;		// Noise UV 타일
	float	fNoiseTileV = 1.f;
	float	fNoiseSpeedX = -0.1f;	// Noise 스크롤 스피드
	float	fNoiseSpeedY = 0.f;
	float	fBasePower = 1.f;
	float	fBaseDensity = 1.f;
	float	fBaseTileU = 1.f;
	float	fBaseTileV = 1.f;
	float	fBaseSpeedX = 0.f;
	float	fBaseSpeedY = 0.f;
	float	fNoisePower = 1.f;
	float	fNoiseDensity = 1.f;
	float	fEmiIntensity = 1.f;	// 발광 강도
	bool	bUseAlphaY = false;
	float	fAlphaYPower = 15.f;
	float	fAlphaYPower2 = 15.f;
	float	fAlphaYPivot = 0.5f;
	int		iAlphaYMode = 1;
	int		iBlendMode = 1;

	bool	bUseDistortion = false;
	float	fDistortion = 10.f;
};

struct RotOverLifeModuleDesc
{
	bool	bEnabled = false;
	vector<FloatKeyframe> vKeyFrames =
	{
		{ 0.f, 0.f, 0 },
		{ 1.f, 90.f, 0 }
	};
};

struct TransformModuleDesc
{
	float fPosition[3] = { 0.f, 0.f, 0.f };
	float fRotation[3] = { 0.f, 0.f, 0.f };
	float fScale[3] = { 1.f, 1.f, 1.f };
};

struct MaterialModuleDesc
{
	string	strDiffusePath;
	int		iBlendMode = 1;
	bool	bZWrite = false;
	bool	bZTest = true;
	bool	bLinearOpacity = false;
	float	fBaseDensity = 1.f;
	float	fBasePower = 1.f;
};

struct UVDistortModuleDesc
{
	bool	bEnabled = false;				
	string	strNoiseAPath;					// UV 왜곡 노이즈 1
	string	strNoiseBPath;					// UV 왜곡 노이즈 2
	float	fPanningA[2] = { 0.f, 0.f };	// 노이즈 A 흐름 속도
	float	fPanningB[2] = { 0.f, 0.f };	// 노이즈 B 흐름 속도
	float	fTilingA[2] = { 1.f, 1.f };
	float	fTilingB[2] = { 1.f, 1.f };
	float	fStrength = 0.1f;				// 메인 UV 왜곡 강도
};

struct DistortionModuleDesc
{
	bool bEnabled = false;
	float fStrength = 10.f; // 화면 굴절 세기
	int iMode = 0;			// 0. 굴절만 | 1. 굴절 + 색
	bool bDepthMask = true; // 앞 물체 번짐 방지
	string strNoisePath;
	float fPanning[2] = { 0.05f, 0.05f }; // 노이즈 스크롤 속도
	float fTiling[2] = { 1.f, 1.f }; // 노이즈 타일링
};

struct EmissiveModuleDesc
{
	bool	bEnabled = false; 
	string	strEmissiveAPath;
	string	strEmissiveBPath;				// Emissive B 노이즈 
	float	fTilingA[2] = { 0.f, 0.f };
	float	fPanningA[2] = { 0.f, 0.f };
	float	fTilingB[2] = { 0.f, 0.f };
	float	fPanningB[2] = { 0.f, 0.f };
	float	fStrA = 0.f;
	float	fStr = 1.f;
	float	fPower = 1.f;
	float	fTint[3] = { 1.f, 1.f, 1.f };
	int		iBlendMode = 0;
	float	fLerp = 0.f;
	float	fUVNoiseStr = 0.f;
};

struct Scale3DModuleDesc
{
	bool					bEnabled = false;
	vector<FloatKeyframe>	vKeyX = { { 0.f, 1.f, 0}, {1.f, 1.f, 0} };
	vector<FloatKeyframe>	vKeyY = { { 0.f, 1.f, 0}, {1.f, 1.f, 0} };
	vector<FloatKeyframe>	vKeyZ = { { 0.f, 1.f, 0}, {1.f, 1.f, 0} };
};

struct FresnelModuleDesc
{
	bool	bEnabled = false;
	float	fOutlinePower = 0.f;			// 외곽 집중도
	float	fOutlineIntensity = 0.f;		// 외곽 발광 강도
	float	fInsideIntensity = 0.f;			// 안쪽 채움
	float	fInverseStr = 10.f;				// 역프레넬(중앙 막)
	float	fColor[3] = { 1.f, 1.f, 1.f };	// 림 색
	string	strCrackPath;
};

struct DissolveModuleDesc
{
	bool	bEnabled = false;
	string	strDissolvePath;
	float	fTiling[2] = { 0.f , 0.f };
	float	fZoom = 1.5f;
	float	fPower = 1.f;
	float	fEdge = 0.1f;
	float	fEdgeColor[3] = { 0.f, 0.f, 0.f };
	float	fEdgeIntensity = 0.f;
	bool	bUseAge = true;
	float	fThreshold = 0.5f;
};

struct MaskModuleDesc
{
	bool	bEnabled = false;
	string	strMaskPath;
	bool	bClamp = true;					// Clamp / Wrap
	float	fPower = 2.f;					// 마스크 대비 강도
	float	fTiling[2] = { 1.f, 1.f };	// UV 타일링 수치
	float	fOffset[2] = { 0.f, 0.f };	// UV 위치 오프셋
};

struct PreviewMotionDesc
{
	bool	bEnabled = false;
	float	fCenter[3] = { 0.f, 0.f, 0.f };
	float	fRadius = 1.f;
	float	fSpeed = 1.f;
	float	fAxis[3] = { 0.f, 1.f, 0.f };
};

struct SkeletalEmitDesc
{
	bool			bEnabled = false;
	vector<string>	BoneName;
	bool			bInheritRotation = true;
	bool			bAttach = false;
};

struct EmitterDesc
{
	string					strName = "Emitter";
	float					fDotColor[4] = { 0.3f, 0.3f, 0.3f, 1.f };
	bool					bActive = true;

	TransformModuleDesc		tTransform;
	MaterialModuleDesc		tMaterial;
	UVDistortModuleDesc     tUVDistort;
	DistortionModuleDesc	tDistortion;
	EmissiveModuleDesc		tEmissive;
	MaskModuleDesc          tMask;
	DissolveModuleDesc		tDissolve;
	FresnelModuleDesc       tFresnel;
	Scale3DModuleDesc		tScale3D;
	MainModuleDesc			tMain;
	ShapeModuleDesc			tShape;
	RendererModuleDesc		tRenderer;
	VelocityModuleDesc		tVelocity;
	PhysicsModuleDesc		tPhysics;
	ColorModuleDesc			tColor;
	SizeModuleDesc			tSize;
	NoiseModuleDesc			tNoise;
	VectorFieldModuleDesc	tVectorField;
	TextureSheetModuleDesc	TextureSheet;
	TrailModuleDesc			tTrail;
	RotOverLifeModuleDesc	tRotOverLife;
	SpeedOverLifeModuleDesc tSpeedOverLife;
	SkeletalEmitDesc		tSkeletal;
	PreviewMotionDesc		tPreview;
};

// 데칼 구조체 선언
// 이펙트 데칼 - 수명 따라 페이드되는 데칼 | 맵 데칼 - 영구적으로 지형에 붙어있는 데칼
struct DecalDesc
{
	string					strName = "Decal";
	bool					bActive = true;

	// 투영 박스 (로컬 단위 큐브를 이 스케일/회전/오프셋으로 배치
	float					fBoxSize[3] = { 1.f, 1.f, 1.f };	// 투영 크기
	float					fOffset[3] = { 0.f, 0.f, 0.f };		// 위치 오프셋
	float					fRotation[3] = { 0.f, 0.f, 0.f };	// 투영 방향

	// 각도 페이드 - 표면의 노말과 데칼의 투영축 각도로 알파 페이드를 통해서 측면이 늘어지는 현상을 방지한다.
	float					fAngleFadeStart = 30.f;
	float					fAngleFadeEnd = 80.f;

	// 텍스쳐 바인딩
	string					strAlbedoPath;
	string					strNormalPath;
	string					strORMSPath;		// Occlusion | Roughness | Metal | Specular
	string					strEmissivePath;
	string					strMaskPath;

	// GBuffer 채널 별 블렌드 가중치
	float					fAlbedoWeight = 1.f;
	float					fNormalWeight = 1.f;
	float					fORMSWeight = 0.f;
	float					fEmissiveWeight = 0.f;

	float					fTint[4] = { 1.f, 1.f, 1.f, 1.f };
	float					fEmissiveIntensity = 1.f;

	float					fTiling[2] = { 1.f, 1.f };
	float					fUVOffset[2] = { 0.f, 0.f };

	// 정렬 우선순위 (겹칠 때 지정)
	int						iSortPriority = 0;

	bool					bUseLifetime = true;
	float					fStartDelay = 0.f;
	float					fLifeTime = 1.5f;
	vector<FloatKeyframe>	vFadeCurve = { { 0.f, 0.f, 0 }, { 0.1f, 1.f, 0 }, { 1.f, 0.f, 0 } };
};

struct EffectDesc
{
	string				strName = "NewEffect";
	string				strFilePath;
	bool				bPositionOnly = false;
	vector<EmitterDesc> Emitters;
	vector<DecalDesc>	Decals;
};

struct PreviewSkeletonData
{
	bool			bLoaded = false;
	string			strModelName;
	vector<string>	BoneName;
	vector<int>		ParentIndices;
};

enum class SELECT_KIND { None, Emitter, Decal };

struct EffectEditorContext
{
	EffectDesc			tEffect;

	SELECT_KIND			eSelectKind = SELECT_KIND::None;
	int					iSelectedIndex = -1;

	bool				bPlaying = false;
	float				fPlayTime = 0.f;
	float				fTotalDuration = 2.f;

	PreviewSkeletonData	tPreviewSkeleton;

	bool IsEmitterSelected() const
	{
		return eSelectKind == SELECT_KIND::Emitter
			&& iSelectedIndex >= 0 && iSelectedIndex < static_cast<int>(tEffect.Emitters.size());
	}

	EmitterDesc& SelectedEmitter()
	{
		return tEffect.Emitters[iSelectedIndex];
	}

	bool IsDecalSelected() const
	{
		return eSelectKind == SELECT_KIND::Decal
			&& iSelectedIndex >= 0 && iSelectedIndex < static_cast<_int>(tEffect.Decals.size());
	}

	DecalDesc& SelectedDecal()
	{
		return tEffect.Decals[iSelectedIndex];
	}

	void Select_Emitter(int iIndex)
	{
		eSelectKind = (iIndex >= 0) ? SELECT_KIND::Emitter : SELECT_KIND::None;
		iSelectedIndex = iIndex;
	}

	void Select_Decal(int iIndex)
	{
		eSelectKind = (iIndex >= 0) ? SELECT_KIND::Decal : SELECT_KIND::None;
		iSelectedIndex = iIndex;
	}

	void Clear_Selection()
	{
		eSelectKind = SELECT_KIND::None;
		iSelectedIndex = -1;
	}

	void RefreshTotalDuration()
	{
		fTotalDuration = 0.f;
		for (const auto& tEmit : tEffect.Emitters)
			fTotalDuration = max(fTotalDuration, tEmit.tMain.fStartDelay + tEmit.tMain.fDuration);
		for (const auto& tDecal : tEffect.Decals)
			if (tDecal.bUseLifetime && tDecal.fLifeTime > 0.f)
				fTotalDuration = max(fTotalDuration, tDecal.fStartDelay + tDecal.fLifeTime);
		if (fTotalDuration <= 0.f)
			fTotalDuration = 2.f;
	}
};

NS_END