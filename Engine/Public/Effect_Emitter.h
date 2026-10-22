#pragma once
#include "PoolableObject.h"
#include "Effect_Define.h"

NS_BEGIN(Engine)

class CStructuredBuffer;
class CComputeShader;
class CVIBuffer_GPUParticle;
class CVIBuffer_GPUMesh;
class CTexture;

// 모든 Emitter가 공유하는 Shader 집합. Effect_Manager에서 최초 컴파일 후, Emitter가 가져옴.
struct EMITTER_SHADER_BUNDLE
{
	ID3D11VertexShader*	pVS = { nullptr };
	ID3D11PixelShader*	pPS = { nullptr };
	ID3D11PixelShader*	pPS_WBOIT = { nullptr };
	ID3D11VertexShader* pMeshVS = { nullptr };
	ID3D11PixelShader*	pMeshPS = { nullptr };
	ID3D11PixelShader*	pMeshPS_WBOIT = { nullptr };
	ID3D11InputLayout*	pMeshLayout = { nullptr };
	CComputeShader*		pSimulateCS = { nullptr };
	CComputeShader*		pInitCS = { nullptr };
	CComputeShader*		pEmitCS = { nullptr };
	CComputeShader*		pSortCS = { nullptr };
	CComputeShader*		pMeshSkinCS = { nullptr };
	ID3D11VertexShader* pTrailVS = { nullptr };
	ID3D11PixelShader*	pTrailPS = { nullptr };
	ID3D11PixelShader*	pTrailPS_WBOIT = { nullptr };
	ID3D11InputLayout*	pTrailLayout = { nullptr };
	ID3D11VertexShader* pDecalVS = { nullptr };
	ID3D11PixelShader*	pDecalPS = { nullptr };
	ID3D11InputLayout*	pDecalLayout = { nullptr };
};

struct EFFECT_EMITTER_DESC
{
	_float		fRate = { 100.f };
	_float		fLifeMin = { 0.8f };
	_float		fLifeMax = { 1.6f };
	_float		fSpeedMin = { 1.f };
	_float		fSpeedMax = { 3.f };
	_float		fSizeMin = { 0.1f };
	_float		fSizeMax = { 0.1f };
	_float		fShapeRadius = { 0.3f };
	_uint		iShapeType = { 0 };
	_float3		vBiasDir = { 0.f, 1.f, 0.f };
	_float		fBiasWeight = { 0.5f };
	_bool		bEdgeEmit = { false };
	_bool		bSurfaceRelease = { false };
	_float3		vGravity = { 0.f, 1.f, 0.f };
	_float		fDrag = { 0.f };
	_float4		vColor = { 0.2f, 0.8f, 1.f, 1.f };
	_float3		vEmitPos = { 0.f, 0.f, 0.f };
	_float3		vEmitRot = { 0.f, 0.f, 0.f };
	_uint		iMaxParticles = { 8192 };

	_bool		bInfinite = { false };
	_uint		iColorCurveMode = { 2 };
	_uint		iSizeCurveMode = { 2 };
	_uint		iRotCurveMode = { 2 };
	_uint		iSpeedCurveMode = { 2 };

	_bool		bNoise = { false };
	_uint		iNoiseType = { 0 };
	_float		fNoiseStrength = { 1.f };
	_float		fNoiseFrequency = { 1.f };
	_float		fNoiseDamping = { 0.f };
	_float		fNoiseScroll = { 0.f };

	_bool		bFlipbook = { false };
	_uint		iFlipCols = { 1 };
	_uint		iFlipRows = { 1 };
	_float		fFlipFrameRate = { 16.f };
	_bool		bFlipSeedBased = { false };
	_uint		iFlipPlayMode = { 0 };
	_uint		iFlipFixedFrame = { 0 };

	_bool		bSoftParticle = { false };
	_float		fSoftFade = { 0.5f };

	_uint		iAlignment = { 0 };
	_float		fStretchScale = { 0.1f };

	_uint		iSpinMode = { 0 };
	_float3		vSpinAxis = { 0.f, 1.f, 0.f };
	_float3		vMeshScale = { 1.f, 1.f, 1.f };
	_uint		iMeshIndex = { 0 };
	_float2		vPivot = { 0.5f, 0.5f };

	// Velocity Module
	_bool		bVelocity = { false };
	_float3		vLinearVel = { 0.f, 0.f, 0.f };
	_float		fOrbit = { 0.f };
	_float3		vOrbitCenter = { 0.f, 0.f, 0.f };
	_float3		vOrbitAxis = { 0.f, 1.f, 0.f };
	_bool		bSpiral = { false };
	_bool		bConverge = { false };
	_float		fArrivalRadius = { 0.1f };

