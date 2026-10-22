#ifndef Engine_Struct_h__
#define Engine_Struct_h__

namespace Engine
{
	//============================================================================
	// DESC
	//============================================================================
	typedef struct tagEngineDesc
	{
		HINSTANCE			hInstance;
		HWND				hWnd;
		WINMODE				eWinMode;
		unsigned int		iViewportWidth, iViewportHeight;
		unsigned int		iNumLevels;
	}ENGINE_DESC;

	typedef struct tagLightDesc
	{
		LIGHT				eType;
		XMFLOAT4			vDiffuse, vAmbient, vSpecular;

		XMFLOAT4			vDirection;
		XMFLOAT4			vPosition;
		float				fRange;

		//Spot Light 전용 원뿔 Cosf
		// fInner > fOuter 유지
		//추가
		float				fInnerCos;
		float				fOuterCos;

		// 그림자 맵 생성할 애들만 true
		bool				bShadow = { true };
		// GOD Ray 쓰는 애들만 true ( 스포트라이트만임 )
		bool				bGodRay = { false };
		// TODO: 추후 적용 
		float				fGodRayIntensity = { 1.f };

	}LIGHT_DESC;

	typedef struct tagSpotData
	{
		XMFLOAT4X4	 LightViewProj[4];
		XMFLOAT4	 AtlasRect[4];
		unsigned int iShadowCount;
		unsigned int iCurShadow;
		unsigned int iTitleSizePixels;
		unsigned int iPad;
	}SpotData;

	typedef struct tagCSMData
	{
		XMFLOAT4X4 CascadeViewProj[4];
		XMFLOAT4   CascadeAtlasRect[4];
		XMFLOAT4   CascadeSplitViewZ;
		int		   iCascadeCount;
		int		   iCurCascade;
		int		   iCSMTileSize;
		int		   iCSMEnabled;
	}CSMData;

	typedef struct tagPointData
	{
		XMFLOAT4X4  FaceViewProj[6 * 2];
		XMFLOAT4    FaceAtlasRect[6 * 2];
		XMFLOAT4    LightPosRange[2];
		int         iPointShadowCount;
		int         iCurFace;
		int         iPointTileSize;
		int         iPointPad;
	} PointData;
	
	typedef struct tagGodRayBlurData
	{
		XMFLOAT2 vBlurDir;
		float	 fDepthFallOff;
		float	 fPad;
	}GodRayBlurData;

	typedef struct tagFogVolume
	{
		XMFLOAT3  vCenter;
		float	  fRadius;

		XMFLOAT3  vColor;
		float	  fDensity;

		XMFLOAT3  vExtents;
		float	  fHeightFalloff;

		XMFLOAT3  vAxisX;
		int		  iShapeType;

		XMFLOAT3  vAxisY;
		float	  fHeightFalloffDirection = { 1.f };

		XMFLOAT3  vAxisZ;
		float	  fPad1;
	} FogVolume;



	//typedef struct tagKeyFrame
	//{
	//	XMFLOAT3		vScale;
	//	XMFLOAT4		vRotation;
	//	XMFLOAT3		vTranslation;
	//	float			fTrackPosition;
	//}KEYFRAME;

	//TODO : 프레임워크 수정사항. 애니메이션 키프레임 구조체 분리
	typedef struct tagKeyFrame
	{
		float				fTrackPosition;
	} KEYFRAME;

	typedef struct tagKeyFrameVector : public KEYFRAME
	{
		XMFLOAT3			vVector3;
	}KEYFRAME_VEC;

	typedef struct tagKeyFrameQuaternion : public KEYFRAME
	{
		XMFLOAT4			vQuat;
	}KEYFRAME_QUAT;

	typedef struct tagTransformData
	{
		XMFLOAT4			vScale = { 1.f, 1.f, 1.f,1.f };
		XMFLOAT4			vRot = { 0.f,0.f,0.f,1.f };
		XMFLOAT4			vTranslation = { 0.f,0.f,0.f,1.f };
	} TransformData;

	using POSE = vector<TransformData>;

	typedef struct tagKeyFrameIndices
	{
		unsigned int		iScaleIndex;
		unsigned int		iRotIndex;
		unsigned int		iTransIndex;

	} KeyFrameIndices;

