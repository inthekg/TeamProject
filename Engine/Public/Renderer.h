#pragma once

#include "ConstantBuffer.h"
#include "RenderStruct.h"
#include "EditableStructuredBuffer.h"

NS_BEGIN(Engine)

class CAllocator;
class CShader;
class CTexture;
class CVIBuffer;
class CIBL_Baker;
class CComputeShader;
class CDecal;

class CRenderer final : public CBase
{
private:
	//Clear용 SRV
	static ID3D11ShaderResourceView* m_pNullSRVs[128];

private:
	CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CRenderer() = default;

public:
	HRESULT						Initialize();
	void						Add_RenderGroup(RENDERID eGroupID, class CGameObject* pGameObject);
	void						Add_Decal(CDecal* pDecal);
	HRESULT						Draw();

	//현재 진행 중인 렌더 패스(여러 패스에 그리는 오브젝트가 분기용으로 사용)
	RENDERID					Get_CurRenderPass() const { return m_eCurRenderPass; }

public:
	ID3D11ShaderResourceView*	Get_HiZSRV() const { return m_pHiZSRV; }
	_bool						Get_HiZHistory(HIZ_HISTORY_DESC* pOutHistory) const;
	HRESULT						Resize_HiZ(_uint iViewportWidth, _uint iViewportHeight);

public:
	HRESULT						Bind_LightBuffer(LIGHT eLightType, _uint iIndex, _int iShadowIndex = -1);
	//현재 파이프라인에 세팅된 카메라 데이터로 업데이트
	HRESULT						Bind_CameraBuffer();
	//매개변수로 전달된 데이터로 카메라 업데이트
	HRESULT						Bind_CameraBuffer(PerCameraData& tCameraData);

	HRESULT						Bind_Material_Textures(CMaterial* pMaterial);

	HRESULT						Bind_ObjectBuffer(RenderCommand* pCommand);
	HRESULT						Bind_MaterialBuffer(CMaterial* pMaterial);
	HRESULT						Update_MaterialBuffer(vector<CMaterial*>* pMaterialVector);

	HRESULT						Bind_UIBuffer(const UICommand* pCommand);


public:
	void						Submit_Static(StaticCommand& command);
	void						Submit_Skinned(SkinnedCommand& command);
    void						Submit_Effect(EffectCommand& command);
	void						Submit_Static_Resolved(StaticCommand& command, _bool bInstanced);
	void						Submit_Skinned_Resolved(SkinnedCommand& command);
	void						Resolve_Command(RenderCommand& tCommand, _bool bSkinned, _bool bInstanced);

	HRESULT						Command_Execute();

	// UI용 추가
	void						Submit_UI(UICommand& command);
	void						Submit_UIText(UITextCommand& Command);

    //직전 커맨드 처리값이 남아있지 않도록 초기화함
    void                        Clear_LastState();

	//해당 슬롯의 SRV용 레지스터 클리어
	void						Clear_BindingSRV(_uint iTextureSlot, _uint iCount = 1);

    //정점버퍼없이 풀스크린 랜더링할때  사용
    void						Render_FullScreenRect();

	//RTV 용 Shdare만 여기에서 생성
	HRESULT						Create_Shaders();
	// 렌더 타겟 생성
	HRESULT						Create_MRTs();

	// IBL BAKING
	HRESULT						Bake_IBL();

	HRESULT						Command_Execute_Depth(CShader* pStatic, CShader* pSkinned);
	void                        Add_FogVolume(const FogVolume& tVolume);
	void						Set_ExposureScale(_float fTarget, _float fBlendTime = 0.f);
	void						Update_ExposureScale(_float fTimeDelta);
	void						Update_SceneTime(_float fTimeDelta);
	void						Set_ReflectionPlane(_float fHeight, _bool bEnable);

	HRESULT						Load_RenderTuning_FromFile();
	HRESULT						Save_RenderTuning_ToFile();


	HRESULT						Ready_DefaultTexture();
	void						Bind_TextureStack(class CMaterial* pMaterial, TEXTURE_TYPE eType, _uint iSlot, ID3D11ShaderResourceView** pCache, ID3D11ShaderResourceView* pDefaultSRV);

#ifdef _DEBUG
	/* Shader Editor 내부 값 수정을 위해 밖으로 빼놓기 작업*/
public:
	// RenderTuning
	const RenderTuning& Get_RenderTuning() const { return m_tRenderTuning; }
	void  Set_RenderTuning(const RenderTuning& t) { m_tRenderTuning = t; }
	void  Update_RenderTuning_DEBUG() { if (m_pRenderTuningBuffer) m_pRenderTuningBuffer->Update_Buffer(&m_tRenderTuning, 1); }

