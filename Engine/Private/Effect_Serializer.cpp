#include "Effect_Serializer.h"
#include "GameInstance.h"
#include "Engine_Utility.h"

using ejson = nlohmann::ordered_json;

NS_BEGIN(Engine)

namespace
{
	template <size_t N>
	void Read_FloatArray(const ejson& j, const char* szKey, float(&out)[N])
	{
		auto it = j.find(szKey);
		if (it == j.end() || !it->is_array())
			return;
		const ejson& arr = *it;
		for (size_t i = 0; i < N && i < arr.size(); ++i)
			out[i] = arr[i].get<float>();
	}

	template <size_t N>
	ejson Write_FloatArray(const float(&in)[N])
	{
		ejson arr = ejson::array();
		for (size_t i = 0; i < N; ++i)
			arr.push_back(in[i]);
		return arr;
	}

	string Normalize_Sep(string s)
	{
		for (char& c : s)
		{
			if (c == '/')
				c = '\\';
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		}

		return s;
	}

	string Resource_Root()
	{
		if (CGameInstance* pGameInstance = CGameInstance::Get_Instance())
		{
			const _wstring& strResourcePath = pGameInstance->Get_ResourcePath();
			if (!strResourcePath.empty())
				return WStringToString(strResourcePath);
		}
		return string();
	}

	// 절대경로를 리소스 루트 기준 상대 경로로 변경할 때 사용하는 함수(저장용). 루트 밖에 있다면 절대 경로를 유지함.
	string To_RelativePath(const string& strAbs)
	{
		if (strAbs.empty())
			return strAbs;

		string strRoot = Resource_Root();
		if (strRoot.empty())
			return strAbs;
		if (strRoot.back() != '\\' && strRoot.back() != '/')
			strRoot += '\\';

		const string nAbs = Normalize_Sep(strAbs);
		const string nRoot = Normalize_Sep(strRoot);

		if (nAbs.size() >= nRoot.size() && nAbs.compare(0, nRoot.size(), nRoot) == 0)
			return strAbs.substr(nRoot.size());

		return strAbs;
	}

	// 리소스 기준 상대경로를 절대 경로로 변환하는 함수(로드용), 이미 절대경로라면 그대로 리턴.
	string To_AbsolutePath(const string& strRel)
	{
		if (strRel.empty())
			return strRel;

		// 드라이브 문자(:) 또는 경로 지정자 (\\)이면 이미 절대 경로 이므로 그대로 반환
		if ((strRel.size() >= 2 && strRel[1] == ':') ||
			(strRel.size() >= 2 && (strRel[0] == '\\' || strRel[0] == '/') &&
			(strRel[1] == '\\' || strRel[1] == '/')))
			return strRel;

		string strRoot = Resource_Root();
		if (strRoot.empty())
			return strRel;
		if (strRoot.back() != '\\' && strRoot.back() != '/')
			strRoot += '\\';

		return strRoot + strRel;
	}
}

static void to_json(ejson& j, const FloatKeyframe& k)
{
	j = ejson{ { "fTime", k.fTime }, { "fValue", k.fValue }, { "iInterp", k.iInterp } };
}

static void from_json(const ejson& j, FloatKeyframe& k)
{
	k.fTime = j.value("fTime", k.fTime);
	k.fValue = j.value("fValue", k.fValue);
	k.iInterp = j.value("iInterp", k.iInterp);
}

static void to_json(ejson& j, const ColorKeyframe& k)
{
	j = ejson{ { "fTime", k.fTime }, { "fColor", Write_FloatArray(k.fColor) }, { "iInterp", k.iInterp } };
}
static void from_json(const ejson& j, ColorKeyframe& k)
{
	k.fTime = j.value("fTime", k.fTime);
	Read_FloatArray(j, "fColor", k.fColor);
	k.iInterp = j.value("iInterp", k.iInterp);
}

static void to_json(ejson& j, const MainModuleDesc& d)
{
	j = ejson{
		{ "bRateEnabled", d.bRateEnabled }, { "fRateOverTime", d.fRateOverTime },
		{ "bBurstEnabled", d.bBurstEnabled }, { "iBurstMin", d.iBurstMin }, { "iBurstMax", d.iBurstMax },
		{ "fStartSpeedMin", d.fStartSpeedMin }, { "fStartSpeedMax", d.fStartSpeedMax },
		{ "fStartSizeMin", d.fStartSizeMin }, { "fStartSizeMax", d.fStartSizeMax },
		{ "fStartRotMin", d.fStartRotMin }, { "fStartRotMax", d.fStartRotMax },
		{ "iMaxParticles", d.iMaxParticles },
		{ "fLifeMin", d.fLifeMin }, { "fLifeMax", d.fLifeMax },
		{ "fStartDelay", d.fStartDelay }, { "fDuration", d.fDuration },
		{ "bLoop", d.bLoop }, { "iSimSpace", d.iSimSpace },
		{ "bInterpSpawn", d.bInterpSpawn }, { "bSplineSpawn", d.bSplineSpawn },
		{ "bInfinite", d.bInfinite },
		{ "iColorCurveMode", d.iColorCurveMode }, { "iSizeCurveMode", d.iSizeCurveMode },
		{ "iRotCurveMode", d.iRotCurveMode }, { "iSpeedCurveMode", d.iSpeedCurveMode }
	};
}