	//typedef struct tagTriggerData { class CGameObject* pHitActor; } TriggerData;

	/* Physic */
	typedef struct tagContactData
	{
		XMFLOAT3			vHitPosition;
		XMFLOAT3			vHitNormal;
		XMFLOAT3			vHitDirection;
		float				fImpulse;
	} ContactData;

	typedef struct tagRaycastData
	{
		XMFLOAT3			vHitPosition;
		float				fDistance;
	} RaycastData;

	typedef struct tagSweepData
	{
		XMFLOAT3			vHitPosition; // 월드 좌표
		XMFLOAT3			vHitNormal;
		XMFLOAT3 			vHitDirection;
		float				fDistance;	
		PxShape*			pHitShape; // 충돌한 부위 정보
	} SweepData;


	//충돌 결과 데이터 
	typedef struct tagCollisionResult
	{
		COLLISION_TYPE		eCollisionType = { COLLISION_TYPE::END };
		PxPairFlag::Enum	eCollisionFlag = { PxPairFlag::eNOTIFY_TOUCH_FOUND };
		class CGameObject* pOtherActor = { nullptr };

		union
		{
			ContactData		tContactData;
			RaycastData		tRaycastData;
			SweepData		tSweepData;
		};

	} CollisionResult;

	//CollisionResult에서 피격 결과 데이터로 변환할 때 사용되는 구조체
	typedef struct tagHitInfo
	{
		class CGameObject* pOtherActor  = { nullptr };
		class CGameObject* pDamangeCauser = { nullptr };

		float	 fDamage				= { 0.f };
		HIT_TYPE eHitType				= { HIT_TYPE::End };
		XMFLOAT3 vHitPosition			= { XMFLOAT3(0.f, 0.f, 0.f) };
		XMFLOAT3 vHitNormal				= { XMFLOAT3(0.f, 1.f, 0.f) };
		bool	 bParried				= { false };
		float    fGroggy				= { 0.f };
		//속성 공격 데이터 
		SPECIAL_ATTACK_TYPE eSpecialAttackType = { SPECIAL_ATTACK_TYPE::END };
		float	 fSpecialAttackValue	= { 0.f };

	} HitInfo;

	typedef struct tagCameraPose
	{
		XMFLOAT4	vPosition = { 0.f,0.f,0.f,1.f };
		XMFLOAT4	vQuat = { 0.f,0.f,0.f,1.f };
	} CameraPose;

	typedef struct tagCCTResult
	{
		XMFLOAT4	vSimulatedPosition = { 0.f ,0.f,0.f,0.f };
		bool		m_bGrounded = { false };
		bool		m_bSideHit = { false };
		bool		m_bCeilingHit = { false };
	} CCTResult;

	typedef struct tagShapeDesc
	{
		GEOMETRY_TYPE		eType = GEOMETRY_TYPE::END;
		XMFLOAT3			vHalfExtents = { 0.5f,0.5f,0.5f };	//BOX	
		float				fRadius = { 0.5f };				//SPHERE, CAPSULE
		float				fHalfHeight = { 0.5f };				//CAPSULE

		XMFLOAT3			vLocalOffset = { 0.f,0.f,0.f };
		XMFLOAT4			vLocalRot = { 0.f,0.f,0.f,1.f };

		bool				bTrigger = { true };				// Trigger or Collider
		//피격된 부위 판정이 필요하면 Shape Filter타입 추가로 저장 
	} SHAPE_DESC;

	typedef struct tagPhysicDesc
	{
		class CGameObject* pOwner = { nullptr };
		XMFLOAT3			vInitPos = { 0.f,0.f,0.f };
		XMFLOAT4			vInitQuat = { 0.f,0.f,0.f,1.f };

		//Actor에  붙일 Shape 정보
		vector<SHAPE_DESC>	Shapes;

		float				fMass = { 1.f };				//질량
		float				fFriction = { 0.5f };			//마찰력
		bool				bIsKinematic = { false };
		bool				bEnableCCD = { false };			// CCD 활성화 여부(고속이동 터널링 방지 플래그)

		unsigned int		iFilterWord0 = { 0 };			// 자신이 속한 그룹
		unsigned int		iFilterWord1 = { 0 };			// 자신이 속한 그룹
	} PHYSIC_DESC;