	_uint		iSimSpace = { 0 };
	_bool		bInterpSpawn = { false };
	_bool       bSplineSpawn = { false };

	_float		fStartRotMin = { 0.f };
	_float		fStartRotMax = { 0.f };
	_float		fEmiIntensity = { 1.f };
	_float		fSphereHardness = { 0.5f };
	_float		fBaseDensity = { 1.f };
	_float		fBasePower = { 1.f };
	_float		fDiffuseSharpness = { 1.f };

	_float		fGravStart = { 0.f };
	_float		fGravEnd = { -9.8f };
	_float		fVelSpeedMin = { 0.f };
	_float		fVelSpeedMax = { 0.f };
	_float		fSpeedByAngle = { 0.f };
	_float		fNoiseOffset = { 0.f };
	_bool		bSizeMultiply = { false };

	_bool		bUVDistort = { false };
	_float2		vUVPanA = { 0.f, 0.f };
	_float2		vUVPanB = { 0.f, 0.f };
	_float		fUVDistortStr = { 0.1f };
	_float2		vNoiseTileA = { 1.f, 1.f };
	_float2		vNoiseTileB = { 1.f, 1.f };

	_bool		bDistort = { false };
	_float		fDistortStr = { 10.f };
	_uint		iDistortMode = { 0 };
	_bool		bDistortDepthMask = { true };
	_float2		vDistortPan = { 0.05f, 0.05f };
	_float2		vDistortTile = { 1.f, 1.f };

	_bool		bEmissive = { false };
	_float2		vEmisTileA = { 0.f, 0.f };
	_float2		vEmisPanA = { 0.f, 0.f };
	_float2		vEmisTileB = { 0.f, 0.f };
	_float2		vEmisPanB = { 0.f, 0.f };
	_float		fEmisStrA = { 0.f };
	_float		fEmisStr = { 0.f };
	_float		fEmisPower = { 1.f };
	_float3		vEmisTint = { 1.f, 1.f, 1.f };
	_float		fEmisLerp = { 0.f };
	_float		fEmisUVNoiseStr = { 0.f };
	_uint		iEmisBlendMode = { 0 };

	_bool		bLinearOpacity = { false };
	_bool		bMask = { false };
	_bool		bMaskClamp = { true };
	_float		fMaskPower = { 2.f };
	_float2		vMaskTiling = { 1.f, 1.f };
	_float2		vMaskOffset = { 0.f, 0.f };

	_bool		bDissolve = { false };
	_float2		vDissolveTiling = { 0.3f, 4.f };
	_float		fDissolveZoom = { 1.5f };
	_float		fDissolvePower = { 1.f };
	_float		fDissolveEdge = { 0.1f };
	_float3		vDissolveEdgeColor = { 1.f, 0.5f, 0.2f };
	_float		fDissolveEdgeInt = { 0.f };
	_float		fDissolveThreshold = { 0.5f };
	_bool		bDissolveUseAge = { true };

	_bool		bFresnel = { false };
	_float		fFresPower = { 6.f };
	_float		fFresOutInt = { 5.f };
	_float		fFresInInt = { 0.f };
	_float		fFresInverseStr = { 10.f };
	_float3		vFresColor = { 1.f, 1.f, 1.f };

	_bool		bScale3D = { false };
};

class ENGINE_DLL CEffect_Emitter : public CPoolableObject
{
public:
	static const _uint LUT_SIZE = 64;

private:
	CEffect_Emitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CEffect_Emitter() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void	Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

public:
	static HRESULT	Build_SharedShaders(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, EMITTER_SHADER_BUNDLE& Out);
	static void		Free_SharedShaders(EMITTER_SHADER_BUNDLE& InOut);

	void			Bind_SharedShaders(const EMITTER_SHADER_BUNDLE* pBundle);

public:
	void			Set_Desc(const EFFECT_EMITTER_DESC& tDesc) { m_tDesc = tDesc; }