static void from_json(const ejson& j, MainModuleDesc& d)
{
	d.bRateEnabled = j.value("bRateEnabled", d.bRateEnabled);
	d.fRateOverTime = j.value("fRateOverTime", d.fRateOverTime);
	d.bBurstEnabled = j.value("bBurstEnabled", d.bBurstEnabled);
	d.iBurstMin = j.value("iBurstMin", d.iBurstMin);
	d.iBurstMax = j.value("iBurstMax", d.iBurstMax);
	d.fStartSpeedMin = j.value("fStartSpeedMin", d.fStartSpeedMin);
	d.fStartSpeedMax = j.value("fStartSpeedMax", d.fStartSpeedMax);
	d.fStartSizeMin = j.value("fStartSizeMin", d.fStartSizeMin);
	d.fStartSizeMax = j.value("fStartSizeMax", d.fStartSizeMax);
	d.fStartRotMin = j.value("fStartRotMin", d.fStartRotMin);
	d.fStartRotMax = j.value("fStartRotMax", d.fStartRotMax);
	d.iMaxParticles = j.value("iMaxParticles", d.iMaxParticles);
	d.fLifeMin = j.value("fLifeMin", d.fLifeMin);
	d.fLifeMax = j.value("fLifeMax", d.fLifeMax);
	d.fStartDelay = j.value("fStartDelay", d.fStartDelay);
	d.fDuration = j.value("fDuration", d.fDuration);
	d.bLoop = j.value("bLoop", d.bLoop);
	d.iSimSpace = j.value("iSimSpace", d.iSimSpace);
	d.bInterpSpawn = j.value("bInterpSpawn", d.bInterpSpawn);
	d.bSplineSpawn = j.value("bSplineSpawn", d.bSplineSpawn);
	d.bInfinite = j.value("bInfinite", d.bInfinite);
	d.iColorCurveMode = j.value("iColorCurveMode", d.iColorCurveMode);
	d.iSizeCurveMode = j.value("iSizeCurveMode", d.iSizeCurveMode);
	d.iRotCurveMode = j.value("iRotCurveMode", d.iRotCurveMode);
	d.iSpeedCurveMode = j.value("iSpeedCurveMode", d.iSpeedCurveMode);
}

static void to_json(ejson& j, const ShapeModuleDesc& d)
{
	j = ejson{
		{ "iShapeType", d.iShapeType }, { "fBiasDir", Write_FloatArray(d.fBiasDir) },
		{ "fBiasWeight", d.fBiasWeight }, { "fRadius", d.fRadius }, { "bEdgeEmit", d.bEdgeEmit },
		{ "strEmitMeshPath", d.strEmitMeshPath }, { "iEmitMeshIndex", d.iEmitMeshIndex },
		{ "fEmitMeshScale", d.fEmitMeshScale }, { "iEmitMeshSource", d.iEmitMeshSource },
		{ "bSurfaceRelease", d.bSurfaceRelease }, { "iSpawnSourceIndex", d.iSpawnSourceIndex}
	};
}
static void from_json(const ejson& j, ShapeModuleDesc& d)
{
	d.iShapeType = j.value("iShapeType", d.iShapeType);
	Read_FloatArray(j, "fBiasDir", d.fBiasDir);
	d.fBiasWeight = j.value("fBiasWeight", d.fBiasWeight);
	d.fRadius = j.value("fRadius", d.fRadius);
	d.bEdgeEmit = j.value("bEdgeEmit", d.bEdgeEmit);
	d.strEmitMeshPath = j.value("strEmitMeshPath", d.strEmitMeshPath);
	d.iEmitMeshIndex = j.value("iEmitMeshIndex", d.iEmitMeshIndex);
	d.fEmitMeshScale = j.value("fEmitMeshScale", d.fEmitMeshScale);
	d.iEmitMeshSource = j.value("iEmitMeshSource", d.iEmitMeshSource);
	d.bSurfaceRelease = j.value("bSurfaceRelease", d.bSurfaceRelease);
	d.iSpawnSourceIndex = j.value("iSpawnSourceIndex", d.iSpawnSourceIndex);
}

static void to_json(ejson& j, const RendererModuleDesc& d)
{
	j = ejson{
		{ "iAlignment", d.iAlignment }, { "iSortMode", d.iSortMode },
		{ "fEmiIntensity", d.fEmiIntensity },
		{ "fSphereHardness", d.fSphereHardness }, { "fDiffuseSharpness", d.fDiffuseSharpness }
		, { "strMeshPath", To_RelativePath(d.strMeshPath)},
		{ "bSoftParticle", d.bSoftParticle }, { "fSoftFadeDistance", d.fSoftFadeDistance },
		{ "fStretchScale", d.fStretchScale }, { "iMeshIndex", d.iMeshIndex },
		{ "iSpinMode", d.iSpinMode }, { "fSpinAxis", Write_FloatArray(d.fSpinAxis) },
		{ "fPivot", Write_FloatArray(d.fPivot) }
	};
}