	/* Status관련 구조체 */
	typedef struct tagStatusProfile
	{
		bool bInputLocked = { false };
		bool bMovementLocked = { false };
		bool bRotationLocked = { false };
		bool bJumpLocked = { false };
		bool bCameraLocked = { false };
		bool bSuperArmor = { false };
		bool bInvincible = { false };
	} StatusProfile;

	/* Shader Reflection 관련 */
	using PropertyBinder = function<void(BYTE* pBuffer)>;

	typedef struct tagMaterialProperty
	{
		string				strName;
		// 오프셋만큼 더한 버퍼 위치에 속성을 저장할 함수
		PropertyBinder		Binder;
		// 이 프로퍼티가 메테리얼 버퍼 안에서 가질 바이트 오프셋
		unsigned int		iByteOffset;
	}MaterialProperty;

	typedef struct tagShaderValueReflectionData
	{
		unsigned int iOffset;
		SHADER_VALUE_TYPE eValueType;
	} Shader_ReflectionData;

	/* Event 관련 Desc */ 
	typedef struct tagEventDesc
	{
	} Event_Desc;

	typedef struct tagActorSpawnEventDesc : public Event_Desc
	{
		class CGameObject* pActor = { nullptr };
		ACTOR_TYPE eActorType = { ACTOR_TYPE::END };
	} ActorSpawn_EventDesc;

	typedef struct tagActorDespawnEventDesc : public Event_Desc
	{
		class CGameObject* pActor = { nullptr };
		ACTOR_TYPE eActorType = { ACTOR_TYPE::END };
	} ActorDespawn_EventDesc;

	//============================================================================
	// Shader Need Struct
	//============================================================================


	typedef struct tagMaterialMeta
	{
		SHADING_MODEL	eShadingModel	=  SHADING_MODEL::DEFAULT_LIT ;
		BLEND_MODE		eBlendMode		=  BLEND_MODE::BLEND_OPAQUE ;
		PACKING_MODE	ePackingMode	=  PACKING_MODE::ARM ;
		bool			bTwoSide		=  false ;
		float			fOpacityClip	=  0.333f ;
		bool			bIsNull			=  false ;
	}Material_Meta;

	typedef struct tagShaderKey
	{
		SHADING_MODEL eShadingModel = SHADING_MODEL::DEFAULT_LIT;
		BLEND_MODE	  eBlendMode = BLEND_MODE::BLEND_OPAQUE;
		bool		  bSkinned = false;
		bool		  bInstanced = false;
		bool operator==(const tagShaderKey& tShaderKey) const
		{
			return (eShadingModel == tShaderKey.eShadingModel)
				&& (eBlendMode == tShaderKey.eBlendMode)
				&& (bSkinned == tShaderKey.bSkinned)
				&& (bInstanced == tShaderKey.bInstanced);
		}

	}Shader_Key;

	typedef struct tagShaderHashKey
	{
		size_t operator()(const tagShaderKey& tShaderKey) const
		{
			return (static_cast<size_t>(tShaderKey.eShadingModel) * 31
				+ static_cast<size_t>(tShaderKey.eBlendMode) * 4
				+ (tShaderKey.bSkinned ? 2u : 0u)
				+ (tShaderKey.bInstanced ? 1u : 0u));
		}
	}Shader_HashKey;
	//============================================================================
	// Vertex 
	//============================================================================

	typedef struct tagVertexPos
	{
		XMFLOAT3			vPosition;
	}VTXPOS;

	typedef struct tagVertexPosColor
	{
		XMFLOAT3			vPosition;
		XMFLOAT4			vColor;

	}VTXCOLOR;

	typedef struct tagVertexColorTex
	{
		XMFLOAT3			vPosition;
		XMFLOAT4			vColor;
		XMFLOAT2			vTexcoord;

	}VTXCOLORTEX;

	typedef struct tagVertexPositionTexcoord
	{
		XMFLOAT3			vPosition;
		XMFLOAT2			vTexcoord;

	}VTXTEX;