	void			Set_ColorCurve(const _float4* pColors, _bool bEnabled);
	void			Set_SizeCurve(const _float* pSizes, _bool bEnabled);
	void			Set_RotCurve(const _float* pRotDegrees, _bool bEnabled);
	void			Set_SpeedCurve(const _float* pSpeeds, _bool bEnabled);
	void			Set_Scale3DCurve(const _float3* pSacle, _bool bEnabled);
	_bool			Curves_Changed_Update(
							const vector<ColorKeyframe>& color, _bool bColorEn,
							const vector<FloatKeyframe>& size, _bool bSizeEn,
							const vector<FloatKeyframe>& rot, _bool bRotEn,
							const vector<FloatKeyframe>& sx, const vector<FloatKeyframe>& sy,
							const vector<FloatKeyframe>& sz, _bool bScale3DEn,
							const vector<FloatKeyframe>& speed, _bool bSpeedEn);
	void			Reset();

	void			Request_Burst(_uint iCount) { m_iPendingBurst += iCount; }

	void			Set_Material(_uint iBlendMode, _bool bZWrite, _bool bZTest, const string& strDiffusePath);

	void			Set_UVDistort(const string& strNoiseAPath, const string& strNoiseBPath);
	void			Set_SceneSRV(ID3D11ShaderResourceView* pSRV) { m_pSceneSRV = pSRV; }
	void			Set_DistortNoise(const string& strDistortPath);
	void            Set_Emissive(const string& strEmissiveAPath, const string& strEmissiveBPath);
	void			Set_Mask(const string& strMaskPath);
	void			Set_Mesh(const string& strMeshPath, _uint iMeshIndex);
	void			Set_EmitMesh(const string& strMeshPath, _uint iMeshIndex, _float fScale, _bool bEnabled);
	void			Set_EmitSkinnedMesh(const string& strMeshPath, _int iMeshIndex, _bool bEnabled);
	void			Set_EmitSkinnedMeshGroups(const vector<string>& GroupPath, _bool bEnabled);
	void			Set_EmitMeshSkinSource(ID3D11ShaderResourceView* pPaletteSRV, const _uint* pGroupBases, _uint iGroupCount, _uint iPaletteCapacity, const _float4x4& MatWorld);
	void            Set_Dissolve(const string& strDissolvePath);
	void            Set_Fresnel(const string& strCrackPath);

	_uint			Get_BlendMode() const { return m_iBlendMode; }

	virtual void	Activate(void* pArg = nullptr) override;
	virtual void	InActivate() override;

	void            Set_VectorField(const string& strPath, _bool bEnabled, _float fIntensity, _float fTightness, const _float3& vTileSize);
	void			Set_EmitPos(const _float3& vPos) { m_tDesc.vEmitPos = vPos; }
	void            Set_EmitArc(const _float3& vAxis, _float fAngle, _bool bEnabled) { m_vArcAxis = vAxis; m_fArcAngle = fAngle; m_bArcEnable = bEnabled; }
	void			Set_OrbitCenter(const _float3& vCenter) { m_tDesc.vOrbitCenter = vCenter; }
	void            Set_EmitExtraRot(const _float4x4& matRot) { m_matExtraRot = matRot; }

	// 선택한 본들의 월드 행렬을 넘겨서 이미터를 본의 위치에서 나올 수 있도록 한다.
	void			Set_BoneSources(const _float4x4* pWorldMats, _uint iCount, _bool bInheritRot, _bool bAttach = false);

	void			Set_SpawnSource(ID3D11ShaderResourceView* pPoolSRV, ID3D11ShaderResourceView* pAliveSRV, 
		ID3D11ShaderResourceView* pAliveCountSRV, _bool bEnabled);
	ID3D11ShaderResourceView*		Get_PoolSRV() const;
	ID3D11ShaderResourceView*		Get_AliveSRV() const;
	ID3D11ShaderResourceView*		Get_AliveCountSRV() const { return m_pAliveCountSRV;}

private:
	HRESULT			Ready_Pool_CPU();
	HRESULT			Ready_ComputeAndShaders();
	HRESULT			Ready_MeshMode();
	HRESULT			Ready_LifecycleBuffers();
	HRESULT			Create_ListBuffers();
	HRESULT			Resize_Pool(_uint iNewCount);
	void			Update_CameraCB();
	void			Update_RenderCB();
	void			Dispatch_Lifecycle(_float fTimeDelta);
	CTexture*		Load_Cached_Texture(const wstring& strWPath);
	CTexture*		LoadTexture(const string& strPath);

private:
	_uint						m_iMaxCount = { 2048 };
	_uint						m_iCapacity = { 2048 };

	CStructuredBuffer*			m_pPool = { nullptr };
	CComputeShader*				m_pSimulateCS = { nullptr };
	CVIBuffer_GPUParticle*		m_pWrapper = { nullptr };