	// IBL
	const PerIBLData& Get_IBLData() const { return m_tIBLData; }
	void  Set_IBLData(const PerIBLData& t) { m_tIBLData = t; }
	void  Update_IBLData_DEBUG() { if (m_pIBLBuffer) m_pIBLBuffer->Update_Buffer(&m_tIBLData, 1); }

	// RimProfile (8 슬롯)
	const RimProfile& Get_RimProfile(int i) const { return m_RimProfiles[i]; }
	void  Set_RimProfile(int i, const RimProfile& t) { m_RimProfiles[i] = t; }
	void  Update_RimProfiles_DEBUG() { if (m_pRimProfileBuffer) m_pRimProfileBuffer->Update_Buffer(m_RimProfiles, 8); }

#endif
private:
	// 뷰포트 세팅 헬퍼
	HRESULT						Render_RenderGroup(RENDERID eGroup);
	void						Set_Viewport(_float fWidth, _float fHeight);
	void						Set_ShadowPassState();
	void						Set_AtlasTile(_int iTileIndex, _uint iTileSize, _uint iAtlasCols);

	HRESULT						Ready_HiZ();		// 0714 _KSB추가
	HRESULT						Build_ShadowViews();
	HRESULT						Build_SpotData();
	HRESULT						Build_CSMData();
	HRESULT						Build_PointData();
	HRESULT						Build_HiZ();		// 0714 _KSB추가
	_int						Find_ShadowSlot_OfSpot(_uint iSpotIndex) const;
	_int						Find_ShadowSlot_OfPoint(_uint iPointIndex) const;
	void						Release_HiZ();
	HRESULT						Update_PostGlobals();


	//TODO : 랜더링 테스트 후 제거하기 . 테스트용
	HRESULT						Render_Test();

	HRESULT						Render_Priority();
	HRESULT						Render_Shadow();
	HRESULT						Render_NonBlend();
	HRESULT						Render_Decals();
    HRESULT                     Render_Lights();
	HRESULT						Render_Cubemap();
	HRESULT                     Render_UIPreviewBackground();	// 캐릭터 프리뷰 UI
    HRESULT                     Render_Combined();
    HRESULT                     Render_NonLight();
    HRESULT                     Render_Blend();
	HRESULT						Render_Blend_WBOIT();
    HRESULT                     Render_Final();
    HRESULT                     Render_Forward();
    HRESULT                     Render_SSAO();
    HRESULT                     Render_SSR();
	HRESULT                     Render_GodRay();
	HRESULT                     Render_GodRay_Blur();
	HRESULT						Render_UI();
	HRESULT						Render_HalfRes_Effects();	// Half Res 묶음
	HRESULT						Render_PostComposite();		// refl + godRay 합성 묶음
	HRESULT						Render_CSM(list<class CGameObject*>& shadowObjs);
	HRESULT						Render_SPOT(list<class CGameObject*>& shadowObjs);
	HRESULT						Render_POINT(list<class CGameObject*>& shadowObjs);
	HRESULT						Render_Bloom();
	HRESULT						Render_Fog();
	HRESULT						Render_Blend2();
	HRESULT						Render_SMAA();
	HRESULT						Render_PlanarReflection();



#ifdef _DEBUG
	HRESULT						Render_Debug();
#endif // _DEBUG

private:
	const _uint					m_iAllocMemory						= { 1024 * 1024 * 2 }; // 2MB

	ID3D11Device*				m_pDevice							= { nullptr };
	ID3D11DeviceContext*		m_pContext							= { nullptr };
    class CGameInstance*        m_pGameInstance						= { nullptr };
private:
    _float4x4					m_WorldMatrix{}, m_ViewMatrix{}, m_ProjMatrix{};

private:
	//TODO 0714 _KSB HIZ 용
	ID3D11Buffer*				 m_pHiZBuildConstantBuffer			= { nullptr };
	ID3D11Texture2D*			 m_pHiZTexture						= { nullptr };

private:
	//Render Queue
	CAllocator*					m_pAllocator						= { nullptr };
    class CVIBuffer_Rect*       m_pVIBuffer_Rect					= { nullptr };

	//Shader