	typedef struct tagVertexCube
	{
		XMFLOAT3			vPosition;
		XMFLOAT3			vTexcoord;

	}VTXCUBE;

	typedef struct tagVertexPositionNormalTexcoord
	{
		XMFLOAT3			vPosition;
		XMFLOAT3			vNormal;
		XMFLOAT2			vTexcoord;

	}VTXNORTEX;

	typedef struct tagVertexMesh
	{
		XMFLOAT3			vPosition;
		XMFLOAT3			vNormal;
		XMFLOAT2			vTexcoord;

		XMFLOAT3			vTangent;
		XMFLOAT3			vBinormal;

	}VTXMESH;


	typedef struct tagVertexAnimationMesh
	{
		XMFLOAT3			vPosition;
		XMFLOAT3			vNormal;
		XMFLOAT2			vTexcoord;

		XMFLOAT3			vTangent;
		XMFLOAT3			vBinormal;

		XMUINT4				vBlendIndex;
		XMFLOAT4			vBlendWeight;

	}VTXANIMMESH;

	typedef struct tagVertexParticleInstance
	{
		XMFLOAT4			vRight;
		XMFLOAT4			vUp;
		XMFLOAT4			vLook;
		XMFLOAT4			vTranslation;	
		XMFLOAT2			vLifeTime;
	}VTXPARTICLE_INSTANCE;

	//인스턴싱은 Shader에 Element Desc 전달해야함 
	typedef struct tagVertexPointInstanceDesc
	{
		static const unsigned int		iNumElements = { 6 };

		static constexpr D3D11_INPUT_ELEMENT_DESC   Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},