	CVIBuffer_GPUMesh*			m_pMeshWrapper = { nullptr };
	ID3D11VertexShader*			m_pMeshVS = { nullptr };
	ID3D11PixelShader*			m_pMeshPS = { nullptr };
	ID3D11PixelShader*			m_pMeshPS_WBOIT = { nullptr };
	ID3D11InputLayout*			m_pMeshLayout = { nullptr };
	ID3D11Buffer*				m_pMeshIndirectArgs = { nullptr };

	ID3D11Buffer*				m_pSimCB = { nullptr };
	ID3D11Buffer*				m_pCameraCB = { nullptr };
	ID3D11Buffer*				m_pRenderCB = { nullptr };

	ID3D11VertexShader*			m_pVS = { nullptr };
	ID3D11PixelShader*			m_pPS = { nullptr };
	ID3D11PixelShader*			m_pPS_WBOIT = { nullptr };

	CStructuredBuffer*			m_pDeadList = { nullptr }; // 빈 슬롯
	CStructuredBuffer*			m_pAliveList = { nullptr }; // 실제 살아있는 파티클들이 저장될 곳
	ID3D11Buffer*				m_pIndirectArgs = { nullptr }; // DrawInstancedIndirect 인자로 전달할 버퍼
	CComputeShader*				m_pInitCS = { nullptr };
	CComputeShader*				m_pEmitCS = { nullptr };
	ID3D11Buffer*				m_pEmitCB = { nullptr };

	_bool						m_bInitialized = { false };
	_float						m_fSpawnRate = { 400.f };
	_float						m_fSpawnAccum = { 0.f };
	_float						m_fEmitTime = { 0.f };
	_uint						m_iPendingBurst = { 0 };

	ID3D11Buffer*				m_pDeadCountBuf = { nullptr };
	ID3D11ShaderResourceView*	m_pDeadCountSRV = { nullptr };

	// GPU 정렬 용 버퍼
	CComputeShader*				m_pSortCS = { nullptr };
	CStructuredBuffer*			m_pSortedList = { nullptr }; // 정렬된 인덱스
	ID3D11Buffer*				m_pAliveCountBuf = { nullptr }; // AliveList 개수
	ID3D11ShaderResourceView*	m_pAliveCountSRV = { nullptr };
	_bool						m_bSortDrawing = { false }; // 현재 정렬 리스트로 드로우 중인지 판별

	CStructuredBuffer*			m_pColorLUT = { nullptr };
	CStructuredBuffer*			m_pSizeLUT = { nullptr };
	CStructuredBuffer*			m_pRotLUT = { nullptr };
	CStructuredBuffer*			m_pSpeedLUT = { nullptr };
	CStructuredBuffer*			m_pScale3DLUT = { nullptr };
	_bool						m_bColorOverLife = { false };
	_bool						m_bSizeOverLife = { false };
	_bool						m_bRotOverLife = { false };
	_bool						m_bScale3DOverLife = { false };
	_bool						m_bSpeedOverLife = { false };

	// LUT 업데이트 캐시 (커브가 바뀔 때만 UpdateResource를 호출하게 하여 최적화)
	_float4						m_CachedColorLUT[LUT_SIZE] = {};
	_float						m_CachedSizeLUT[LUT_SIZE] = {};
	_float						m_CachedRotLUT[LUT_SIZE] = {};
	_float						m_CachedSpeedLUT[LUT_SIZE] = {};
	_float3						m_CachedScale3DLUT[LUT_SIZE] = {};
	_bool						m_bColorLUTUpdated = { false };
	_bool						m_bSizeLUTUpdated = { false };
	_bool						m_bRotLUTUpdated = { false };
	_bool						m_bSpeedLUTUpdated = { false };
	_bool						m_bScale3DLUTUpdated = { false };

	// 커브 변경 감지 캐시 (직전 적용한 키프레임/활성 플래그)
	vector<ColorKeyframe>		m_LastColorKeys;
	vector<FloatKeyframe>		m_LastSizeKeys, m_LastRotKeys, m_LastSpeedKeys, m_LastSXKeys, m_LastSYKeys, m_LastSZKeys;
	_bool						m_bLastColorEn = { false }, m_bLastSizeEn = { false };
	_bool						m_bLastRotEn = { false }, m_bLastSpeedEn = { false }, m_bLastScale3DEn = { false };
	_bool						m_bCurvesInit = { false };

	string						m_strMeshPath = { "<none>" };
	_uint						m_iMeshIndex = { 0 };