static void from_json(const ejson& j, RendererModuleDesc& d)
{
	d.iAlignment = j.value("iAlignment", d.iAlignment);
	d.iSortMode = j.value("iSortMode", d.iSortMode);
	d.fEmiIntensity = j.value("fEmiIntensity", d.fEmiIntensity);
	d.fSphereHardness = j.value("fSphereHardness", d.fSphereHardness);
	d.fDiffuseSharpness = j.value("fDiffuseSharpness", d.fDiffuseSharpness);
	d.strMeshPath = To_AbsolutePath(j.value("strMeshPath", d.strMeshPath));
	d.bSoftParticle = j.value("bSoftParticle", d.bSoftParticle);
	d.fSoftFadeDistance = j.value("fSoftFadeDistance", d.fSoftFadeDistance);
	d.fStretchScale = j.value("fStretchScale", d.fStretchScale);
	d.iMeshIndex = j.value("iMeshIndex", d.iMeshIndex);
	d.iSpinMode = j.value("iSpinMode", d.iSpinMode);
	Read_FloatArray(j, "fSpinAxis", d.fSpinAxis);
	Read_FloatArray(j, "fPivot", d.fPivot);
}

static void to_json(ejson& j, const VelocityModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled }, { "fSpeedMin", d.fSpeedMin }, { "fSpeedMax", d.fSpeedMax },
		{ "fSpeedByAngle", d.fSpeedByAngle }, { "fOrbitVelocity", d.fOrbitVelocity },
		{ "fLinearVelocity", Write_FloatArray(d.fLinearVelocity) },
		{ "fOrbitCenter", Write_FloatArray(d.fOrbitCenter) },
		{ "fOrbitAxis", Write_FloatArray(d.fOrbitAxis) },
		{ "bSpiral", d.bSpiral },
		{ "bConverge", d.bConverge }, { "fArrivalRadius", d.fArivalRadius }
	};
}

static void from_json(const ejson& j, VelocityModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.fSpeedMin = j.value("fSpeedMin", d.fSpeedMin);
	d.fSpeedMax = j.value("fSpeedMax", d.fSpeedMax);
	d.fSpeedByAngle = j.value("fSpeedByAngle", d.fSpeedByAngle);
	d.fOrbitVelocity = j.value("fOrbitVelocity", d.fOrbitVelocity);
	Read_FloatArray(j, "fLinearVelocity", d.fLinearVelocity);
	Read_FloatArray(j, "fOrbitCenter", d.fOrbitCenter);
	Read_FloatArray(j, "fOrbitAxis", d.fOrbitAxis);
	d.bSpiral = j.value("bSpiral", d.bSpiral);
	d.bConverge = j.value("bConverge", d.bConverge);
	d.fArivalRadius = j.value("fArrivalRadius", d.fArivalRadius);
}

static void to_json(ejson& j, const PhysicsModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "fGravityStart", d.fGravityStart }, { "fGravityEnd", d.fGravityEnd }, { "fDrag", d.fDrag }
	};
}

static void from_json(const ejson& j, PhysicsModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.fGravityStart = j.value("fGravityStart", d.fGravityStart);
	d.fGravityEnd = j.value("fGravityEnd", d.fGravityEnd);
	d.fDrag = j.value("fDrag", d.fDrag);
}

static void to_json(ejson& j, const ColorModuleDesc& d)
{
	j = ejson{ { "bEnabled", d.bEnabled }, { "vKeyFrames", d.vKeyFrames } };
}

static void from_json(const ejson& j, ColorModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	if (j.contains("vKeyFrames"))
		d.vKeyFrames = j["vKeyFrames"].get<vector<ColorKeyframe>>();
}

static void to_json(ejson& j, const SizeModuleDesc& d)
{
	j = ejson{ { "bEnabled", d.bEnabled }, { "bMultiply", d.bMultiply }, { "vKeyframes", d.vKeyframes } };
}

static void from_json(const ejson& j, SizeModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.bMultiply = j.value("bMultiply", d.bMultiply);
	if (j.contains("vKeyframes"))
		d.vKeyframes = j["vKeyframes"].get<vector<FloatKeyframe>>();
}

static void to_json(ejson& j, const NoiseModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled }, { "iNoiseType", d.iNoiseType }, { "fStrength", d.fStrength },
		{ "fFrequency", d.fFrequency }, { "fDamping", d.fDamping },
		{ "fOffset", d.fOffset }, { "fScrollSpeed", d.fScrollSpeed }
	};
}

static void from_json(const ejson& j, NoiseModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.iNoiseType = j.value("iNoiseType", d.iNoiseType);
	d.fStrength = j.value("fStrength", d.fStrength);
	d.fFrequency = j.value("fFrequency", d.fFrequency);
	d.fDamping = j.value("fDamping", d.fDamping);
	d.fOffset = j.value("fOffset", d.fOffset);
	d.fScrollSpeed = j.value("fScrollSpeed", d.fScrollSpeed);
}

static void to_json(ejson& j, const TextureSheetModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled }, { "iRows", d.iRows }, { "iCols", d.iCols },
		{ "fFrameRate", d.fFrameRate }, { "bSeedBased", d.bSeedBased },
		{ "iPlayMode", d.iPlayMode },  { "iFixedFrame", d.iFixedFrame }
	};
}