			{"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},

			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		};
	}VTXPOINT_INSTANCE_DESC;

	//TODO : 0607 맵 인스턴싱 추가 ksb

	typedef struct tagVertextMapInstance
	{
		XMFLOAT4		vRight;
		XMFLOAT4		vUp;
		XMFLOAT4		vLook;
		XMFLOAT4		vTranslation;
	}VTXMAP_INSTANCE;


	typedef struct tagVertexMapInstanceDesc
	{
		static const unsigned int		iNumElements = { 9 };

		static constexpr D3D11_INPUT_ELEMENT_DESC   Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0},
			{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 32, D3D11_INPUT_PER_VERTEX_DATA,   0},
			{"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 44, D3D11_INPUT_PER_VERTEX_DATA,   0},

			{"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		};
	}VTXMAP_INSTANCE_DESC;


	typedef struct tagVertexMapIndexedInstanceDesc
	{
		static const unsigned int iNumElements = { 5 };

		static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
	}VTXMAP_INDEXED_INSTANCE_DESC;


	typedef struct tagMapInstanceWorld
	{
		XMFLOAT4 vRight;
		XMFLOAT4 vUp;
		XMFLOAT4 vLook;
		XMFLOAT4 vTranslation;
	} MAP_INSTANCE_WORLD;

	typedef struct tagMapInstanceBounds
	{
		XMFLOAT3 vCenter;
		float  fPadding0;
		XMFLOAT3 vExtents;
		float  fPadding1;
	} MAP_INSTANCE_BOUNDS;

	typedef struct tagMapCullCSDesc
	{
		XMFLOAT4 vFrustumPlanes[6];
		XMFLOAT4 vCameraPosition;
		float fCullDistance;
		int iCandidateCount;
		float fDepthBias;
		float fScreenMargin;
		XMFLOAT4X4 PreviousViewProjection;
		XMFLOAT2 vHiZTextureSize;
		int iHiZMipCount;
		int iUseHiZ;
	} MAP_CULL_CS_DESC;

	typedef struct tagHiZBuildDesc
	{
		int iSourceWidth;
		int iSourceHeight;
		int iDestinationWidth;
		int iDestinationHeight;
	} HIZ_BUILD_DESC;

	typedef struct tagHiZHistoryDesc
	{
		XMFLOAT4X4 PreviousViewProjection;
		XMFLOAT2 vTextureSize;
		int iMipCount;
		int iValid;
	} HIZ_HISTORY_DESC;


	typedef struct tagMapInstanceRenderMeta
	{
		int iGroupID;
		int iVisibleBase;
		int iPadding0;
		int iPadding1;
	} MAP_INSTANCE_RENDER_META;

	typedef struct tagMapInstanceRenderDesc
	{
		int iVisibleBase;
		XMFLOAT3 vPadding;
	} MAP_INSTANCE_RENDER_DESC;

	//============================================================================
	// Fixed Shader Buffer Struct
	//============================================================================

	//셰이더에 전달할 메테리얼 속성값 
	typedef struct tagMaterialInstanceData 
	{
		//float g_Tile_X = 1.0f;
		//float g_Tile_Y = 1.0f;
		//float g_UV2Use = 0.0f;
		//float g_1BitMask = 0.0f;

		//float g_UseNormalMap = 1.0f;
		//float g_UseDetailNormalMap = 0.0f;
		//float g_DetailNormalTile_X = 1.0f;
		//float g_DetailNormalTile_Y = 1.0f;

	} MaterialInstData;

	//Material Data . 구조체버퍼는 16정렬 안해도됨. 단, 셰이더 Material.hlsli의 구조체와 완전 일치시키기 
	typedef struct tagMaterialData
	{
		XMFLOAT4	EmissiveColor		= { 0.f,0.f,0.f,1.f };
		XMFLOAT4	BaseColor_Tint		= { 0.f,0.f,0.f,1.f };
		XMFLOAT4	GlassColor			= { 1.f,1.f,1.f,1.f };
		XMFLOAT4	DirtyColor			= { 0.f,0.f,0.f,1.f };
		XMFLOAT4	EmissiveEyeColor	= { 0.f,0.f,0.f,1.f };

		float		Roughness			= { 1.f };
		float		Roughness_Factor	= { 1.f };
		float		Emissive			= { 1.f };
		float		Specular			= { 0.5f };

		float		AO_Pow				= { 1.f };
		float		Metallic_Factor		= { 1.f };
		float		RefractionDepthBias	= { 1.f };
		float		EmissiveMode		= { 0.f };
	} MaterialData;

	//오브젝트 버퍼 데이터 : 오브젝트별 데이터 
	typedef struct tagPerObjectBufferData
	{
		XMFLOAT4X4			WorldMatrix;
		int					iMaterialIndex;
		int					iAlphaClip = { 0 };
		float				fEmissiveAmount = { 1.f };
		int					iRimProfile = { 0 };
		float				fRimAmount = { 0.f };
		float				fDissolveAmount = { 0.f };
		unsigned int		iBoneOffset = { 0 };
		unsigned int		iBoneCount = { 0 };
		XMFLOAT4			vDissolveColor = {};
	} PerObjectData;

	typedef struct tagPerLightBufferData
	{
		XMFLOAT4			vDiffuse, vAmbient, vSpecular;
		XMFLOAT4			vDirection;
		XMFLOAT4		    vLight_Pos;
		float			    fLight_Range;	
		//추가
		float				fLight_InnerCos;
		float				fLight_OuterCos;
		unsigned int		iShadowIndex;

	} PerLightData;

	typedef struct tagPerCameraBufferData
	{
		XMFLOAT4X4			ViewMatrix;
		XMFLOAT4X4			ProjMatrix;
		XMFLOAT4X4			ViewInvMatrix;
		XMFLOAT4X4			ProjInvMatrix;
		XMFLOAT4			vCamPos;
	} PerCameraData;

	struct VTXPARTICLE_GPU
	{
		XMFLOAT3		vPos;
		float			fSize;
		XMFLOAT3		vVelocity;
		float			fAge;
		XMFLOAT4		vColor;
		float			fLifeTime;
		float			fRotation;
		XMFLOAT2		fPadding;
		XMFLOAT4		vSpawnT;
		XMFLOAT4		vSpawnB;
		XMFLOAT3		vRenderVel;
		float			fBaseSpeed;
		XMFLOAT3		vOrbitCenter;
		float			fOrbitPad;
		XMFLOAT3		vOrbitAxis;
		float			fOrbitAxisPad;
		unsigned int	iAnchorTri;
		float			fBaryU;
		float			fBaryV;
		unsigned int	bAnchored;
	};

	typedef struct tagPrefilterData
	{
		float fRoughness;
		unsigned int iMipSize;
		XMFLOAT2 vPadding;
	}PerPrefilterData;

	typedef struct tagIBLData
	{
		float    fMaxMip;
		float    fDiffuseIntensity;
		float    fSpecularIntensity;
		float    fPadding;
	} PerIBLData;

	typedef struct tagShadowViewGPU
	{
		XMFLOAT4X4	ViewProj;
		XMFLOAT4	AtlasRect;
	}ShadowViewGPU;

	typedef struct tagShadowGlobalData
	{
		int       iSpotBase, iSpotCount, iSpotTile, iSpotEnabled;
		int       iCSMBase, iCSMCount, iCSMTile, iCSMEnabled;
		int       iPointBase, iPointCount, iPointTile, iPointEnabled;
		int       iCurView, iPad0, iPad1, iPad2;
		XMFLOAT4  CSMSplitViewZ;
		XMFLOAT4  PointLightPosRange[2];
	}ShadowGlobalsData;

	typedef struct tagRenderTuning
	{
		/* 그림자 Bias / Filter*/
		float	 fCSMBiasBase		= { 0.0008f };
		float	 fCSMFilterTexels	= { 1.5f };
		float	 fSpotBias			= { 0.0002f };
		float	 fSpotFilterTexels	= { 2.0f };

		float	 fPointBias			= { 0.0015f };
		float	 fPointFilterTexels	= { 2.0f };
		float	 fShadowStrength	= { 1.f };
		float	 fSSAODiffuseMin	= { 0.5f };

		/* 앰비언트 */
		XMFLOAT3 vAmbientFloor		= { 0.09f, 0.09f, 0.09f };
		float	 fAmbientDesat		= { 0.3f };

		/* SSAO */
		float    fSSAORadius		= { 0.1f };
		float    fSSAOBias			= { 0.02f };
		float    fSSAOPower			= { 1.8f };
		float    fSSAOStrength		= { 1.0f };

		/* SSR */
		float    fSSRMaxDistance	= { 50.f };
		float    fSSRThickness		= { 0.6f };
		float    fSSRRoughCutoff	= { 0.6f };
		float    fSSRBias			= { 0.05f };

		/* GodRay */
		float    fGodRayG			= { 0.55f};
		float    fGodRayDensity		= { 0.04f};
		float    fGodRayIntensity	= { 2.0f};
		float    fGodRayMaxDist		= { 60.f };


		float	 fExposure			= { 1.f };
		
		/* Tone Mapping*/
		float    fGamma				= { 2.2f };


		/* Bloom */	
		float    fBloomThreshold	= { -1.f };
		float    fBloomKnee			= { 0.15f };			// 16 Cut
		float    fBloomIntensity	= { 0.675f };

		/* Emissive */
		float    fEmissiveScale		= { 0.015f };
		float    fLampEmissiveScale	= { 0.2f };

		/* Fog On Off */
		int		 iGlobalFogEnable	= { 1 };


		/* Glass */
		float    fGlassBaseAlpha	= { 0.10f };
		float    fGlassTint			= { 0.25f };
		float    fGlassFresnel		= { 0.50f };
		float    fGlassReflect		= { 0.40f };
		float    fGlassSpec			= { 0.25f };
		float    fGlassDirtAlpha	= { 0.45f };

		XMFLOAT2 vPad				={};

		/* Fog */
		XMFLOAT3 vFogColor			= { 0.62f, 0.66f, 0.72f };
		float	 fFogDensity		= { 0.008f };

		float	 fFogStart			= { 10.f };
		float	 fFogHeight			= { 0.f };
		float	 fFogHeightFalloff	= { 0.12f };
		float	 fFogFeather		= { 0.35f };
	}RenderTuning;

	typedef struct tagPostGlobals
	{
		XMFLOAT4 vSMAA_RT_METRICS = {};

		XMFLOAT3 vFogColor = { 0.62f, 0.66f, 0.72f };
		float    fFogDensity = { 0.008f };

		float    fFogStart = { 10.f };
		float    fFogHeight = { 0.0f };
		float    fFogHeightFalloff = { 0.12f };
		int      iFogVolumeCount = { 0 };

		float    fFeather = { 0.35f };
		float    fExposureScale = { 1.f };
		float    fTime = { 0.f };
		float    fReflectClipHeight = { 0.f };
	}PostGlobals;

	typedef struct tagRimProfile
	{
		XMFLOAT3 vColor;
		float    fFill;
		float	 fIntensity;
		XMFLOAT3 vPadding;
	}RimProfile;


	/* Shake Structure */
	struct FOscillatorDesc
	{
		float	fAmplitude			= { 0.f };
		float	fFrequency			= { 0.f };
		int		iWaveform			= { 0 };			// 0 : sineWave, 1 : PerlineNoise 
		int		iInitialOffset		= { 0 };
	};

	/* Camera Structure */
	struct SHAKE_DESC
	{
		float			fOscillationDuration		= { 0.f };
		float			fOscillationBlendInTime		= { 0.f };
		float			fOscillationBlendOutTime	= { 0.f };
		FOscillatorDesc RotPitch, RotYaw, RotRoll;			
		FOscillatorDesc LocX, LocY, LocZ;					
		FOscillatorDesc FOV;								
		
		string			AnimSequenceKey;
		float			fAnimBlendInTime			= { 0.f };
		float			fAnimBlendOutTime			= { 0.f };
		float			fAnimPlayRate				= { 1.f };
		//같은 Shake 재생중이면 재시작 
		bool			bSingleInstance				= { false };

	};

	/* Scene Structure*/
	struct FCurveKey
	{
		float			fTime						= { 0.f };
		float			fValue						= { 0.f };
		float			fArriveTangent				= { 0.f };
		float			fLeaveTangent				= { 0.f };
		CURVE_INTERP	Interp						= { CURVE_INTERP::CUBIC };
		CURVE_TANGENT	Tangent						= { CURVE_TANGENT::AUTO };
	};

	struct FCurveChannel
	{
		float fDefaultValue = { 0.f };
		vector<FCurveKey> Keys;
	};

	struct SCENE_DESC
	{
		float			fFieldOfView		= { 0.f };
		float			fAspectRatio		= { 0.f };
		int				iTickResolution		= { 24000 };
		int				iDisplayRate		= { 30 };
		float			fDuration			= { 0.f };

		FCurveChannel	LocX, LocY, LocZ;
		FCurveChannel	RotX, RotY, RotZ;
		FCurveChannel	ScaleX, ScaleY, ScaleZ;
	};

	struct CAMWORK_DESC
	{
		//시각축 Envelope(시간변화에따른 애니메이션 변화 데이터값 )

		//Cam Offset : 기준에서 얼마나 이동할지
		FCurveChannel	OffsetX, OffsetY, OffsetZ;
		FCurveChannel	FovOffset; 
		// Weight 가중치 
		FCurveChannel	BoneAlpha; 

		float			fDuration		= { 1.f };
		//카메라 거리 = Arm * DistanceRate
		float			fDistanceRate	= { 1.f };
		//옆으로 스텝
		float			fSideAmount		= { 0.f };
		float			fInterpSpeed	= { 0.f };
		float			fBlendInTime	= { 0.1f };
		float			fBlendOutTime	= { 0.1f };
		//사이드 스탭하는가 
		bool			bUseSide		= { false };
		//거리기준. true : 실제거리 , false : 팔길이 로 시작
		bool			bUseStartDistance = { false };
		//카메라 월드 위치 고정 
		bool			bWorldFixed		= { false };
	};
	
	typedef struct tagPerUIData
	{
		XMFLOAT4	vColor = { 1.f, 1.f, 1.f, 1.f };

		XMFLOAT4	vColorLT = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4	vColorRT = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4	vColorRB = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4	vColorLB = { 1.f, 1.f, 1.f, 1.f };

		XMFLOAT4	vUVRect = { 0.f, 0.f, 1.f, 1.f };

		float		fFillAmount = { 1.f };
		int			iUseTexture = { 1 };
		int			iBlendMode = { 0 };

		float		fRevealProgress = { 1.f };
		float		fRevealSoftness = { 0.02f };
		XMFLOAT2	vRevealCenter = { 0.5f, 0.5f };

		int			iRevealType = { 0 };
	}PerUIData;
}


#endif // Engine_Struct_h__