	CTexture*					m_pDiffuseTex = { nullptr };
	string						m_strDiffusePath = {};
	_uint						m_iBlendMode = { 1 };
	_bool						m_bZWrite = { false };
	_bool						m_bZTest = { false };
	_bool						m_bUseTexture = { false };

	CTexture*					m_pNoiseATex = { nullptr };
	CTexture*					m_pNoiseBTex = { nullptr };
	string						m_strNoiseAPath = {};
	string						m_strNoiseBPath = {};

	ID3D11ShaderResourceView*	m_pSceneSRV = { nullptr };

	CTexture*					m_pShapeMaskTex = { nullptr };
	string						m_strShapeMaskPath = {};

	CTexture*					m_pDistortNoiseTex = { nullptr };
	string						m_strDistortNoisePath = {};

	CTexture*					m_pEmissiveATex = { nullptr };
	string						m_strEmissiveAPath = {};

	CTexture*					m_pEmissiveBTex = { nullptr };
	string                      m_strEmissiveBPath = {};

	CTexture*					m_pDissolveTex = { nullptr };
	string                      m_strDissolvePath = {};

	CTexture*					m_pCrackTex = { nullptr };
	string                      m_strCrackPath = {};

	EFFECT_EMITTER_DESC			m_tDesc = {};
	_float4x4					m_matExtraRot = {};

	// Bone 관련 소스 버퍼, 본 개수만큼 매 프레임 갱신함.
	CStructuredBuffer*			m_pBoneSrcBuf = { nullptr };
	_uint						m_iBoneSrcCapacity = { 0 };
	_uint						m_iBoneCount = { 0 };
	_bool						m_bBoneEmit = { false };
	_bool						m_bBoneInheritRot = { true };
	_bool						m_bBoneAttach = { false };
	_uint						m_iBoneFrameSeed = { 0 };
	vector<_float4x4>			m_BoneUploadScratch;

	_float3						m_vPrevEmitPos = {};
	_float3                     m_vPrevEmitPos2 = {};
	_bool						m_bPrevEmitValid = { false };

	_bool						m_bSpawnFromSource = { false };
	ID3D11ShaderResourceView*	m_pSrcPoolSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pSrcAliveSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pSrcCountSRV = { nullptr };

	CStructuredBuffer*			m_pMeshEmitBuf = { nullptr };
	_uint						m_iMeshEmitTriCount = { 0 };
	_bool						m_bMeshEmit = { false };
	string						m_strEmitMeshPath = { "<none>" };
	_uint						m_iEmitMeshIndex = { 0 };
	_float						m_fEmitMeshScale = { 1.f };

	// 표면 고정 이펙트를 위한 변수
	CComputeShader*				m_pMeshSkinCS = { nullptr };
	ID3D11Buffer*				m_pMeshSkinCB = { nullptr };
	CStructuredBuffer*			m_pSkinInBuf = { nullptr };
	CStructuredBuffer*			m_pSkinOutBuf = { nullptr };
	CStructuredBuffer*			m_pPinIndexBuf = { nullptr };
	CStructuredBuffer*			m_pPinCDFBuf = { nullptr };
	_uint						m_iSkinVertexCount = { 0 };
	_uint						m_iPinIndexCount = { 0 };
	_uint						m_iPinTriCount = { 0 };
	_bool						m_bMeshSkinned = { false };
	string						m_strPinMeshPath = { "<none>" };
	_int						m_iPinMeshIndex = { 0 };
	vector<string>				m_PinGroupPaths;
	_uint						m_iPinGroupCount = { 0 };

	static const _uint			MAX_PIN_GROUPS = 16;
	ID3D11ShaderResourceView*	m_pGlobalPaletteSRV = { nullptr };
	_uint						m_PinGroupBase[MAX_PIN_GROUPS] = {};
	_uint						m_iPinSourceGroupCount = { 0 };
	_uint						m_iPaletteCapacity = { 0 };
	_float4x4					m_matPinWorld = {};
	_bool						m_bPinSourceValid = { false };
	_float3                     m_vArcAxis = {};
	_float                      m_fArcAngle = { 0.f };
	_bool                       m_bArcEnable = { false };

	CTexture*					m_pVectorField = { nullptr };
	ID3D11SamplerState*			m_pVFSampler = { nullptr };
	_bool						m_bUseVF = { false };
	_float						m_fVFIntensity = { 1.f };
	_float						m_fVFTightness = { 0.f };
	_float3						m_vVFTileSize = { 100.f, 100.f, 100.f };
	string						m_strVFPath = {};

public:
	static CEffect_Emitter* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override { return nullptr; }
	virtual void			Free() override;
};

NS_END