static void to_json(ejson& j, const VectorFieldModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled }, { "strFieldPath", To_RelativePath(d.strFieldPath) },
		{ "fIntensity", d.fIntensity }, { "fTightness", d.fTightness },
		{ "fTileSize", Write_FloatArray(d.fTileSize) }
	};
}

static void from_json(const ejson& j, VectorFieldModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.strFieldPath = To_AbsolutePath(j.value("strFieldPath", d.strFieldPath));
	d.fIntensity = j.value("fIntensity", d.fIntensity);
	d.fTightness = j.value("fTightness", d.fTightness);
	Read_FloatArray(j, "fTileSize", d.fTileSize);
}

static void from_json(const ejson& j, TextureSheetModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.iRows = j.value("iRows", d.iRows);
	d.iCols = j.value("iCols", d.iCols);
	d.fFrameRate = j.value("fFrameRate", d.fFrameRate);
	d.bSeedBased = j.value("bSeedBased", d.bSeedBased);
	d.iPlayMode = j.value("iPlayMode", d.iPlayMode);
	d.iFixedFrame = j.value("iFixedFrame", d.iFixedFrame);
}

static void to_json(ejson& j, const TrailModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled }, { "fWidth", d.fWidth }, { "fMaxLength", d.fMaxLength },
		{ "fTailWidthScale", d.fTailWidthScale }, { "fTrailLife", d.fTrailLife },
		{ "fPointInterval", d.fPointInterval }, { "bSpline", d.bSpline },
		{ "fSinSpeed", d.fSinSpeed }, { "fWaveAmplitude", d.fWaveAmplitude },
		{ "bFadeOut", d.bFadeOut }, { "iTexMode", d.iTexMode },
		{ "strTexturePath", To_RelativePath(d.strTexturePath) },
		{ "bDualSocket", d.bDualSocket },
		{ "strNoiseTexPath", To_RelativePath(d.strNoiseTexPath) },
		{ "fNoiseTileU", d.fNoiseTileU }, { "fNoiseTileV", d.fNoiseTileV },
		{ "fNoiseSpeedX", d.fNoiseSpeedX }, { "fNoiseSpeedY", d.fNoiseSpeedY },
		{ "fBasePower", d.fBasePower }, { "fBaseDensity", d.fBaseDensity },
		{ "fBaseTileU", d.fBaseTileU }, { "fBaseTileV", d.fBaseTileV },
		{ "fBaseSpeedX", d.fBaseSpeedX }, { "fBaseSpeedY", d.fBaseSpeedY },
		{ "fNoisePower", d.fNoisePower }, { "fNoiseDensity", d.fNoiseDensity },
		{ "fEmiIntensity", d.fEmiIntensity },
		{ "bUseAlphaY", d.bUseAlphaY }, { "fAlphaYPower", d.fAlphaYPower }, { "fAlphaYPower2", d.fAlphaYPower2 },
		{ "iAlphaYMode", d.iAlphaYMode }, { "fAlphaYPivot", d.fAlphaYPivot },
		{ "iBlendMode", d.iBlendMode },
		{ "bUseDistortion", d.bUseDistortion }, { "fDistortion", d.fDistortion }
	};
}
static void from_json(const ejson& j, TrailModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.fWidth = j.value("fWidth", d.fWidth);
	d.fMaxLength = j.value("fMaxLength", d.fMaxLength);
	d.fTailWidthScale = j.value("fTailWidthScale", d.fTailWidthScale);
	d.fTrailLife = j.value("fTrailLife", d.fTrailLife);
	d.fPointInterval = j.value("fPointInterval", d.fPointInterval);
	d.bSpline = j.value("bSpline", d.bSpline);
	d.fSinSpeed = j.value("fSinSpeed", d.fSinSpeed);
	d.fWaveAmplitude = j.value("fWaveAmplitude", d.fWaveAmplitude);
	d.bFadeOut = j.value("bFadeOut", d.bFadeOut);
	d.iTexMode = j.value("iTexMode", d.iTexMode);
	d.strTexturePath = To_AbsolutePath(j.value("strTexturePath", d.strTexturePath));
	d.bDualSocket = j.value("bDualSocket", d.bDualSocket);
	d.strNoiseTexPath = To_AbsolutePath(j.value("strNoiseTexPath", d.strNoiseTexPath));
	d.fNoiseTileU = j.value("fNoiseTileU", d.fNoiseTileU);
	d.fNoiseTileV = j.value("fNoiseTileV", d.fNoiseTileV);
	d.fNoiseSpeedX = j.value("fNoiseSpeedX", d.fNoiseSpeedX);
	d.fNoiseSpeedY = j.value("fNoiseSpeedY", d.fNoiseSpeedY);
	d.fBasePower = j.value("fBasePower", d.fBasePower);
	d.fBaseDensity = j.value("fBaseDensity", d.fBaseDensity);
	d.fBaseTileU = j.value("fBaseTileU", d.fBaseTileU);
	d.fBaseTileV = j.value("fBaseTileV", d.fBaseTileV);
	d.fBaseSpeedX = j.value("fBaseSpeedX", d.fBaseSpeedX);
	d.fBaseSpeedY = j.value("fBaseSpeedY", d.fBaseSpeedY);
	d.fNoisePower = j.value("fNoisePower", d.fNoisePower);
	d.fNoiseDensity = j.value("fNoiseDensity", d.fNoiseDensity);
	d.fEmiIntensity = j.value("fEmiIntensity", d.fEmiIntensity);
	d.bUseAlphaY = j.value("bUseAlphaY", d.bUseAlphaY);
	d.fAlphaYPower = j.value("fAlphaYPower", d.fAlphaYPower);
	d.fAlphaYPower2 = j.value("fAlphaYPower2", d.fAlphaYPower2);
	d.fAlphaYPivot = j.value("fAlphaYPivot", d.fAlphaYPivot);
	if (j.contains("iAlphaYMode"))
		d.iAlphaYMode = j.value("iAlphaYMode", d.iAlphaYMode);
	else
		d.iAlphaYMode = j.value("bAlphaYDir", true) ? 1 : 0;
	d.iBlendMode = j.value("iBlendMode", d.iBlendMode);
	d.bUseDistortion = j.value("bUseDistortion", d.bUseDistortion);
	d.fDistortion = j.value("fDistortion", d.fDistortion);
}