	CShader*					m_pLightDirectShaderCom				= { nullptr };
	CShader*					m_pLightPointShaderCom				= { nullptr };
	CShader*					m_pLightSpotShaderCom				= { nullptr };
	CShader*					m_pIBLShaderCom						= { nullptr };
	CShader*					m_pCombineShaderCom					= { nullptr };
	CShader*					m_pFinalShaderCom					= { nullptr };
	CShader*					m_pSSAOShaderCom					= { nullptr };
	CShader*					m_pSSAOBlurShaderCom				= { nullptr };
	CShader*					m_pSSRShaderCom						= { nullptr };
	CShader*					m_pGodRayShaderCom					= { nullptr };
	CShader*					m_pGodRayBlurShaderCom				= { nullptr };
	CShader*					m_pReflCompositeShaderCom			= { nullptr };
	CShader*					m_pCSMShader_Static					= { nullptr };
	CShader*					m_pCSMShader_Skinned				= { nullptr };
	CShader*					m_pSpotShader_Static				= { nullptr };
	CShader*					m_pSpotShader_SKinned				= { nullptr };
	CShader*					m_pPointShader_Static				= { nullptr };
	CShader*					m_pPointShader_Skinned				= { nullptr };
	CShader*					m_pBloomPrefilterShaderCom			= { nullptr };
	CShader*					m_pBloomDownShaderCom				= { nullptr };
	CShader*					m_pBloomUpShaderCom					= { nullptr };
	CShader*					m_pBloomCompositeShaderCom			= { nullptr };
	CShader*					m_pFogShaderCom						= { nullptr };
	CShader*					m_pRimShaderCom						= { nullptr };
	CShader*					m_pSMAAEdgeShaderCom				= { nullptr };
	CShader*					m_pSMAAWeightShaderCom				= { nullptr };
	CShader*					m_pSMAABlendShaderCom				= { nullptr };
	CShader*					m_pWBOITShaderCom					= { nullptr };

	CComputeShader*				m_pHiZCopyCs						= { nullptr };
	CComputeShader*				m_pHiZDownsampleCS					= { nullptr };

#ifdef _DEBUG
	CShader*					m_pDebugShaderCom					= { nullptr };
#endif // _DEBUG
	//IBL_BAKER
	CIBL_Baker*					m_pIBLBaker							= { nullptr };

private :
	_bool						m_bIBLBaked							= { false };

	/* SpotLight */
	SpotData					m_tSpotData							= {};
	_int						m_SlotOwnerSpot[g_iMaxSpots]		= {};
	_int						m_PrevSlotOwnerSpot[g_iMaxSpots]	= {};
	_bool                       m_bGodRayActiveThisFrame			= { false };

	/* CSMLight */
	CSMData						m_tCSMData							= {};
	_bool						m_bCSMEnabled						= { false };

	/* PointLight */
	PointData					m_tPointData						= {};
	_int						m_SlotOwnerPoint[g_iMaxPoints]		= {};
	_int                        m_PrevSlotOwnerPoint[g_iMaxPoints]	= {};

	/*ShadowData*/
	ShadowGlobalsData			m_tShadowGlobals					= {};
	RenderTuning				m_tRenderTuning						= {};
	PerIBLData					m_tIBLData							= {};
	RimProfile					m_RimProfiles[8]					= {};

	vector<FogVolume>           m_FogVolumes						= {};
private:
	list<class CGameObject*>	m_RenderObjects[ETOUI(RENDERID::END)];
	list<CDecal*>				m_Decals;

	//고정데이터 벼퍼
	CConstantBuffer*			m_pPerObjectBuffer					= { nullptr };
	CConstantBuffer*			m_pPerCameraBuffer					= { nullptr };

	CConstantBuffer*			m_pPerLightBuffer					= { nullptr };
    CConstantBuffer*            m_pEffectBuffer						= { nullptr };
    CConstantBuffer*            m_pPerMaterialBuffer				= { nullptr };
	static constexpr _uint		m_iMaterialCBufferMax				= { 256 };

	CConstantBuffer*			m_pUIBuffer							= { nullptr };

	PostGlobals					m_tPostGlobals						= {};


	_float						m_fExposureScale					= { 1.f };
	_float						m_fExposureScaleTarget				= { 1.f };
	_float						m_fExposureBlendRemain				= { 0.f };
	_float						m_fSceneTime						= { 0.f };
	_float						m_fReflectionHeight					= { 0.f };
	_bool						m_bReflectionEnable					= { false };

	//그 외  버퍼 . 일시적 데이터 담당

	CConstantBuffer*			m_pGodRayBlurBuffer					= { nullptr };
	CConstantBuffer*			m_pShadowGlobalsBuffer				= { nullptr };
	CConstantBuffer*			m_pPostGlobalsBuffer				= { nullptr };

	CEditableStructuredBuffer*	m_pMaterialsBuffer					= { nullptr };
	CEditableStructuredBuffer*	m_pIBLBuffer						= { nullptr };
	CEditableStructuredBuffer*	m_pShadowViewsBuffer				= { nullptr };
	CEditableStructuredBuffer*	m_pRenderTuningBuffer				= { nullptr };
	CEditableStructuredBuffer*	m_pFogVolumeBuffer					= { nullptr };
	CEditableStructuredBuffer*	m_pRimProfileBuffer					= { nullptr };