static void to_json(ejson& j, const SpeedOverLifeModuleDesc& d)
{
	j = ejson{ { "bEnabled", d.bEnabled }, { "vKeyFrames", d.vKeyFrames } };
}
static void from_json(const ejson& j, SpeedOverLifeModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	if (j.contains("vKeyFrames"))
		d.vKeyFrames = j["vKeyFrames"].get<vector<FloatKeyframe>>();
}

static void to_json(ejson& j, const RotOverLifeModuleDesc& d)
{
	j = ejson{ { "bEnabled", d.bEnabled }, { "vKeyFrames", d.vKeyFrames } };
}
static void from_json(const ejson& j, RotOverLifeModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	if (j.contains("vKeyFrames"))
		d.vKeyFrames = j["vKeyFrames"].get<vector<FloatKeyframe>>();
}

static void to_json(ejson& j, const TransformModuleDesc& d)
{
	j = ejson{
		{ "fPosition", Write_FloatArray(d.fPosition) },
		{ "fRotation", Write_FloatArray(d.fRotation) },
		{ "fScale", Write_FloatArray(d.fScale) }
	};
}
static void from_json(const ejson& j, TransformModuleDesc& d)
{
	Read_FloatArray(j, "fPosition", d.fPosition);
	Read_FloatArray(j, "fRotation", d.fRotation);
	Read_FloatArray(j, "fScale", d.fScale);
}

static void to_json(ejson& j, const MaterialModuleDesc& d)
{
	j = ejson{
		{ "strDiffusePath", To_RelativePath(d.strDiffusePath) }, { "iBlendMode", d.iBlendMode },
		{ "bZWrite", d.bZWrite }, { "bZTest", d.bZTest }, { "bLinearOpacity", d.bLinearOpacity },
		{ "fBaseDensity", d.fBaseDensity}, { "fBasePower", d.fBasePower }
	};
}
static void from_json(const ejson& j, MaterialModuleDesc& d)
{
	d.strDiffusePath = To_AbsolutePath(j.value("strDiffusePath", d.strDiffusePath));
	d.iBlendMode = j.value("iBlendMode", d.iBlendMode);
	d.bZWrite = j.value("bZWrite", d.bZWrite);
	d.bZTest = j.value("bZTest", d.bZTest);
	d.bLinearOpacity = j.value("bLinearOpacity", d.bLinearOpacity);
	d.fBaseDensity = j.value("fBaseDensity", d.fBaseDensity);
	d.fBasePower = j.value("fBasePower", d.fBasePower);
}

static void to_json(ejson& j, const UVDistortModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "strNoiseAPath", To_RelativePath(d.strNoiseAPath) },
		{ "strNoiseBPath", To_RelativePath(d.strNoiseBPath) },
		{ "fPanningA", Write_FloatArray(d.fPanningA) },
		{ "fPanningB", Write_FloatArray(d.fPanningB) },
		{ "fTilingA", Write_FloatArray(d.fTilingA) },
		{ "fTilingB", Write_FloatArray(d.fTilingB) },
		{ "fStrength", d.fStrength }
	};
}
static void from_json(const ejson& j, UVDistortModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.strNoiseAPath = To_AbsolutePath(j.value("strNoiseAPath", d.strNoiseAPath));
	d.strNoiseBPath = To_AbsolutePath(j.value("strNoiseBPath", d.strNoiseBPath));
	Read_FloatArray(j, "fPanningA", d.fPanningA);
	Read_FloatArray(j, "fPanningB", d.fPanningB);
	Read_FloatArray(j, "fTilingA", d.fTilingA);
	Read_FloatArray(j, "fTilingB", d.fTilingB);
	d.fStrength = j.value("fStrength", d.fStrength);
}

static void to_json(ejson& j, const DistortionModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "fStrength", d.fStrength },
		{ "iMode", d.iMode },
		{ "bDepthMask", d.bDepthMask },
		{ "strNoisePath", To_RelativePath(d.strNoisePath) },
		{ "fPanning", Write_FloatArray(d.fPanning) },
		{ "fTiling", Write_FloatArray(d.fTiling) }
	};
}
static void from_json(const ejson& j, DistortionModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.fStrength = j.value("fStrength", d.fStrength);
	d.iMode = j.value("iMode", d.iMode);
	d.bDepthMask = j.value("bDepthMask", d.bDepthMask);
	d.strNoisePath = To_AbsolutePath(j.value("strNoisePath", d.strNoisePath));
	Read_FloatArray(j, "fPanning", d.fPanning);
	Read_FloatArray(j, "fTiling", d.fTiling);
}

static void to_json(ejson& j, const EmissiveModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "strEmissiveAPath", To_RelativePath(d.strEmissiveAPath) },
		{ "strEmissiveBPath", To_RelativePath(d.strEmissiveBPath) },
		{ "fTilingA", Write_FloatArray(d.fTilingA) }, { "fPanningA", Write_FloatArray(d.fPanningA) },
		{ "fTilingB", Write_FloatArray(d.fTilingB) }, { "fPanningB", Write_FloatArray(d.fPanningB) },
		{ "fStrA", d.fStrA }, { "fStr", d.fStr }, { "fPower", d.fPower },
		{ "fTint", Write_FloatArray(d.fTint) },
		{ "iBlendMode", d.iBlendMode }, {"fLerp", d.fLerp }, { "fUVNoiseStr", d.fUVNoiseStr }
	};
}
static void from_json(const ejson& j, EmissiveModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.strEmissiveAPath = To_AbsolutePath(j.value("strEmissiveAPath", d.strEmissiveAPath));
	d.strEmissiveBPath = To_AbsolutePath(j.value("strEmissiveBPath", d.strEmissiveBPath));
	Read_FloatArray(j, "fTilingA", d.fTilingA);
	Read_FloatArray(j, "fPanningA", d.fPanningA);
	Read_FloatArray(j, "fTilingB", d.fTilingB);
	Read_FloatArray(j, "fPanningB", d.fPanningB);
	d.fStrA = j.value("fStrA", d.fStrA);
	d.fStr = j.value("fStr", d.fStr);
	d.fPower = j.value("fPower", d.fPower);
	Read_FloatArray(j, "fTint", d.fTint);
	d.iBlendMode = j.value("iBlendMode", d.iBlendMode);
	d.fLerp = j.value("fLerp", d.fLerp);
	d.fUVNoiseStr = j.value("fUVNoiseStr", d.fUVNoiseStr);
}

static void to_json(ejson& j, const Scale3DModuleDesc& d)
{
	j = ejson{ { "bEnabled", d.bEnabled }, { "vKeyX", d.vKeyX }, { "vKeyY", d.vKeyY }, { "vKeyZ", d.vKeyZ } };
}
static void from_json(const ejson& j, Scale3DModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	if (j.contains("vKeyX")) d.vKeyX = j["vKeyX"].get<vector<FloatKeyframe>>();
	if (j.contains("vKeyY")) d.vKeyY = j["vKeyY"].get<vector<FloatKeyframe>>();
	if (j.contains("vKeyZ")) d.vKeyZ = j["vKeyZ"].get<vector<FloatKeyframe>>();
}

static void to_json(ejson& j, const FresnelModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "fOutlinePower", d.fOutlinePower }, { "fOutlineIntensity", d.fOutlineIntensity },
		{ "fInsideIntensity", d.fInsideIntensity }, { "fInverseStr", d.fInverseStr },
		{ "fColor", Write_FloatArray(d.fColor) },
		{ "strCrackPath", To_RelativePath(d.strCrackPath) }
	};
}
static void from_json(const ejson& j, FresnelModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.fOutlinePower = j.value("fOutlinePower", d.fOutlinePower);
	d.fOutlineIntensity = j.value("fOutlineIntensity", d.fOutlineIntensity);
	d.fInsideIntensity = j.value("fInsideIntensity", d.fInsideIntensity);
	d.fInverseStr = j.value("fInverseStr", d.fInverseStr);
	Read_FloatArray(j, "fColor", d.fColor);
	d.strCrackPath = To_AbsolutePath(j.value("strCrackPath", d.strCrackPath));
}

static void to_json(ejson& j, const DissolveModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "strDissolvePath", To_RelativePath(d.strDissolvePath) },
		{ "fTiling", Write_FloatArray(d.fTiling) },
		{ "fZoom", d.fZoom }, { "fPower", d.fPower }, { "fEdge", d.fEdge },
		{ "fEdgeColor", Write_FloatArray(d.fEdgeColor) }, { "fEdgeIntensity", d.fEdgeIntensity },
		{ "bUseAge", d.bUseAge }, { "fThreshold", d.fThreshold }
	};
}
static void from_json(const ejson& j, DissolveModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.strDissolvePath = To_AbsolutePath(j.value("strDissolvePath", d.strDissolvePath));
	Read_FloatArray(j, "fTiling", d.fTiling);
	d.fZoom = j.value("fZoom", d.fZoom);
	d.fPower = j.value("fPower", d.fPower);
	d.fEdge = j.value("fEdge", d.fEdge);
	Read_FloatArray(j, "fEdgeColor", d.fEdgeColor);
	d.fEdgeIntensity = j.value("fEdgeIntensity", d.fEdgeIntensity);
	d.bUseAge = j.value("bUseAge", d.bUseAge);
	d.fThreshold = j.value("fThreshold", d.fThreshold);
}