	//MaterialTexture - SRV
	ID3D11ShaderResourceView*	m_pAlbedoSRVs[g_iMaxTextureStack]	= { nullptr };
	ID3D11ShaderResourceView*	m_pNormalSRVs[g_iMaxTextureStack]	= { nullptr };
	ID3D11ShaderResourceView*	m_pMaskSRVs[g_iMaxTextureStack]		= { nullptr };
	ID3D11ShaderResourceView*	m_pMask2SRV							= { nullptr };
	ID3D11ShaderResourceView*	m_pDetaillNormalSRV					= { nullptr };
	ID3D11ShaderResourceView*	m_pEmissiveSRV						= { nullptr };
	ID3D11ShaderResourceView*	m_pDefaultAlbedoSRV					= { nullptr };
    //Default Texture
	ID3D11ShaderResourceView*	m_pDefaultMaskSRV					= { nullptr };
	ID3D11ShaderResourceView*	m_pAreaTexSRV						= { nullptr };
	ID3D11ShaderResourceView*	m_pSearchTexSRV						= { nullptr };

	ID3D11ShaderResourceView*	m_pHiZSRV							= { nullptr };

	//Defulat Sampler
    //Default는 Wrap
	ID3D11SamplerState*			m_pLinearSampler					= { nullptr };
    ID3D11SamplerState*         m_pPointSampler						= { nullptr };
    ID3D11SamplerState*         m_pLinearClampSampler				= { nullptr };

	//Default ResterizerState
	ID3D11RasterizerState*		m_pDefaultRS						= { nullptr };
    ID3D11RasterizerState*      m_pNoneCullRS						= { nullptr };
	ID3D11RasterizerState*		m_pShadowScissorRS					= { nullptr };

	//Depth Stencil State
	ID3D11DepthStencilState*	m_pDepth_DisableState				= { nullptr };
	ID3D11DepthStencilState*	m_pDepth_EnableState				= { nullptr };
    ID3D11DepthStencilState*    m_pDepth_OnlyReadState				= { nullptr };
    ID3D11DepthStencilState*    m_pDepth_Fwd_StencilWrite			= { nullptr };
    ID3D11DepthStencilState*    m_pDepth_Refl_StencilTest			= { nullptr };
	ID3D11DepthStencilState*	m_pDepth_Sky						= { nullptr };
	ID3D11DepthStencilState*	m_pDepth_GBuffer_ReceiverWrite		= { nullptr };
	ID3D11DepthStencilState*	m_pDepth_Decal_ReceiverTest			= {	nullptr };

	//Depth Stencil View
	ID3D11DepthStencilView*		m_pSpotDSV							= { nullptr };
	ID3D11DepthStencilView*		m_pCSMDSV							= { nullptr };
	ID3D11DepthStencilView*		m_pPointDSV							= { nullptr };
	ID3D11DepthStencilView*		m_pReflectionDSV					= { nullptr };


	//Blend State
	ID3D11BlendState*			m_pAlphaBlendState					= { nullptr };
	ID3D11BlendState*			m_pEffectAlphaBlendState			= { nullptr };
	// 가산 블렌딩을 위해 이펙트는 쉐이더 안에서 알파 연산을 수행하고 있으므로 자체적인 알파블렌드 스테이트를 가짐.
    ID3D11BlendState*           m_pAdditiveBlendState				= { nullptr };
	ID3D11BlendState*			m_pMultiplyBlendState				= { nullptr };
	ID3D11BlendState*			m_pWBOITBlendState					= { nullptr };
	ID3D11BlendState*			m_pBloomUpsampleBlendState			= { nullptr };
	vector<RenderCommand*>		m_pCommands;
	RenderCommand*				m_pLastCommand						= { nullptr };
	bool						m_bNeedDepthSort					= {};

	//UI용 행렬
	_float4x4					m_OrthoProjMatrix					= {};
	_float4x4					m_OriginProjMatrix					= {};

	// 모든 UI Text가 공유하는 DirectXTK SpriteBatch
	SpriteBatch*				m_pUISpriteBatch					= { nullptr };

	//TODO 0714 _KSB HIZ 용
	vector<ID3D11ShaderResourceView*> m_HiZMipSRVs;
	vector<ID3D11UnorderedAccessView*> m_HiZMipUAVs;
	HIZ_HISTORY_DESC			m_HiZHistory{};
	_float4						m_vHiZPreviousCameraPosition = {};
	_float4						m_vHiZPreviousCameraLook = {};
	_float2						m_vHiZPreviousProjectionScale = {};
	_uint						m_iHiZCameraDiscontinuityRevision = { UINT_MAX };
	_uint						m_iHiZWidth = {0};
	_uint						m_iHiZHeight = {0};
	_uint						m_iHiZMipCount = {0};

    RENDERID                    m_eCurRenderPass					= { RENDERID::END };


public:
	static CRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END