static void to_json(ejson& j, const MaskModuleDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "strMaskPath", To_RelativePath(d.strMaskPath) },
		{ "bClamp", d.bClamp },
		{ "fPower", d.fPower },
		{ "fTiling", Write_FloatArray(d.fTiling) },
		{ "fOffset", Write_FloatArray(d.fOffset) }
	};
}
static void from_json(const ejson& j, MaskModuleDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	d.strMaskPath = To_AbsolutePath(j.value("strMaskPath", d.strMaskPath));
	d.bClamp = j.value("bClamp", d.bClamp);
	d.fPower = j.value("fPower", d.fPower);
	Read_FloatArray(j, "fTiling", d.fTiling);
	Read_FloatArray(j, "fOffset", d.fOffset);
}

static void to_json(ejson& j, const SkeletalEmitDesc& d)
{
	j = ejson{
		{ "bEnabled", d.bEnabled },
		{ "BoneNames", d.BoneName },
		{ "bInheritRotation", d.bInheritRotation },
		{ "bAttach", d.bAttach }
	};
}
static void from_json(const ejson& j, SkeletalEmitDesc& d)
{
	d.bEnabled = j.value("bEnabled", d.bEnabled);
	if (j.contains("BoneNames") && j["BoneNames"].is_array())
		d.BoneName = j["BoneNames"].get<vector<string>>();
	d.bInheritRotation = j.value("bInheritRotation", d.bInheritRotation);
	d.bAttach = j.value("bAttach", d.bAttach);
}

static void to_json(ejson& j, const EmitterDesc& d)
{
	j = ejson{
		{ "strName", d.strName }, { "fDotColor", Write_FloatArray(d.fDotColor) }, { "bActive", d.bActive },
		{ "tTransform", d.tTransform }, { "tMaterial", d.tMaterial }, { "tUVDistort", d.tUVDistort },
		{ "tDistortion", d.tDistortion }, { "tEmissive", d.tEmissive }, { "tMask", d.tMask }, { "tDissolve", d.tDissolve },
		{ "tFresnel", d.tFresnel }, { "tScale3D", d.tScale3D },
		{ "tMain", d.tMain }, { "tShape", d.tShape }, { "tRenderer", d.tRenderer },
		{ "tVelocity", d.tVelocity }, { "tPhysics", d.tPhysics },
		{ "tColor", d.tColor }, { "tSize", d.tSize }, { "tNoise", d.tNoise }, { "tVectorField", d.tVectorField },
		{ "TextureSheet", d.TextureSheet }, { "tTrail", d.tTrail }, { "tRotOverLife", d.tRotOverLife },
		{ "tSpeedOverLife", d.tSpeedOverLife }, { "tSkeletal", d.tSkeletal }
	};
}
static void from_json(const ejson& j, EmitterDesc& d)
{
	d.strName = j.value("strName", d.strName);
	Read_FloatArray(j, "fDotColor", d.fDotColor);
	d.bActive = j.value("bActive", d.bActive);

	d.tTransform = j.value("tTransform", d.tTransform);
	d.tMaterial = j.value("tMaterial", d.tMaterial);
	d.tUVDistort = j.value("tUVDistort", d.tUVDistort);
	d.tDistortion = j.value("tDistortion", d.tDistortion);
	d.tEmissive = j.value("tEmissive", d.tEmissive);
	d.tMask = j.value("tMask", d.tMask);
	d.tDissolve = j.value("tDissolve", d.tDissolve);
	d.tFresnel = j.value("tFresnel", d.tFresnel);
	d.tScale3D = j.value("tScale3D", d.tScale3D);
	d.tMain = j.value("tMain", d.tMain);
	d.tShape = j.value("tShape", d.tShape);
	d.tRenderer = j.value("tRenderer", d.tRenderer);
	d.tVelocity = j.value("tVelocity", d.tVelocity);
	d.tPhysics = j.value("tPhysics", d.tPhysics);
	d.tColor = j.value("tColor", d.tColor);
	d.tSize = j.value("tSize", d.tSize);
	d.tNoise = j.value("tNoise", d.tNoise);
	d.tVectorField = j.value("tVectorField", d.tVectorField);
	d.TextureSheet = j.value("TextureSheet", d.TextureSheet);
	d.tTrail = j.value("tTrail", d.tTrail);
	d.tRotOverLife = j.value("tRotOverLife", d.tRotOverLife);
	d.tSpeedOverLife = j.value("tSpeedOverLife", d.tSpeedOverLife);
	d.tSkeletal = j.value("tSkeletal", d.tSkeletal);
}

static void to_json(ejson& j, const DecalDesc& d)
{
	j = ejson{
		{ "strName", d.strName }, { "bActive", d.bActive },
		{ "fBoxSize", Write_FloatArray(d.fBoxSize) }, { "fOffset", Write_FloatArray(d.fOffset) },
		{ "fRotation", Write_FloatArray(d.fRotation) },
		{ "fAngleFadeStart", d.fAngleFadeStart }, { "fAngleFadeEnd", d.fAngleFadeEnd },
		{ "strAlbedoPath", To_RelativePath(d.strAlbedoPath) }, { "strNormalPath", To_RelativePath(d.strNormalPath) },
		{ "strORMSPath", To_RelativePath(d.strORMSPath) }, { "strEmissivePath", To_RelativePath(d.strEmissivePath) }, { "strMaskPath", To_RelativePath(d.strMaskPath) },
		{ "fAlbedoWeight", d.fAlbedoWeight }, { "fNormalWeight", d.fNormalWeight },
		{ "fORMSWeight", d.fORMSWeight }, { "fEmissiveWeight", d.fEmissiveWeight },
		{ "fTint", Write_FloatArray(d.fTint) }, { "fEmissiveIntensity", d.fEmissiveIntensity },
		{ "fTiling", Write_FloatArray(d.fTiling) }, { "fUVOffset", Write_FloatArray(d.fUVOffset) },
		{ "iSortPriority", d.iSortPriority },
		{ "bUseLifetime", d.bUseLifetime }, { "fStartDelay", d.fStartDelay }, { "fLifeTime", d.fLifeTime },
		{ "vFadeCurve", d.vFadeCurve }
	};
}

static void from_json(const ejson& j, DecalDesc& d)
{
	d.strName = j.value("strName", d.strName);
	d.bActive = j.value("bActive", d.bActive);
	Read_FloatArray(j, "fBoxSize", d.fBoxSize);
	Read_FloatArray(j, "fOffset", d.fOffset);
	Read_FloatArray(j, "fRotation", d.fRotation);
	d.fAngleFadeStart = j.value("fAngleFadeStart", d.fAngleFadeStart);
	d.fAngleFadeEnd = j.value("fAngleFadeEnd", d.fAngleFadeEnd);
	d.strAlbedoPath = To_AbsolutePath(j.value("strAlbedoPath", d.strAlbedoPath));
	d.strNormalPath = To_AbsolutePath(j.value("strNormalPath", d.strNormalPath));
	d.strORMSPath = To_AbsolutePath(j.value("strORMSPath", d.strORMSPath));
	d.strEmissivePath = To_AbsolutePath(j.value("strEmissivePath", d.strEmissivePath));
	d.strMaskPath = To_AbsolutePath(j.value("strMaskPath", d.strMaskPath));
	d.fAlbedoWeight = j.value("fAlbedoWeight", d.fAlbedoWeight);
	d.fNormalWeight = j.value("fNormalWeight", d.fNormalWeight);
	d.fORMSWeight = j.value("fORMSWeight", d.fORMSWeight);
	d.fEmissiveWeight = j.value("fEmissiveWeight", d.fEmissiveWeight);
	Read_FloatArray(j, "fTint", d.fTint);
	d.fEmissiveIntensity = j.value("fEmissiveIntensity", d.fEmissiveIntensity);
	Read_FloatArray(j, "fTiling", d.fTiling);
	Read_FloatArray(j, "fUVOffset", d.fUVOffset);
	d.iSortPriority = j.value("iSortPriority", d.iSortPriority);
	d.bUseLifetime = j.value("bUseLifetime", d.bUseLifetime);
	d.fStartDelay = j.value("fStartDelay", d.fStartDelay);
	d.fLifeTime = j.value("fLifeTime", d.fLifeTime);
	if (j.contains("vFadeCurve"))
		d.vFadeCurve = j["vFadeCurve"].get<vector<FloatKeyframe>>();
}

static void to_json(ejson& j, const EffectDesc& d)
{
	j = ejson {
		{ "version", 1 },
		{ "strName", d.strName },
		{ "bPositionOnly", d.bPositionOnly },
		{ "Emitters", d.Emitters },
		{ "Decals", d.Decals }
	};
}
static void from_json(const ejson& j, EffectDesc& d)
{
	d.strName = j.value("strName", d.strName);
	d.bPositionOnly = j.value("bPositionOnly", d.bPositionOnly);
	if (j.contains("Emitters"))
		d.Emitters = j["Emitters"].get<vector<EmitterDesc>>();
	if (j.contains("Decals"))
		d.Decals = j["Decals"].get<vector<DecalDesc>>();
}

HRESULT Save_Effect(const EffectDesc& tEffect, const string& strPath)
{
	ofstream file(strPath, ios::trunc);
	if (!file.is_open())
		return E_FAIL;

	try
	{
		ejson j = tEffect;
		file << j.dump(4);
	}
	catch (const exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT Load_Effect(EffectDesc& tOut, const string& strPath)
{
	ifstream file(strPath);
	if (!file.is_open())
		return E_FAIL;

	try
	{
		ejson j;
		file >> j;
		tOut = j.get<EffectDesc>();
	}
	catch (const exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

NS_END