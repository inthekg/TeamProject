#include "Effect_Emitter.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "StructuredBuffer.h"
#include "ComputeShader.h"
#include "VIBuffer_GPUParticle.h"
#include "VIBuffer_GPUMesh.h"
#include "Texture.h"
#include "ModelDataStruct.h"

namespace 
{
	struct SimCB 
	{	_float		fDt;			_float3	fGrav; 
		_uint		iMaxCount;		_uint	bColorOL;		_uint	bSizeOL;		_uint	iLutSize;
		_float		fDrag;			_float	fTime;			_uint	iNoise;			_uint	iNoiseType;
		_float		fNoiseStrengh;	_float	fNoiseFreq;		_float	fNoiseDamping;	_float	fNoiseScroll;
		_uint		bNoiseTex;		_uint	bRotOverLife;	_float2 fPadding;
		_float3		vLinearVel;		_float	fOrbit;
		_float3		vOrbitCenter;	_uint	bVelocity;
		_float3		vOrbitAxis;		_uint	bSpiral;
		_float		fGravStart;		_float	fGravEnd;		_float	fVelSpeedMin;	_float	fVelSpeedMax;
		_float3		vEmitOrigin;	_float	fSpeedByAngle;
		_float		fNoiseOffset;	_uint	bSizeMultiply;	_uint	bSpeedOL;		_float	fSimPad2;
		_uint       bUseVF;		    _float  vfIntensity;    _float  vfTightness;    _float  vfPad0;
		_float3     vfTileSize;     _float  vfPad1;
		_uint		bConverge;		_float	fArrivalRadius;	_uint	bInfinite;		_uint	iColorMode;
		_uint		iSizeMode;		_uint	iRotMode;		_uint	iSpeedMode;		_uint	Simpad;
	}; 
}

namespace { struct CamCB { _float4x4 matVP; _float4 vRight; _float4 vUp; _float4 vCamPos; }; }

namespace {
	struct RenderCB
	{
		_uint		bFlipbook;			_uint	iCols;			_uint	iRows;				_float fFrameRate;
		_uint		bSeedBased;			_uint	bSoft;			_float	fSoftFade;			_float fCamFar;
		_uint		iAlignment;			_float	fStretchScale;	_uint	iFlipPlayMode;		_uint  iFlipFixedFrame;
		_uint		iSpinMode;			_float3 vSpinAxis;
		_float3		vMeshScale;			_float	fBasePower;
		_uint		bLocalSpace;		_float	fEmiIntensity;	_float	fSphereHardness;	_float fDiffuseSharpness;
		_float4x4	matEmitterWorld;

		_uint		bUVDistort;			_float	fUVDistortStr;	_float	fTime;				_float fBaseDensity;
		_float4		vUVPan;
		_float4		vNoiseTile;

		_uint		bMask;				_float	fMaskPower;		_uint	bMaskClamp;			_float fLinearOpacity;
		_float4		vMaskTileOffset;

		_uint       bEmissive;			_float  fEmisStr;       _float  fEmisStrA;          _float fEmisPower;
		_float4		vEmisTileA;
		_float4		vEmisTileB;
		_float4		vEmisTint;
		_float4		vEmisExtra;
		_uint		bDissolve;			_float	fDissolveZoom;	_float	fDissolvePower;		_float fDissolveEdge;
		_float4		vDissolveTiling;
		_float4		vDissolveEdgeColor;
		_uint       bFresnel;			_float  fFresPower;     _float  fFresOutInt;        _float fFresInInt;
		_float4		vFresColor;
		_uint		bScale3D;			_float3 vEmitOrigin;
		_uint		bDistort;			_float	fDistortStr;	_uint	iDistortMode;		_uint  bDistortDepth;
		_float2		vScreenSize;		_float2	vDistortPan;
		_float2		vDistortTile;		_uint	bDistortTex;	_float	distortPad;
		_float2		vPivot;				_float2	vPivotPad;
	};
}

namespace {
	struct EmitCB
	{
		_uint		iSpawnCount;	_float	fTime;				_float fLifeMin;		_float fLifeMax;
		_float		fSpeedMin;		_float	fSpeedMax;			_float fSizeMin;		_float fSizeMax;
		_float4		vColor;
		_float3		vEmitPos;		_float	fShapeRadius;
		_float3		vBiasDir;		_float	fBiasWeight;
		_uint		iShapeType;		_uint	bEdgeEmit;			_float fStartRotMin;	_float fStartRotMax;
		_float4x4	matEmitRot;
		_float3		vPrevEmitPos;	_float	fMeshScale;
		_float3		vOrbitCenter;	_uint	iMeshPinned;
		_float3		vOrbitAxis;		_uint	bConverge;
		_uint		iBoneCount;		_uint	bBoneInheritRot;	_uint	iFrameSeed;		_uint  iMeshTriCount;
		_uint		iMeshRelease;	_uint	bSpawnFromSource;	_float	fDt;			_uint  bArcEnable;
		_float3		vArcAxis;		_float	fArcAngle;
		_float3     vPrevEmitPos2;  _uint   bSplineEnable;
		_uint		bBoneAttach;	_float3	fEmitPad0;
	};

	struct MeshEmitTri
	{
		_float3		p0;				_float	fCdf;
		_float3		p1;				_float	pad1;
		_float3		p2;				_float	pad2;
		_float3		nrm;			_float	pad3;
	};

	struct SKIN_IN_VTX
	{
		_float3		pos;			_uint	meshBoneBase;
		_float3		nrm;			_uint	groupIndex;
		XMUINT4		bidx;
		_float4		bw;
	};

	struct SKIN_OUT_VTX
	{
		_float3		pos;			_float	pad0;
		_float3		nrm;			_float	pad1;
	};

	static const _uint MAX_SKIN_GROUPS = 16;

	struct MeshSkinCB
	{
		_uint		iVertexCount;	_uint	iBoneOffset;	_uint	iPaletteCapacity;	_uint	iSkinPad;
		_float4x4	matWorld;
		_uint		iGroupBase[MAX_SKIN_GROUPS];
	};
}

namespace
{
	void MapCB(ID3D11DeviceContext* pContext, ID3D11Buffer* pCB, const void* pSrc, _uint iSize)
	{
		D3D11_MAPPED_SUBRESOURCE m{};
		if (SUCCEEDED(pContext->Map(pCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
		{
			memcpy(m.pData, pSrc, iSize);
			pContext->Unmap(pCB, 0);
		}
	}

	template<typename T>
	_bool SameKeys(const vector<T>& a, const vector<T>& b)
	{
		if (a.size() != b.size())
			return false;
		return a.empty() || memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0;
	}

	HRESULT Load_StaticMeshGeometry(const wstring& strFilePath, _uint iMeshIndex,
		vector<VTXMESH>& OutVerts, vector<_uint>& OutIndices)
	{
		ifstream fin(strFilePath, ios::binary);
		if (!fin.is_open())
			return E_FAIL;

		uint32_t iBoneCount = 0, iMeshCount = 0;
		fin.read(reinterpret_cast<char*>(&iBoneCount), sizeof(uint32_t));
		fin.read(reinterpret_cast<char*>(&iMeshCount), sizeof(uint32_t));

		if (iMeshCount == 0)
			return E_FAIL;
		if (iMeshIndex >= iMeshCount)
			iMeshIndex = 0;

		fin.seekg(static_cast<streamoff>(iBoneCount) * sizeof(BoneData), ios::cur);

		for (uint32_t i = 0; i < iMeshCount; ++i)
		{
			MeshData header{};
			fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));
			if (!fin)
				return E_FAIL;

			if (i == iMeshIndex)
			{
				if (header.HasAnimation)
					return E_FAIL;

				OutVerts.resize(header.vertexCount);
				fin.read(reinterpret_cast<char*>(OutVerts.data()),
					static_cast<streamsize>(sizeof(VTXMESH)) * header.vertexCount);

				OutIndices.resize(header.indexCount);
				fin.read(reinterpret_cast<char*>(OutIndices.data()),
					static_cast<streamsize>(sizeof(_uint)) * header.indexCount);

				return (!fin || OutVerts.empty() || OutIndices.empty()) ? E_FAIL : S_OK;
			}

			fin.seekg(static_cast<streamoff>(header.vertexStride) * header.vertexCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.indexCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.boneCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_float4x4)) * header.boneCount, ios::cur);
		}
		return E_FAIL;
	}

	HRESULT Load_SkinnedMeshGeometry(const wstring& strFilePath, _uint iMeshIndex,
		vector<VTXANIMMESH>& OutVerts, vector<_uint>& OutIndices)
	{
		ifstream fin(strFilePath, ios::binary);
		if (!fin.is_open())
			return E_FAIL;

		uint32_t iBoneCount = 0, iMeshCount = 0;
		fin.read(reinterpret_cast<char*>(&iBoneCount), sizeof(uint32_t));
		fin.read(reinterpret_cast<char*>(&iMeshCount), sizeof(uint32_t));
	
		if (iMeshCount == 0)
			return E_FAIL;
		if (iMeshIndex >= iMeshCount)
			iMeshIndex = 0;

		fin.seekg(static_cast<streamoff>(iBoneCount) * sizeof(BoneData), ios::cur);

		for (uint32_t i = 0; i < iMeshCount; ++i)
		{
			MeshData header{};
			fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));
			if (!fin)
				return E_FAIL;

			if (i == iMeshIndex)
			{
				if (!header.HasAnimation)
					return E_FAIL;

				OutVerts.resize(header.vertexCount);
				fin.read(reinterpret_cast<char*>(OutVerts.data()),
					static_cast<streamsize>(sizeof(VTXANIMMESH)) * header.vertexCount);

				OutIndices.resize(header.indexCount);
				fin.read(reinterpret_cast<char*>(OutIndices.data()),
					static_cast<streamsize>(sizeof(_uint)) * header.indexCount);

				return (!fin || OutVerts.empty() || OutIndices.empty()) ? E_FAIL : S_OK;
			}

			fin.seekg(static_cast<streamoff>(header.vertexStride) * header.vertexCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.indexCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.boneCount, ios::cur);
			fin.seekg(static_cast<streamoff>(sizeof(_float4x4)) * header.boneCount, ios::cur);
		}
		return E_FAIL;
	}

	HRESULT Load_SkinnedMeshMerged(const wstring& strFilePath, _int iMeshIndex, _uint iGroupIndex,
		vector<SKIN_IN_VTX>& OutVerts, vector<_uint>& OutIndices)
	{
		ifstream fin(strFilePath, ios::binary);
		if (!fin.is_open())
			return E_FAIL;

		uint32_t iBoneCount = 0, iMeshCount = 0;
		fin.read(reinterpret_cast<char*>(&iBoneCount), sizeof(uint32_t));
		fin.read(reinterpret_cast<char*>(&iMeshCount), sizeof(uint32_t));
		if (iMeshCount == 0)
			return E_FAIL;
		 
		fin.seekg(static_cast<streamoff>(iBoneCount) * sizeof(BoneData), ios::cur);

		const _bool bAll = (iMeshIndex < 0);
		_uint iBonePrefix = 0;

		for (uint32_t i = 0; i < iMeshCount; ++i)
		{
			MeshData header{};
			fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));
			if (!fin)
				return E_FAIL;

			const streamoff iVtxBytes = header.HasAnimation
				? static_cast<streamoff>(sizeof(VTXANIMMESH))
					: static_cast<streamoff>(sizeof(VTXMESH));

			const _bool bWant = header.HasAnimation && (bAll || static_cast<_int>(i) == iMeshIndex);

			if (bWant)
			{
				vector<VTXANIMMESH> verts(header.vertexCount);
				fin.read(reinterpret_cast<char*>(verts.data()),
					iVtxBytes * header.vertexCount);

				vector<_uint> indices(header.indexCount);
				fin.read(reinterpret_cast<char*>(indices.data()),
					static_cast<streamsize>(sizeof(_uint)) * header.indexCount);

				fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.boneCount, ios::cur);
				fin.seekg(static_cast<streamoff>(sizeof(_float4x4)) * header.boneCount, ios::cur);

				if (!fin)
					return E_FAIL;

				const _uint iVertBase = static_cast<_uint>(OutVerts.size());
				for (const VTXANIMMESH& v : verts)
				{
					SKIN_IN_VTX o;
					o.pos = v.vPosition;	o.meshBoneBase = iBonePrefix;
					o.nrm = v.vNormal;		o.groupIndex = iGroupIndex;
					o.bidx = v.vBlendIndex;
					o.bw = v.vBlendWeight;
					OutVerts.push_back(o);
				}
				for (_uint idx : indices)
					OutIndices.push_back(iVertBase + idx);
			}
			else
			{
				fin.seekg(iVtxBytes * header.vertexCount, ios::cur);
				fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.indexCount, ios::cur);
				fin.seekg(static_cast<streamoff>(sizeof(_uint)) * header.boneCount, ios::cur);
				fin.seekg(static_cast<streamoff>(sizeof(_float4x4)) * header.boneCount, ios::cur);
			}

			iBonePrefix += header.boneCount;
		}
		
		return (OutVerts.empty() || OutIndices.empty()) ? E_FAIL : S_OK;
	}

	HRESULT Load_SkinnedMeshGroups(const vector<wstring>& GroupPaths,
		vector<SKIN_IN_VTX>& OutVerts, vector<_uint>& OutIndices)
	{
		const _uint iGroups = static_cast<_uint>(GroupPaths.size());
		for (_uint g = 0; g < iGroups; ++g)
		{
			Load_SkinnedMeshMerged(GroupPaths[g], -1, g, OutVerts, OutIndices);
		}
		return (OutVerts.empty() || OutIndices.empty()) ? E_FAIL : S_OK;
	}

	bool BuildSkinCache(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
		const vector<SKIN_IN_VTX>& SkinIn, const vector<_uint>& indices,
		CStructuredBuffer** ppIn, CStructuredBuffer** ppOut,
		CStructuredBuffer** ppIdx, CStructuredBuffer** ppCDF,
		_uint& oVtx, _uint& oIdx, _uint& oTri)
	{
		const _uint iVtxCount = static_cast<_uint>(SkinIn.size());
		const _uint iIdxCount = static_cast<_uint>(indices.size());
		const _uint iTriCount = iIdxCount / 3;
		if (iVtxCount == 0 || iTriCount == 0)
			return false;

		vector<_float> CDF(iTriCount);
		_float fTotalArea = 0.f;
		for (_uint i = 0; i < iTriCount; ++i)
		{
			const SKIN_IN_VTX& v0 = SkinIn[indices[i * 3]];
			const SKIN_IN_VTX& v1 = SkinIn[indices[i * 3 + 1]];
			const SKIN_IN_VTX& v2 = SkinIn[indices[i * 3 + 2]];

			_vector vP0 = XMLoadFloat3(&v0.pos);
			_vector vE1 = XMVectorSubtract(XMLoadFloat3(&v1.pos), vP0);
			_vector vE2 = XMVectorSubtract(XMLoadFloat3(&v2.pos), vP0);
			_float fArea = 0.5f * XMVectorGetX(XMVector3Length(XMVector3Cross(vE1, vE2)));

			fTotalArea += fArea;
			CDF[i] = fTotalArea;
		}

		if (fTotalArea > 1e-8f)
		{
			const _float fInv = 1.f / fTotalArea;
			for (_uint i = 0; i < iTriCount; ++i)
				CDF[i] *= fInv;
		}
		else
		{
			for (_uint i = 0; i < iTriCount; ++i)
				CDF[i] = (i + 1) / static_cast<_float>(iTriCount);
		}
		CDF[iTriCount - 1] = 1.f;

		CStructuredBuffer::SB_DESC InDesc{ sizeof(SKIN_IN_VTX), iVtxCount, const_cast<SKIN_IN_VTX*>(SkinIn.data()), false };
		CStructuredBuffer* pIn = CStructuredBuffer::Create(pDevice, pContext, InDesc);

		CStructuredBuffer::SB_DESC outDesc{ sizeof(SKIN_OUT_VTX), iVtxCount, nullptr, false };
		CStructuredBuffer* pOut = CStructuredBuffer::Create(pDevice, pContext, outDesc);

		CStructuredBuffer::SB_DESC idxDesc{ sizeof(_uint), iIdxCount, const_cast<_uint*>(indices.data()), false };
		CStructuredBuffer* pIdx = CStructuredBuffer::Create(pDevice, pContext, idxDesc);

		CStructuredBuffer::SB_DESC cdfDesc{ sizeof(_float), iTriCount, CDF.data(), false };
		CStructuredBuffer* pCDF = CStructuredBuffer::Create(pDevice, pContext, cdfDesc);
	
		if (pIn == nullptr || pOut == nullptr || pIdx == nullptr || pCDF == nullptr)
		{
			Safe_Release(pIn);
			Safe_Release(pOut);
			Safe_Release(pIdx);
			Safe_Release(pCDF);
			return false;
		}

		*ppIn = pIn;    *ppOut = pOut;    *ppIdx = pIdx;    *ppCDF = pCDF;
		oVtx = iVtxCount;    oIdx = iIdxCount;    oTri = iTriCount;
		return true;
	}
}


HRESULT CEffect_Emitter::Build_SharedShaders(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, EMITTER_SHADER_BUNDLE& Out)
{
	const wstring strShaderDir = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/";

	CComputeShader::COMPUTE_SHADER_DESC csDesc{};
	csDesc.strShaderName = L"Particle_Simulate";
	csDesc.strCSFilePath = strShaderDir + L"Compute_ParticleSimulate.hlsl";
	csDesc.strCSFunName = "CS_Main";
	Out.pSimulateCS = CComputeShader::Create(pDevice, pContext, csDesc);

	csDesc.strShaderName = L"Particle_Init";
	csDesc.strCSFilePath = strShaderDir + L"Compute_ParticleInit.hlsl";
	Out.pInitCS = CComputeShader::Create(pDevice, pContext, csDesc);

	csDesc.strShaderName = L"Particle_Emit";
	csDesc.strCSFilePath = strShaderDir + L"Compute_ParticleEmit.hlsl";
	Out.pEmitCS = CComputeShader::Create(pDevice, pContext, csDesc);

	csDesc.strShaderName = L"Particle_Sort";
	csDesc.strCSFilePath = strShaderDir + L"Compute_ParticleSort.hlsl";
	Out.pSortCS = CComputeShader::Create(pDevice, pContext, csDesc);

	csDesc.strShaderName = L"Mesh_VertexSkin";
	csDesc.strCSFilePath = strShaderDir + L"Compute_MeshVertexSkin.hlsl";
	Out.pMeshSkinCS = CComputeShader::Create(pDevice, pContext, csDesc);

	if (Out.pSimulateCS == nullptr || Out.pInitCS == nullptr  
		|| Out.pEmitCS == nullptr || Out.pSortCS == nullptr || Out.pMeshSkinCS == nullptr)
		return E_FAIL;

	_uint iFlag = 0;
#ifdef _DEBUG
	iFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	{
		D3D_SHADER_MACRO macros[] = { { "USE_ALIVE_LIST", "1"}, { nullptr, nullptr } };
		wstring strPath = strShaderDir + L"Shader_Particle.hlsl";
		ID3D10Blob* pVSBlob = nullptr; ID3D10Blob* pPSBlob = nullptr; ID3D10Blob* pErr = nullptr;

		if (FAILED(D3DCompileFromFile(strPath.c_str(), macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_Main", "vs_5_0", iFlag, 0, &pVSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Particle VS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &Out.pVS);

		if (FAILED(D3DCompileFromFile(strPath.c_str(), macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Particle PS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pPSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pPS);
		Safe_Release(pPSBlob);

		if (FAILED(D3DCompileFromFile(strPath.c_str(), macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main_WBOIT", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Particle PS_WBOIT Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pPSBlob);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pPS_WBOIT);
		Safe_Release(pPSBlob);
		Safe_Release(pVSBlob);
	}

	{
		wstring strPath = strShaderDir + L"Shader_MeshParticle.hlsl";
		ID3D10Blob* pVSBlob = nullptr; ID3D10Blob* pPSBlob = nullptr; ID3D10Blob* pErr = nullptr;

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_Main", "vs_5_0", iFlag, 0, &pVSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "MeshParticle VS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &Out.pMeshVS);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		if (FAILED(pDevice->CreateInputLayout(layout, 3, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &Out.pMeshLayout)))
		{
			Safe_Release(pVSBlob);
			return E_FAIL;
		}

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "MeshParticle PS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			Safe_Release(pPSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pMeshPS);
		Safe_Release(pPSBlob);

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main_WBOIT", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "MeshParticle PS_WBOIT Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pPSBlob);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pMeshPS_WBOIT);
		Safe_Release(pPSBlob);
		Safe_Release(pVSBlob);
	}

	{
		wstring strPath = strShaderDir + L"Shader_Trail.hlsl";
		ID3D10Blob* pVSBlob = nullptr; ID3D10Blob* pPSBlob = nullptr; ID3D10Blob* pErr = nullptr;

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_Main", "vs_5_0", iFlag, 0, &pVSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Trail VS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &Out.pTrailVS);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (FAILED(pDevice->CreateInputLayout(layout, 3, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &Out.pTrailLayout)))
		{
			Safe_Release(pVSBlob);
			return E_FAIL;
		}

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Trail PS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			Safe_Release(pPSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pTrailPS);
		Safe_Release(pPSBlob);

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main_WBOIT", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Trail PS_WBOIT Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			Safe_Release(pPSBlob);
			return E_FAIL;
		}

		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pTrailPS_WBOIT);
		Safe_Release(pPSBlob);
		Safe_Release(pVSBlob);
	}

	{
		wstring strPath = strShaderDir + L"Shader_Decal.hlsl";
		ID3D10Blob* pVSBlob = nullptr; ID3D10Blob* pPSBlob = nullptr; ID3D10Blob* pErr = nullptr;

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VS_Main", "vs_5_0", iFlag, 0, &pVSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Decal VS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			return E_FAIL;
		}
		pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &Out.pDecalVS);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (FAILED(pDevice->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &Out.pDecalLayout)))
		{
			Safe_Release(pVSBlob);
			return E_FAIL;
		}

		if (FAILED(D3DCompileFromFile(strPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PS_Main", "ps_5_0", iFlag, 0, &pPSBlob, &pErr)))
		{
			if (pErr)
				MessageBoxA(nullptr, (char*)pErr->GetBufferPointer(), "Decal PS Compile Error", MB_OK);
			Safe_Release(pErr);
			Safe_Release(pVSBlob);
			Safe_Release(pPSBlob);
			return E_FAIL;
		}
		pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &Out.pDecalPS);
		Safe_Release(pVSBlob);
		Safe_Release(pPSBlob);
	}

	return S_OK;
}

void CEffect_Emitter::Free_SharedShaders(EMITTER_SHADER_BUNDLE& InOut)
{
	Safe_Release(InOut.pVS);
	Safe_Release(InOut.pPS);
	Safe_Release(InOut.pPS_WBOIT);
	Safe_Release(InOut.pMeshVS);
	Safe_Release(InOut.pMeshPS);
	Safe_Release(InOut.pMeshPS_WBOIT);
	Safe_Release(InOut.pMeshLayout);
	Safe_Release(InOut.pSimulateCS);
	Safe_Release(InOut.pInitCS);
	Safe_Release(InOut.pEmitCS);
	Safe_Release(InOut.pSortCS);
	Safe_Release(InOut.pMeshSkinCS);
	Safe_Release(InOut.pTrailVS);
	Safe_Release(InOut.pTrailPS);
	Safe_Release(InOut.pTrailPS_WBOIT);
	Safe_Release(InOut.pTrailLayout);
	Safe_Release(InOut.pDecalVS);
	Safe_Release(InOut.pDecalPS);
	Safe_Release(InOut.pDecalLayout);

	InOut = {};
}

CTexture* CEffect_Emitter::LoadTexture(const string& strPath)
{
	if (strPath.empty())
		return nullptr;

	int iLen = MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, &strWPath[0], iLen);

	wstring strName = L"EffectTex";
	return Load_Cached_Texture(strWPath);
}

void CEffect_Emitter::Bind_SharedShaders(const EMITTER_SHADER_BUNDLE* pBundle)
{
	if (pBundle == nullptr)
		return;

	m_pVS			= pBundle->pVS;
	m_pPS			= pBundle->pPS;
	m_pPS_WBOIT		= pBundle->pPS_WBOIT;
	m_pMeshVS		= pBundle->pMeshVS;
	m_pMeshPS		= pBundle->pMeshPS;
	m_pMeshPS_WBOIT = pBundle->pMeshPS_WBOIT;
	m_pMeshLayout	= pBundle->pMeshLayout;
	m_pSimulateCS	= pBundle->pSimulateCS;
	m_pInitCS		= pBundle->pInitCS;
	m_pEmitCS		= pBundle->pEmitCS;
	m_pSortCS		= pBundle->pSortCS;
	m_pMeshSkinCS	= pBundle->pMeshSkinCS;
}

CEffect_Emitter::CEffect_Emitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject { pDevice, pContext }
{
	XMStoreFloat4x4(&m_matExtraRot, XMMatrixIdentity());
}

HRESULT CEffect_Emitter::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Pool_CPU()))
		return E_FAIL;

	if (FAILED(Ready_ComputeAndShaders()))
		return E_FAIL;

	return S_OK;
}

/* Set_Curve: 키프레임을 통해서 생성한 LUT을 Shader로 전달하는 함수 */
void CEffect_Emitter::Set_ColorCurve(const _float4* pColors, _bool bEnabled)
{
	m_bColorOverLife = bEnabled;
	if (bEnabled && pColors && m_pColorLUT
		&& (!m_bColorLUTUpdated || memcmp(m_CachedColorLUT, pColors, sizeof(_float4) * LUT_SIZE) != 0))
	{
		memcpy(m_CachedColorLUT, pColors, sizeof(_float4) * LUT_SIZE);
		m_pContext->UpdateSubresource(m_pColorLUT->Get_Buffer(), 0, nullptr, pColors, 0, 0);
		m_bColorLUTUpdated = true;
	}
}

void CEffect_Emitter::Set_SizeCurve(const _float* pSizes, _bool bEnabled)
{
	m_bSizeOverLife = bEnabled;
	if (bEnabled && pSizes && m_pSizeLUT
		&& (!m_bSizeLUTUpdated || memcmp(m_CachedSizeLUT, pSizes, sizeof(_float) * LUT_SIZE) != 0))
	{
		memcpy(m_CachedSizeLUT, pSizes, sizeof(_float) * LUT_SIZE);
		m_pContext->UpdateSubresource(m_pSizeLUT->Get_Buffer(), 0, nullptr, pSizes, 0, 0);
		m_bSizeLUTUpdated = true;
	}
}

void CEffect_Emitter::Set_RotCurve(const _float* pRotDegrees, _bool bEnabled)
{
	m_bRotOverLife = bEnabled;
	if (bEnabled && pRotDegrees && m_pRotLUT
		&& (!m_bRotLUTUpdated || memcmp(m_CachedRotLUT, pRotDegrees, sizeof(_float) * LUT_SIZE) != 0))
	{
		memcpy(m_CachedRotLUT, pRotDegrees, sizeof(_float) * LUT_SIZE);
		m_pContext->UpdateSubresource(m_pRotLUT->Get_Buffer(), 0, nullptr, pRotDegrees, 0, 0);
		m_bRotLUTUpdated = true;
	}
}

void CEffect_Emitter::Set_SpeedCurve(const _float* pSpeeds, _bool bEnabled)
{
	m_bSpeedOverLife = bEnabled;
	if (bEnabled && pSpeeds && m_pSpeedLUT
		&& (!m_bSpeedLUTUpdated || memcmp(m_CachedSpeedLUT, pSpeeds, sizeof(_float) * LUT_SIZE) != 0))
	{
		memcpy(m_CachedSpeedLUT, pSpeeds, sizeof(_float) * LUT_SIZE);
		m_pContext->UpdateSubresource(m_pSpeedLUT->Get_Buffer(), 0, nullptr, pSpeeds, 0, 0);
		m_bSpeedLUTUpdated = true;
	}
}

void CEffect_Emitter::Set_Scale3DCurve(const _float3* pScale, _bool bEnabled)
{
	m_bScale3DOverLife = bEnabled;
	if (bEnabled && pScale && m_pScale3DLUT
		&& (!m_bScale3DLUTUpdated || memcmp(m_CachedScale3DLUT, pScale, sizeof(_float3) * LUT_SIZE) != 0))
	{
		memcpy(m_CachedScale3DLUT, pScale, sizeof(_float3) * LUT_SIZE);
		m_pContext->UpdateSubresource(m_pScale3DLUT->Get_Buffer(), 0, nullptr, pScale, 0, 0);
		m_bScale3DLUTUpdated = true;
	}

	// Scale3D의 경우, 
	if (m_pScale3DLUT)
	{
		if (m_pMeshWrapper)
			m_pMeshWrapper->Set_Scale3D(m_pScale3DLUT->Get_SRV());
		if (m_pWrapper)
			m_pWrapper->Set_Scale3D(m_pScale3DLUT->Get_SRV());
	}
}

_bool CEffect_Emitter::Curves_Changed_Update(const vector<ColorKeyframe>& color, _bool bColorEn,
											 const vector<FloatKeyframe>& size, _bool bSizeEn,
											 const vector<FloatKeyframe>& rot, _bool bRotEn,
											 const vector<FloatKeyframe>& sx, const vector<FloatKeyframe>& sy,
											 const vector<FloatKeyframe>& sz, _bool bScale3DEn,
											 const vector<FloatKeyframe>& speed, _bool bSpeedEn)
{
	if (m_bCurvesInit &&
		bColorEn == m_bLastColorEn && SameKeys(color, m_LastColorKeys) &&
		bSizeEn == m_bLastSizeEn && SameKeys(size, m_LastSizeKeys) &&
		bRotEn == m_bLastRotEn && SameKeys(rot, m_LastRotKeys) &&
		bScale3DEn == m_bLastScale3DEn &&
		SameKeys(sx, m_LastSXKeys) && SameKeys(sy, m_LastSYKeys) && SameKeys(sz, m_LastSZKeys) &&
		bSpeedEn == m_bLastSpeedEn && SameKeys(speed, m_LastSpeedKeys))
	{
		return false;
	}

	m_LastColorKeys = color;	m_bLastColorEn = bColorEn;
	m_LastSizeKeys = size;	m_bLastSizeEn = bSizeEn;
	m_LastRotKeys = rot; m_bLastRotEn = bRotEn;
	m_LastSXKeys = sx; m_LastSYKeys = sy; m_LastSZKeys = sz; m_bLastScale3DEn = bScale3DEn;
	m_LastSpeedKeys = speed; m_bLastSpeedEn = bSpeedEn;
	m_bCurvesInit = true;
	return true;
}

void CEffect_Emitter::Reset()
{
	m_bInitialized = false;
	m_fSpawnAccum = 0.f;
	m_fEmitTime = 0.f;
	m_iPendingBurst = 0;
	m_bPrevEmitValid = false;
	m_bCurvesInit = false;
	m_bBoneEmit = false;
	m_iBoneCount = 0;
	m_iBoneFrameSeed = 0;
	m_bPinSourceValid = false;
	m_pGlobalPaletteSRV = nullptr;
}

void CEffect_Emitter::Set_Material(_uint iBlendMode, _bool bZWrite, _bool bZTest, const string& strDiffusePath)
{
	m_iBlendMode = iBlendMode;
	m_bZWrite = bZWrite;
	m_bZTest = bZTest;

	if (strDiffusePath == m_strDiffusePath)
		return;

	m_strDiffusePath = strDiffusePath;

	Safe_Release(m_pDiffuseTex);
	m_pDiffuseTex = LoadTexture(strDiffusePath);

	ID3D11ShaderResourceView* pDiffuseSRV = m_pDiffuseTex ? m_pDiffuseTex->Get_SRV() : nullptr;
	m_bUseTexture = (pDiffuseSRV != nullptr);
	if (m_pWrapper)
		m_pWrapper->Set_Diffuse(pDiffuseSRV);
	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_Diffuse(pDiffuseSRV);
}

void CEffect_Emitter::Set_UVDistort(const string& strNoiseAPath, const string& strNoiseBPath)
{
	if (strNoiseAPath == m_strNoiseAPath && strNoiseBPath == m_strNoiseBPath)
		return;

	m_strNoiseAPath = strNoiseAPath;
	m_strNoiseBPath = strNoiseBPath;

	Safe_Release(m_pNoiseATex);
	Safe_Release(m_pNoiseBTex);

	m_pNoiseATex = LoadTexture(strNoiseAPath);
	m_pNoiseBTex = LoadTexture(strNoiseBPath);

	ID3D11ShaderResourceView* pNoiseASRV = m_pNoiseATex ? m_pNoiseATex->Get_SRV() : nullptr;
	ID3D11ShaderResourceView* pNoiseBSRV = m_pNoiseBTex ? m_pNoiseBTex->Get_SRV() : nullptr;
	if (m_pMeshWrapper)
	{
		m_pMeshWrapper->Set_NoiseA(pNoiseASRV);
		m_pMeshWrapper->Set_NoiseB(pNoiseBSRV);
	}
	if (m_pWrapper)
	{
		m_pWrapper->Set_NoiseA(pNoiseASRV);
		m_pWrapper->Set_NoiseB(pNoiseBSRV);
	}
}

void CEffect_Emitter::Set_Emissive(const string& strEmissiveAPath, const string& strEmissiveBPath)
{
	if (strEmissiveAPath != m_strEmissiveAPath)
	{
		m_strEmissiveAPath = strEmissiveAPath;

		Safe_Release(m_pEmissiveATex);

		m_pEmissiveATex = LoadTexture(strEmissiveAPath);

		ID3D11ShaderResourceView* pEmisASRV = m_pEmissiveATex ? m_pEmissiveATex->Get_SRV() : nullptr;
		if (m_pMeshWrapper)
			m_pMeshWrapper->Set_EmissiveA(pEmisASRV);
		if (m_pWrapper)
			m_pWrapper->Set_EmissiveA(pEmisASRV);
	}

	if (strEmissiveBPath != m_strEmissiveBPath)
	{
		m_strEmissiveBPath = strEmissiveBPath;

		Safe_Release(m_pEmissiveBTex);

		m_pEmissiveBTex = LoadTexture(strEmissiveBPath);

		ID3D11ShaderResourceView* pEmisBSRV = m_pEmissiveBTex ? m_pEmissiveBTex->Get_SRV() : nullptr;
		if (m_pMeshWrapper)
			m_pMeshWrapper->Set_EmissiveB(pEmisBSRV);
		if (m_pWrapper)
			m_pWrapper->Set_EmissiveB(pEmisBSRV);
	}
}

void CEffect_Emitter::Set_Mask(const string& strMaskPath)
{
	if (strMaskPath == m_strShapeMaskPath)
		return;

	m_strShapeMaskPath = strMaskPath;

	Safe_Release(m_pShapeMaskTex);

	m_pShapeMaskTex = LoadTexture(strMaskPath);

	ID3D11ShaderResourceView* pMaskSRV = m_pShapeMaskTex ? m_pShapeMaskTex->Get_SRV() : nullptr;
	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_ShapeMask(pMaskSRV);
	if (m_pWrapper)
		m_pWrapper->Set_ShapeMask(pMaskSRV);
}

void CEffect_Emitter::Set_DistortNoise(const string& strDistortPath)
{
	if (strDistortPath == m_strDistortNoisePath)
		return;

	m_strDistortNoisePath = strDistortPath;

	Safe_Release(m_pDistortNoiseTex);

	m_pDistortNoiseTex = LoadTexture(strDistortPath);

	ID3D11ShaderResourceView* pNoiseSRV = m_pDistortNoiseTex ? m_pDistortNoiseTex->Get_SRV() : nullptr;
	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_DistortNoise(pNoiseSRV);
	if (m_pWrapper)
		m_pWrapper->Set_DistortNoise(pNoiseSRV);
}

void CEffect_Emitter::Set_VectorField(const string& strPath, _bool bEnabled,
	_float fIntensity, _float fTightness, const _float3& vTileSize)
{
	m_bUseVF = bEnabled;
	m_fVFIntensity = fIntensity;
	m_fVFTightness = fTightness;
	m_vVFTileSize = vTileSize;

	if (m_pVFSampler == nullptr)
	{
		D3D11_SAMPLER_DESC tSampler{};
		tSampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		tSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		tSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		tSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		tSampler.MinLOD = 0.f;
		tSampler.MaxLOD = D3D11_FLOAT32_MAX;
		m_pDevice->CreateSamplerState(&tSampler, &m_pVFSampler);
	}

	if (strPath == m_strVFPath)
		return;

	m_strVFPath = strPath;

	Safe_Release(m_pVectorField);
	m_pVectorField = LoadTexture(strPath);
}

void CEffect_Emitter::Set_Mesh(const string& strMeshPath, _uint iMeshIndex)
{
	if (m_pMeshWrapper == nullptr)
		return;

	if (strMeshPath == m_strMeshPath && iMeshIndex == m_iMeshIndex)
		return;

	m_strMeshPath = strMeshPath;
	m_iMeshIndex = iMeshIndex;

	HRESULT hr = E_FAIL;
	if (!strMeshPath.empty())
	{
		int iLen = MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, nullptr, 0);
		wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
		if (iLen > 0)
			MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, &strWPath[0], iLen);

		hr = m_pMeshWrapper->Load_StaticMesh(strWPath, iMeshIndex);
	}

	if (FAILED(hr))
		m_pMeshWrapper->Build_Quad();

	if (m_pMeshIndirectArgs)
	{
		_uint args[5] = { m_pMeshWrapper->Get_IndexCount(), 0, 0, 0, 0 };
		m_pContext->UpdateSubresource(m_pMeshIndirectArgs, 0, nullptr, args, 0, 0);
	}

}

void CEffect_Emitter::Set_EmitMesh(const string& strMeshPath, _uint iMeshIndex, _float fScale, _bool bEnabled)
{
	m_fEmitMeshScale = (fScale > 0.0001f) ? fScale : 1.f;

	if (!bEnabled || strMeshPath.empty())
	{
		m_bMeshEmit = false;
		return;
	}

	if (strMeshPath == m_strEmitMeshPath && iMeshIndex == m_iEmitMeshIndex && m_pMeshEmitBuf != nullptr)
	{
		m_bMeshEmit = (m_iMeshEmitTriCount > 0);
		return;
	}

	m_strEmitMeshPath = strMeshPath;
	m_iEmitMeshIndex = iMeshIndex;

	int iLen = MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, &strWPath[0], iLen);

	vector<VTXMESH> verts;
	vector<_uint> indices;
	if (FAILED(Load_StaticMeshGeometry(strWPath, iMeshIndex, verts, indices)))
	{
		Safe_Release(m_pMeshEmitBuf);
		m_iMeshEmitTriCount = 0;
		m_bMeshEmit = false;
		return;
	}

	const _uint iTriCount = static_cast<_uint>(indices.size() / 3);
	if (iTriCount == 0)
	{
		Safe_Release(m_pMeshEmitBuf);
		m_iMeshEmitTriCount = 0;
		m_bMeshEmit = false;
		return;
	}

	vector<MeshEmitTri> Tris(iTriCount);
	_float fTotalArea = 0.f;
	for (_uint i = 0; i < iTriCount; ++i)
	{
		const VTXMESH& v0 = verts[indices[i * 3]];
		const VTXMESH& v1 = verts[indices[i * 3 + 1]];
		const VTXMESH& v2 = verts[indices[i * 3 + 2]];

		MeshEmitTri& t = Tris[i];
		t.p0 = v0.vPosition;
		t.p1 = v1.vPosition;
		t.p2 = v2.vPosition;

		_vector vP0 = XMLoadFloat3(&v0.vPosition);
		_vector vE1 = XMVectorSubtract(XMLoadFloat3(&v1.vPosition), vP0);
		_vector vE2 = XMVectorSubtract(XMLoadFloat3(&v2.vPosition), vP0);
		_vector vCross = XMVector3Cross(vE1, vE2);
		_float fArea = 0.5f * XMVectorGetX(XMVector3Length(vCross));

		_vector vNAvg = XMVectorAdd(XMVectorAdd(
			XMLoadFloat3(&v0.vNormal), XMLoadFloat3(&v1.vNormal)), XMLoadFloat3(&v2.vNormal));
		if (XMVectorGetX(XMVector3LengthSq(vNAvg)) < 1e-8f)
			vNAvg = vCross;
		XMStoreFloat3(&t.nrm, XMVector3Normalize(vNAvg));

		fTotalArea += fArea;
		t.fCdf = fTotalArea;
	}

	if (fTotalArea > 1e-8f)
	{
		const _float fInv = 1.f / fTotalArea;
		for (_uint i = 0; i < iTriCount; ++i)
			Tris[i].fCdf *= fInv;
	}
	else
	{
		for (_uint i = 0; i < iTriCount; ++i)
			Tris[i].fCdf = (i + 1) / static_cast<_float>(iTriCount);
	}
	Tris[iTriCount - 1].fCdf = 1.f;

	Safe_Release(m_pMeshEmitBuf);
	CStructuredBuffer::SB_DESC tDesc{ sizeof(MeshEmitTri), iTriCount, Tris.data(), false };
	m_pMeshEmitBuf = CStructuredBuffer::Create(m_pDevice, m_pContext, tDesc);
	if (m_pMeshEmitBuf == nullptr)
	{
		m_iMeshEmitTriCount = 0;
		m_bMeshEmit = false;
		return;
	}

	m_iMeshEmitTriCount = iTriCount;
	m_bMeshEmit = true;
}

void CEffect_Emitter::Set_EmitSkinnedMesh(const string& strMeshPath, _int iMeshIndex, _bool bEnabled)
{
	if (!bEnabled || strMeshPath.empty())
	{
		m_bMeshSkinned = false;
		return;
	}

	if (strMeshPath == m_strPinMeshPath && iMeshIndex == m_iPinMeshIndex
		&& m_iPinGroupCount == 1 && m_pSkinOutBuf != nullptr)
	{
		m_bMeshSkinned = (m_iPinTriCount > 0);
		return;
	}

	int iLen = MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strMeshPath.c_str(), -1, &strWPath[0], iLen);

	vector<SKIN_IN_VTX> SkinIn;
	vector<_uint> indices;

	if (FAILED(Load_SkinnedMeshMerged(strWPath, iMeshIndex, 0, SkinIn, indices)))
	{
		Safe_Release(m_pSkinInBuf);
		Safe_Release(m_pSkinOutBuf);
		Safe_Release(m_pPinIndexBuf);
		Safe_Release(m_pPinCDFBuf);
		m_iSkinVertexCount = 0;
		m_iPinIndexCount = 0;
		m_iPinTriCount = 0;
		m_iPinGroupCount = 0;
		m_bMeshSkinned = false;
		return;
	}

	CStructuredBuffer* pIn = nullptr; CStructuredBuffer* pOut = nullptr;
	CStructuredBuffer* pIdx = nullptr; CStructuredBuffer* pCDF = nullptr;
	_uint iVtx = 0, iIdx = 0, iTri = 0;

	if (!BuildSkinCache(m_pDevice, m_pContext, SkinIn, indices,
		&pIn, &pOut, &pIdx, &pCDF, iVtx, iIdx, iTri))
	{
		m_bMeshSkinned = false;
		return;
	}

	Safe_Release(m_pSkinInBuf);
	Safe_Release(m_pSkinOutBuf);
	Safe_Release(m_pPinIndexBuf);
	Safe_Release(m_pPinCDFBuf);

	m_pSkinInBuf = pIn;	m_pSkinOutBuf = pOut;
	m_pPinIndexBuf = pIdx;	m_pPinCDFBuf = pCDF;

	m_strPinMeshPath = strMeshPath;
	m_iPinMeshIndex = iMeshIndex;
	m_PinGroupPaths.clear();
	m_PinGroupPaths.push_back(strMeshPath);
	m_iPinGroupCount = 1;
	m_iSkinVertexCount = iVtx;
	m_iPinIndexCount = iIdx;
	m_iPinTriCount = iTri;
	m_bMeshSkinned = true;
}

void CEffect_Emitter::Set_EmitSkinnedMeshGroups(const vector<string>& GroupPaths, _bool bEnabled)
{
	if (!bEnabled || GroupPaths.empty())
	{
		m_bMeshSkinned = false;
		return;
	}

	const _uint iGroups = min(static_cast<_uint>(GroupPaths.size()), MAX_PIN_GROUPS);

	if (m_iPinGroupCount == iGroups && m_pSkinOutBuf != nullptr
		&& m_PinGroupPaths.size() == iGroups)
	{
		_bool bSame = true;
		for (_uint g = 0; g < iGroups; ++g)
		{
			if (m_PinGroupPaths[g] != GroupPaths[g])
			{
				bSame = false;
				break;
			}
		}
		if (bSame)
		{
			m_bMeshSkinned = (m_iPinTriCount > 0);
			return;
		}
	}

	vector<wstring> WPaths;
	WPaths.reserve(iGroups);
	for (_uint g = 0; g < iGroups; ++g)
	{
		const string& s = GroupPaths[g];
		int iLen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
		wstring w(iLen > 0 ? iLen - 1 : 0, L'\0');
		if (iLen > 0)
			MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], iLen);
		WPaths.push_back(std::move(w));
	}

	vector<SKIN_IN_VTX> SkinIn;
	vector<_uint> indices;

	if (FAILED(Load_SkinnedMeshGroups(WPaths, SkinIn, indices)))
	{
		Safe_Release(m_pSkinInBuf);
		Safe_Release(m_pSkinOutBuf);
		Safe_Release(m_pPinIndexBuf);
		Safe_Release(m_pPinCDFBuf);
		m_iSkinVertexCount = 0;
		m_iPinIndexCount = 0;
		m_iPinTriCount = 0;
		m_bMeshSkinned = false;
		return;
	}

	CStructuredBuffer* pIn = nullptr; CStructuredBuffer* pOut = nullptr;
	CStructuredBuffer* pIdx = nullptr; CStructuredBuffer* pCDF = nullptr;
	_uint iVtx = 0, iIdx = 0, iTri = 0;
	if (!BuildSkinCache(m_pDevice, m_pContext, SkinIn, indices,
		&pIn, &pOut, &pIdx, &pCDF, iVtx, iIdx, iTri))
	{
		m_bMeshSkinned = false;
		return;
	}


	Safe_Release(m_pSkinInBuf);
	Safe_Release(m_pSkinOutBuf);
	Safe_Release(m_pPinIndexBuf);
	Safe_Release(m_pPinCDFBuf);

	m_pSkinInBuf = pIn;		m_pSkinOutBuf = pOut;
	m_pPinIndexBuf = pIdx;	m_pPinCDFBuf = pCDF;

	m_strPinMeshPath = GroupPaths[0];
	m_iPinMeshIndex = -1;
	m_PinGroupPaths.assign(GroupPaths.begin(), GroupPaths.begin() + iGroups);
	m_iPinGroupCount = iGroups;
	m_iSkinVertexCount = iVtx;
	m_iPinIndexCount = iIdx;
	m_iPinTriCount = iTri;
	m_bMeshSkinned = true;
}

void CEffect_Emitter::Set_EmitMeshSkinSource(ID3D11ShaderResourceView* pPaletteSRV, const _uint* pGroupBases, _uint iGroupCount, _uint iPaletteCapacity, const _float4x4& matWorld)
{
	if (pPaletteSRV == nullptr || iPaletteCapacity == 0 || pGroupBases == nullptr || iGroupCount == 0)
	{
		m_bPinSourceValid = false;
		return;
	}


	m_pGlobalPaletteSRV = pPaletteSRV;
	const _uint iCount = min(iGroupCount, MAX_PIN_GROUPS);
	for (_uint g = 0; g < iCount; ++g)
		m_PinGroupBase[g] = pGroupBases[g];
	m_iPinSourceGroupCount = iCount;
	m_iPaletteCapacity = iPaletteCapacity;
	XMStoreFloat4x4(&m_matPinWorld, XMMatrixTranspose(XMLoadFloat4x4(&matWorld)));
	m_bPinSourceValid = true;
}

void CEffect_Emitter::Set_Dissolve(const string& strDissolvePath)
{
	if (strDissolvePath == m_strDissolvePath)
		return;

	m_strDissolvePath = strDissolvePath;

	Safe_Release(m_pDissolveTex);

	m_pDissolveTex = LoadTexture(strDissolvePath);

	ID3D11ShaderResourceView* pDissolveSRV = m_pDissolveTex ? m_pDissolveTex->Get_SRV() : nullptr;
	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_Dissolve(pDissolveSRV);
	if (m_pWrapper)
		m_pWrapper->Set_Dissolve(pDissolveSRV);
}

void CEffect_Emitter::Set_Fresnel(const string& strCrackPath)
{
	if (strCrackPath == m_strCrackPath)
		return;

	m_strCrackPath = strCrackPath;

	Safe_Release(m_pCrackTex);

	m_pCrackTex = LoadTexture(strCrackPath);

	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_Crack(m_pCrackTex ? m_pCrackTex->Get_SRV() : nullptr);
}

void CEffect_Emitter::Activate(void* pArg)
{
	Set_Active();
	Reset();
}

void CEffect_Emitter::InActivate()
{
	Set_InActive();
	Reset();
}

void CEffect_Emitter::Set_BoneSources(const _float4x4* pWorldMats, _uint iCount, _bool bInheritRot, _bool bAttach)
{
	if (pWorldMats == nullptr || iCount == 0)
	{
		m_bBoneEmit = false;
		m_iBoneCount = 0;
		m_bBoneAttach = false;
		return;
	}

	// 용량 부족 시 재생성
	if (m_pBoneSrcBuf == nullptr || iCount > m_iBoneSrcCapacity)
	{
		Safe_Release(m_pBoneSrcBuf);
		CStructuredBuffer::SB_DESC tDesc{ sizeof(_float4x4), iCount, nullptr, false };
		m_pBoneSrcBuf = CStructuredBuffer::Create(m_pDevice, m_pContext, tDesc);
		if (m_pBoneSrcBuf == nullptr)
		{
			m_bBoneEmit = false;
			m_iBoneCount = 0;
			m_iBoneSrcCapacity = 0;
			return;
		}
		m_iBoneSrcCapacity = iCount;
	}

	// 셰이더로 들어가기 때문에 전치해서 저장함.
	m_BoneUploadScratch.resize((iCount));
	for (_uint i = 0; i < iCount; ++i)
		XMStoreFloat4x4(&m_BoneUploadScratch[i], XMMatrixTranspose(XMLoadFloat4x4(&pWorldMats[i])));

	D3D11_BOX tBox{};
	tBox.left = 0;
	tBox.right = iCount * sizeof(_float4x4);
	tBox.top = 0; tBox.bottom = 1;
	tBox.front = 0; tBox.back = 1;
	m_pContext->UpdateSubresource(m_pBoneSrcBuf->Get_Buffer(), 0, &tBox, m_BoneUploadScratch.data(), 0, 0);

	m_iBoneCount = iCount;
	m_bBoneInheritRot = bInheritRot;
	m_bBoneEmit = true;
	m_bBoneAttach = bAttach;
}

void CEffect_Emitter::Set_SpawnSource(ID3D11ShaderResourceView* pPoolSRV, ID3D11ShaderResourceView* pAliveSRV, ID3D11ShaderResourceView* pAliveCountSRV, _bool bEnabled)
{
	if (!bEnabled || pPoolSRV == nullptr || pAliveSRV == nullptr || pAliveCountSRV == nullptr)
	{
		m_bSpawnFromSource = false;
		m_pSrcPoolSRV = nullptr;
		m_pSrcAliveSRV = nullptr;
		m_pSrcCountSRV = nullptr;
		return;
	}

	m_bSpawnFromSource = true;
	m_pSrcPoolSRV = pPoolSRV;
	m_pSrcAliveSRV = pAliveSRV;
	m_pSrcCountSRV = pAliveCountSRV;
}

ID3D11ShaderResourceView* CEffect_Emitter::Get_PoolSRV() const
{
	return (m_pPool != nullptr) ? m_pPool->Get_SRV() : nullptr;
}

ID3D11ShaderResourceView* CEffect_Emitter::Get_AliveSRV() const
{
	return (m_pAliveList != nullptr) ? m_pAliveList->Get_SRV() : nullptr;
}

HRESULT CEffect_Emitter::Ready_Pool_CPU()
{
	vector<VTXPARTICLE_GPU> Particles(m_iCapacity);

	for (_uint i = 0; i < m_iCapacity; ++i)
	{
		VTXPARTICLE_GPU& p = Particles[i];

		p.vPos = { 0.f, 0.f, 0.f };
		p.vVelocity = { 0.f, 0.f, 0.f };
		p.fSize = 0.1f;
		p.vColor = { 0.2f, 0.8f, 1.f, 1.f };
		p.fAge = 0.f;
		p.fLifeTime = 0.f;
		p.vSpawnT = { 1.f, 0.f, 0.f, 0.f };
		p.vSpawnB = { 0.f, 1.f, 0.f, 0.f };
	}

	CStructuredBuffer::SB_DESC desc{};
	desc.iElementStride = sizeof(VTXPARTICLE_GPU);
	desc.iElementCount = m_iCapacity;
	desc.pInitialData = Particles.data();
	desc.bAppend = false;
	m_pPool = CStructuredBuffer::Create(m_pDevice, m_pContext, desc);
	
	if (m_pPool == nullptr)
		return E_FAIL;

	return S_OK;
}

HRESULT CEffect_Emitter::Ready_ComputeAndShaders()
{
	auto CreateCB = [&](_uint iSize, ID3D11Buffer** ppOut) -> HRESULT
		{
			D3D11_BUFFER_DESC bd{};
			bd.ByteWidth = iSize;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			return m_pDevice->CreateBuffer(&bd, nullptr, ppOut);
		};

 	if (FAILED(CreateCB(sizeof(SimCB), &m_pSimCB)))
		return E_FAIL;

	if (FAILED(CreateCB(sizeof(CamCB), &m_pCameraCB)))
		return E_FAIL;

	if (FAILED(CreateCB(sizeof(RenderCB), &m_pRenderCB)))
		return E_FAIL;

	if (FAILED(CreateCB(sizeof(EmitCB), &m_pEmitCB)))
		return E_FAIL;

	if (FAILED(CreateCB(sizeof(MeshSkinCB), &m_pMeshSkinCB)))
		return E_FAIL;

	if (FAILED(Ready_LifecycleBuffers()))
		return E_FAIL;

	m_pWrapper = CVIBuffer_GPUParticle::Create(m_pDevice, m_pContext);
	if (m_pWrapper == nullptr)
		return E_FAIL;
	m_pWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pAliveList->Get_SRV(), m_pIndirectArgs, 0, m_pCameraCB);
	m_pWrapper->Set_Render(m_pRenderCB);

	if (FAILED(Ready_MeshMode()))
		return E_FAIL;

	return S_OK;
}

HRESULT CEffect_Emitter::Ready_MeshMode()
{
	m_pMeshWrapper = CVIBuffer_GPUMesh::Create(m_pDevice, m_pContext);
	if (m_pMeshWrapper == nullptr)
		return E_FAIL;
	if (FAILED(m_pMeshWrapper->Build_Quad()))
		return E_FAIL;

	_uint args[5] = { m_pMeshWrapper->Get_IndexCount(), 0, 0, 0, 0 };
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = sizeof(args);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	D3D11_SUBRESOURCE_DATA init{ args, 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&bd, &init, &m_pMeshIndirectArgs)))
		return E_FAIL;

	m_pMeshWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pAliveList->Get_SRV(), m_pMeshIndirectArgs, 0, m_pCameraCB);
	m_pMeshWrapper->Set_Render(m_pRenderCB);

	return S_OK;
}

HRESULT CEffect_Emitter::Create_ListBuffers()
{
	CStructuredBuffer::SB_DESC listDesc{};
	listDesc.iElementStride = sizeof(_uint);
	listDesc.iElementCount = m_iCapacity;
	listDesc.pInitialData = nullptr;
	listDesc.bAppend = true;

	m_pDeadList = CStructuredBuffer::Create(m_pDevice, m_pContext, listDesc);
	if (m_pDeadList == nullptr) return E_FAIL;

	m_pAliveList = CStructuredBuffer::Create(m_pDevice, m_pContext, listDesc);
	if (m_pAliveList == nullptr) return E_FAIL;

	CStructuredBuffer::SB_DESC sortedDesc{ sizeof(_uint), m_iCapacity, nullptr, false };
	m_pSortedList = CStructuredBuffer::Create(m_pDevice, m_pContext, sortedDesc);
	if (m_pSortedList == nullptr) return E_FAIL;

	_uint args[4] = { 4, 0, 0, 0 };
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = sizeof(args);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	D3D11_SUBRESOURCE_DATA init{ args, 0, 0 };
	if (FAILED(m_pDevice->CreateBuffer(&bd, &init, &m_pIndirectArgs)))
		return E_FAIL;

	return S_OK;
}

HRESULT CEffect_Emitter::Resize_Pool(_uint iNewCount)
{
	if (iNewCount == 0)
		iNewCount = 1;

	m_iMaxCount = iNewCount;

	if (iNewCount <= m_iCapacity)
	{
		m_bInitialized = false;
		return S_OK;
	}

	m_iCapacity = iNewCount;

	Safe_Release(m_pPool);
	Safe_Release(m_pDeadList);
	Safe_Release(m_pAliveList);
	Safe_Release(m_pSortedList);
	Safe_Release(m_pIndirectArgs);

	if (FAILED(Ready_Pool_CPU()))
		return E_FAIL;
	if (FAILED(Create_ListBuffers()))
		return E_FAIL;

	if (m_pWrapper)
		m_pWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pAliveList->Get_SRV(), m_pIndirectArgs, 0, m_pCameraCB);

	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pAliveList->Get_SRV(), m_pMeshIndirectArgs, 0, m_pCameraCB);

	m_bSortDrawing = false;
	m_bInitialized = false;

	return S_OK;
}

HRESULT CEffect_Emitter::Ready_LifecycleBuffers()
{
	if (FAILED(Create_ListBuffers()))
		return E_FAIL;

	D3D11_BUFFER_DESC dcDesc{};
	dcDesc.ByteWidth = sizeof(_uint);
	dcDesc.Usage = D3D11_USAGE_DEFAULT;
	dcDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dcDesc.MiscFlags = 0;
	dcDesc.StructureByteStride = 0;
	if (FAILED(m_pDevice->CreateBuffer(&dcDesc, nullptr, &m_pDeadCountBuf)))
		return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC dcSRV{};
	dcSRV.Format = DXGI_FORMAT_R32_UINT;
	dcSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	dcSRV.Buffer.FirstElement = 0;
	dcSRV.Buffer.NumElements = 1;
	if (FAILED(m_pDevice->CreateShaderResourceView(m_pDeadCountBuf, &dcSRV, &m_pDeadCountSRV)))
		return E_FAIL;

	D3D11_BUFFER_DESC acDesc{};
	acDesc.ByteWidth = sizeof(_uint);
	acDesc.Usage = D3D11_USAGE_DEFAULT;
	acDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	acDesc.MiscFlags = 0;
	acDesc.StructureByteStride = 0;
	if (FAILED(m_pDevice->CreateBuffer(&acDesc, nullptr, &m_pAliveCountBuf)))
		return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC acSRV{};
	acSRV.Format = DXGI_FORMAT_R32_UINT;
	acSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	acSRV.Buffer.FirstElement = 0;
	acSRV.Buffer.NumElements = 1;
	if (FAILED(m_pDevice->CreateShaderResourceView(m_pAliveCountBuf, &acSRV, &m_pAliveCountSRV)))
		return E_FAIL;

	{
		vector<_float4> defColor(LUT_SIZE, _float4(1.f, 1.f, 1.f, 1.f));
		CStructuredBuffer::SB_DESC cDesc{ sizeof(_float4), LUT_SIZE, defColor.data(), false };
		m_pColorLUT = CStructuredBuffer::Create(m_pDevice, m_pContext, cDesc);
		if (m_pColorLUT == nullptr)
			return E_FAIL;

		vector<_float> defSize(LUT_SIZE, 0.1f);
		CStructuredBuffer::SB_DESC sDesc{ sizeof(_float), LUT_SIZE, defSize.data(), false };
		m_pSizeLUT = CStructuredBuffer::Create(m_pDevice, m_pContext, sDesc);
		if (m_pSizeLUT == nullptr) 
			return E_FAIL;

		vector<_float> defRot(LUT_SIZE, 0.f);
		CStructuredBuffer::SB_DESC rDesc{ sizeof(_float), LUT_SIZE, defRot.data(), false };
		m_pRotLUT = CStructuredBuffer::Create(m_pDevice, m_pContext, rDesc);
		if (m_pRotLUT == nullptr)
			return E_FAIL;

		vector<_float> defSpeed(LUT_SIZE, 1.f);
		CStructuredBuffer::SB_DESC spDesc{ sizeof(_float), LUT_SIZE, defSpeed.data(), false };
		m_pSpeedLUT = CStructuredBuffer::Create(m_pDevice, m_pContext, spDesc);
		if (m_pSpeedLUT == nullptr)
			return E_FAIL;

		vector<_float3> defScale(LUT_SIZE, _float3(1.f, 1.f, 1.f));
		CStructuredBuffer::SB_DESC s3Desc{ sizeof(_float3), LUT_SIZE, defScale.data(), false };
		m_pScale3DLUT = CStructuredBuffer::Create(m_pDevice, m_pContext, s3Desc);
		if (m_pScale3DLUT == nullptr)
			return E_FAIL;
	}

	return S_OK;
}

void CEffect_Emitter::Update(_float fTimeDelta)
{
	Dispatch_Lifecycle(fTimeDelta);
	Update_CameraCB();
	Update_RenderCB();

	if (m_pWrapper)
	{
		ID3D11ShaderResourceView* pDepthSRV = (m_tDesc.bSoftParticle || m_tDesc.bDistort)
			? m_pGameInstance->Get_RenderTargetSRV(L"Target_Depth") : nullptr;
		m_pWrapper->Set_Depth(pDepthSRV);
		m_pWrapper->Set_Scene(m_tDesc.bDistort ? m_pSceneSRV : nullptr);
	}
	if (m_pMeshWrapper)
		m_pMeshWrapper->Set_Scene(m_tDesc.bDistort ? m_pSceneSRV : nullptr);
}

void CEffect_Emitter::Dispatch_Lifecycle(_float fTimeDelta)
{
	if (m_tDesc.iMaxParticles != m_iMaxCount && m_tDesc.iMaxParticles > 0)
		Resize_Pool(m_tDesc.iMaxParticles);

	const _uint iSimGroups = (m_iMaxCount + 127) / 128;

	SimCB scb{};
	scb.fDt = fTimeDelta;
	scb.fGrav = m_tDesc.vGravity;
	scb.iMaxCount = m_iMaxCount;
	scb.bColorOL = m_bColorOverLife ? 1 : 0;
	scb.bSizeOL = m_bSizeOverLife ? 1 : 0;
	scb.iLutSize = LUT_SIZE;
	scb.fDrag = m_tDesc.fDrag;
	scb.fTime = m_fEmitTime;
	scb.iNoise = m_tDesc.bNoise ? 1 : 0;
	scb.iNoiseType = m_tDesc.iNoiseType;
	scb.fNoiseStrengh = m_tDesc.fNoiseStrength;
	scb.fNoiseFreq = m_tDesc.fNoiseFrequency;
	scb.fNoiseDamping = m_tDesc.fNoiseDamping;
	scb.fNoiseScroll = m_tDesc.fNoiseScroll;
	scb.bRotOverLife = m_bRotOverLife ? 1 : 0;
	scb.bVelocity = m_tDesc.bVelocity ? 1 : 0;
	scb.vLinearVel = m_tDesc.vLinearVel;
	scb.fOrbit = m_tDesc.fOrbit;
	scb.vOrbitCenter = m_tDesc.vOrbitCenter;
	XMStoreFloat3(&scb.vOrbitAxis,
		XMVector3TransformNormal(XMLoadFloat3(&m_tDesc.vOrbitAxis), XMLoadFloat4x4(&m_matExtraRot)));
	scb.bSpiral = m_tDesc.bSpiral ? 1 : 0;
	scb.fGravStart = m_tDesc.fGravStart;
	scb.fGravEnd = m_tDesc.fGravEnd;
	scb.fVelSpeedMin = m_tDesc.fVelSpeedMin;
	scb.fVelSpeedMax = m_tDesc.fVelSpeedMax;
	scb.fSpeedByAngle = m_tDesc.fSpeedByAngle;
	scb.fNoiseOffset = m_tDesc.fNoiseOffset;
	scb.bSizeMultiply = m_tDesc.bSizeMultiply ? 1 : 0;
	scb.bSpeedOL = m_bSpeedOverLife ? 1 : 0;
	scb.vEmitOrigin = m_tDesc.vEmitPos;
	scb.bUseVF = (m_bUseVF && m_pVectorField) ? 1 : 0;
	scb.vfIntensity = m_fVFIntensity;
	scb.vfTightness = m_fVFTightness;
	scb.vfTileSize = m_vVFTileSize;
	scb.bConverge = m_tDesc.bConverge ? 1 : 0;
	scb.fArrivalRadius = m_tDesc.fArrivalRadius;
	scb.bInfinite = m_tDesc.bInfinite ? 1 : 0;
	scb.iColorMode = m_tDesc.iColorCurveMode;
	scb.iSizeMode = m_tDesc.iSizeCurveMode;
	scb.iRotMode = m_tDesc.iRotCurveMode;
	scb.iSpeedMode = m_tDesc.iSpeedCurveMode;

	if (m_tDesc.iSimSpace == 1)
	{	
		_matrix matR = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(m_tDesc.vEmitRot.x),
			XMConvertToRadians(m_tDesc.vEmitRot.y),
			XMConvertToRadians(m_tDesc.vEmitRot.z)) * XMLoadFloat4x4(&m_matExtraRot);

		_matrix matRinv = XMMatrixTranspose(matR);
		_vector vEmitPos = XMLoadFloat3(&m_tDesc.vEmitPos);

		XMStoreFloat3(&scb.fGrav, XMVector3TransformNormal(XMLoadFloat3(&m_tDesc.vGravity), matRinv));
		XMStoreFloat3(&scb.vOrbitCenter,
			XMVector3TransformNormal(XMVectorSubtract(XMLoadFloat3(&m_tDesc.vOrbitCenter), vEmitPos), matRinv));
		XMStoreFloat3(&scb.vOrbitAxis, XMVector3TransformNormal(XMLoadFloat3(&m_tDesc.vOrbitAxis), matRinv));
		scb.vEmitOrigin = _float3(0.f, 0.f, 0.f);
	}

	MapCB(m_pContext, m_pSimCB, &scb, sizeof(scb));

	if (!m_bInitialized)
	{
		m_pInitCS->Bind_ConstantBuffer(0, m_pSimCB);
		m_pInitCS->Bind_UAV(0, m_pDeadList->Get_UAV(), 0);
		m_pInitCS->Bind_UAV(1, m_pPool->Get_UAV());
		m_pInitCS->Dispatch(iSimGroups);
		m_pInitCS->Unbind_UAV(0, 2);
		m_bInitialized = true;
	}

	m_fSpawnAccum += m_tDesc.fRate * fTimeDelta;
	_uint iSpawn = static_cast<_uint>(m_fSpawnAccum);
	m_fSpawnAccum -= iSpawn;
	m_fEmitTime += fTimeDelta;

	iSpawn += m_iPendingBurst;
	m_iPendingBurst = 0;

	if (iSpawn > m_iMaxCount)
		iSpawn = m_iMaxCount;

	const _float3 vCurEmit = (m_tDesc.iSimSpace == 1) ? _float3(0.f, 0.f, 0.f) : m_tDesc.vEmitPos;
	if (!m_bPrevEmitValid)
	{
		m_vPrevEmitPos = vCurEmit;
		m_vPrevEmitPos2 = vCurEmit;
		m_bPrevEmitValid = true;
	}

	const _bool bPinActive = (m_bMeshSkinned && m_bPinSourceValid
		&& m_pSkinInBuf != nullptr && m_pSkinOutBuf != nullptr
		&& m_pPinIndexBuf != nullptr && m_pPinCDFBuf != nullptr
		&& m_iPinTriCount > 0 && m_pMeshSkinCS != nullptr && m_pGlobalPaletteSRV != nullptr);

	if (bPinActive)
	{
		MeshSkinCB mcb{};
		mcb.iVertexCount = m_iSkinVertexCount;
		mcb.iBoneOffset = min(m_iPinGroupCount, m_iPinSourceGroupCount);
		mcb.iPaletteCapacity = m_iPaletteCapacity;
		mcb.matWorld = m_matPinWorld;
		for (_uint g = 0; g < MAX_SKIN_GROUPS; ++g)
			mcb.iGroupBase[g] = (g < m_iPinSourceGroupCount) ? m_PinGroupBase[g] : 0;
		MapCB(m_pContext, m_pMeshSkinCB, &mcb, sizeof(mcb));

		m_pMeshSkinCS->Bind_ConstantBuffer(0, m_pMeshSkinCB);
		m_pMeshSkinCS->Bind_SRV(0, m_pSkinInBuf->Get_SRV());
		m_pMeshSkinCS->Bind_SRV(1, m_pGlobalPaletteSRV);
		m_pMeshSkinCS->Bind_UAV(0, m_pSkinOutBuf->Get_UAV());
		m_pMeshSkinCS->Dispatch((m_iSkinVertexCount + 63) / 64);
		m_pMeshSkinCS->Unbind_UAV(0, 1);
		m_pMeshSkinCS->Unbind_SRV(0, 2);
	}

	if (iSpawn > 0)
	{
		EmitCB ecb{};
		ecb.iSpawnCount = iSpawn;
		ecb.fTime = m_fEmitTime;
		ecb.fLifeMin = m_tDesc.fLifeMin;
		ecb.fLifeMax = m_tDesc.fLifeMax;
		ecb.fSpeedMin = m_tDesc.fSpeedMin;
		ecb.fSpeedMax = m_tDesc.fSpeedMax;
		ecb.fSizeMin = m_tDesc.fSizeMin;
		ecb.fSizeMax = m_tDesc.fSizeMax;
		ecb.vColor = m_tDesc.vColor;
		ecb.fShapeRadius = m_tDesc.fShapeRadius;
		ecb.vBiasDir = m_tDesc.vBiasDir;
		ecb.fBiasWeight = m_tDesc.fBiasWeight;
		ecb.iShapeType = m_tDesc.iShapeType;
		ecb.bEdgeEmit = m_tDesc.bEdgeEmit;
		ecb.fStartRotMin = m_tDesc.fStartRotMin;
		ecb.fStartRotMax = m_tDesc.fStartRotMax;

		if (m_bBoneEmit)
		{
			ecb.vEmitPos = m_tDesc.vEmitPos;
			_matrix matRot = XMMatrixRotationRollPitchYaw(
				XMConvertToRadians(m_tDesc.vEmitRot.x),
				XMConvertToRadians(m_tDesc.vEmitRot.y),
				XMConvertToRadians(m_tDesc.vEmitRot.z));
			XMStoreFloat4x4(&ecb.matEmitRot, XMMatrixTranspose(matRot));
		}
		else if (m_tDesc.iSimSpace == 1)
		{
			ecb.vEmitPos = _float3(0.f, 0.f, 0.f);
			XMStoreFloat4x4(&ecb.matEmitRot, XMMatrixIdentity());
		}
		else
		{
			ecb.vEmitPos = m_tDesc.vEmitPos;
			_matrix matRot = XMMatrixRotationRollPitchYaw(
				XMConvertToRadians(m_tDesc.vEmitRot.x),
				XMConvertToRadians(m_tDesc.vEmitRot.y),
				XMConvertToRadians(m_tDesc.vEmitRot.z)) * XMLoadFloat4x4(&m_matExtraRot);
			XMStoreFloat4x4(&ecb.matEmitRot, XMMatrixTranspose(matRot));
		}

		ecb.vPrevEmitPos = m_tDesc.bInterpSpawn ? m_vPrevEmitPos : ecb.vEmitPos;
		ecb.vPrevEmitPos2 = m_tDesc.bInterpSpawn ? m_vPrevEmitPos2 : ecb.vEmitPos;
		ecb.bArcEnable = (m_tDesc.bInterpSpawn && m_tDesc.bSplineSpawn && m_bArcEnable) ? 1 : 0;
		ecb.vArcAxis = m_vArcAxis;
		ecb.fArcAngle = m_fArcAngle;
		ecb.bSplineEnable = (m_tDesc.bInterpSpawn && m_tDesc.bSplineSpawn && !ecb.bArcEnable) ? 1 : 0;
		ecb.vOrbitCenter = scb.vOrbitCenter;
		ecb.vOrbitAxis = scb.vOrbitAxis;
		ecb.bConverge = m_tDesc.bConverge ? 1 : 0;

		ecb.iBoneCount = m_bBoneEmit ? m_iBoneCount : 0;
		ecb.bBoneInheritRot = m_bBoneInheritRot ? 1 : 0;
		ecb.iFrameSeed = m_iBoneFrameSeed++;
		ecb.bBoneAttach = (m_bBoneEmit && m_bBoneAttach) ? 1 : 0;

		const _bool bMeshEmitActive = (m_bMeshEmit && m_pMeshEmitBuf != nullptr && m_iMeshEmitTriCount > 0);
		if (bPinActive)
		{
			ecb.iMeshPinned = 1;
			ecb.iMeshTriCount = m_iPinTriCount;
		}
		else
		{
			ecb.iMeshPinned = 0;
			ecb.iMeshTriCount = bMeshEmitActive ? m_iMeshEmitTriCount : 0;
		}
		ecb.fMeshScale = m_fEmitMeshScale;
		ecb.iMeshRelease = (bPinActive && m_tDesc.bSurfaceRelease) ? 1 : 0;

		const _bool bSrcActive = (m_bSpawnFromSource && m_pSrcCountSRV != nullptr
			&& m_pSrcAliveSRV != nullptr && m_pSrcCountSRV != nullptr);
		ecb.bSpawnFromSource = bSrcActive ? 1 : 0;
		ecb.fDt = fTimeDelta;

		MapCB(m_pContext, m_pEmitCB, &ecb, sizeof(ecb));

		m_pDeadList->Copy_Count(m_pDeadCountBuf, 0);

		m_pEmitCS->Bind_ConstantBuffer(0, m_pEmitCB);
		m_pEmitCS->Bind_SRV(0, m_pDeadCountSRV);
		m_pEmitCS->Bind_SRV(1, (m_bBoneEmit&& m_pBoneSrcBuf != nullptr) ? m_pBoneSrcBuf->Get_SRV() : nullptr);
		m_pEmitCS->Bind_SRV(2, bMeshEmitActive ? m_pMeshEmitBuf->Get_SRV() : nullptr);
		m_pEmitCS->Bind_SRV(3, bPinActive ? m_pPinCDFBuf->Get_SRV() : nullptr);
		m_pEmitCS->Bind_SRV(4, bPinActive ? m_pPinIndexBuf->Get_SRV() : nullptr);
		m_pEmitCS->Bind_SRV(5, bPinActive ? m_pSkinOutBuf->Get_SRV() : nullptr);
		m_pEmitCS->Bind_SRV(6, bSrcActive ? m_pSrcPoolSRV : nullptr);
		m_pEmitCS->Bind_SRV(7, bSrcActive ? m_pSrcAliveSRV : nullptr);
		m_pEmitCS->Bind_SRV(8, bSrcActive ? m_pSrcCountSRV : nullptr);
		m_pEmitCS->Bind_UAV(0, m_pPool->Get_UAV());
		m_pEmitCS->Bind_UAV(1, m_pDeadList->Get_UAV());
		m_pEmitCS->Dispatch((iSpawn + 63) / 64);
		m_pEmitCS->Unbind_UAV(0, 2);
		m_pEmitCS->Unbind_SRV(0, 9);
	}

	m_vPrevEmitPos2 = m_vPrevEmitPos;
	m_vPrevEmitPos = vCurEmit;

	m_pSimulateCS->Bind_ConstantBuffer(0, m_pSimCB);
	m_pSimulateCS->Bind_SRV(0, m_pColorLUT->Get_SRV());
	m_pSimulateCS->Bind_SRV(1, m_pSizeLUT->Get_SRV());
	m_pSimulateCS->Bind_SRV(2, m_pRotLUT->Get_SRV());
	m_pSimulateCS->Bind_SRV(3, (m_bUseVF && m_pVectorField) ? m_pVectorField->Get_SRV() : nullptr);
	m_pSimulateCS->Bind_SRV(4, m_pSpeedLUT->Get_SRV());
	m_pSimulateCS->Bind_SRV(5, bPinActive ? m_pSkinOutBuf->Get_SRV() : nullptr);
	m_pSimulateCS->Bind_SRV(6, bPinActive ? m_pPinIndexBuf->Get_SRV() : nullptr);
	m_pSimulateCS->Bind_SRV(7, (m_bBoneEmit&& m_bBoneAttach&& m_pBoneSrcBuf != nullptr) ? m_pBoneSrcBuf->Get_SRV() : nullptr);
	m_pSimulateCS->Bind_Sampler(0, m_pVFSampler);
	m_pSimulateCS->Bind_UAV(0, m_pPool->Get_UAV());
	m_pSimulateCS->Bind_UAV(1, m_pDeadList->Get_UAV());
	m_pSimulateCS->Bind_UAV(2, m_pAliveList->Get_UAV(), 0);
	m_pSimulateCS->Dispatch(iSimGroups);
	m_pSimulateCS->Unbind_UAV(0, 3);
	m_pSimulateCS->Unbind_SRV(0, 8);

	m_pAliveList->Copy_Count(m_pIndirectArgs, sizeof(_uint));
	if (m_pMeshIndirectArgs)
		m_pAliveList->Copy_Count(m_pMeshIndirectArgs, sizeof(_uint));

	m_pAliveList->Copy_Count(m_pAliveCountBuf, 0);

	// GPU 정렬 : 알파블렌드 + 단일 Dispatch 정렬 (2048 이하, 2D 파티클만 정렬)
	const _bool bNeedSort = (m_iBlendMode == 0) && (m_iCapacity <= 2048) &&
		(m_tDesc.iAlignment != 2) && (m_pSortCS != nullptr) && (m_pSortedList != nullptr);

	if (bNeedSort)
	{
		m_pSortCS->Bind_ConstantBuffer(0, m_pCameraCB);
		m_pSortCS->Bind_SRV(0, m_pPool->Get_SRV());
		m_pSortCS->Bind_SRV(1, m_pAliveList->Get_SRV());
		m_pSortCS->Bind_SRV(2, m_pAliveCountSRV);
		m_pSortCS->Bind_UAV(0, m_pSortedList->Get_UAV());
		m_pSortCS->Dispatch(1);
		m_pSortCS->Unbind_UAV(0, 1);
		m_pSortCS->Unbind_SRV(0, 3);

		if (!m_bSortDrawing && m_pWrapper)
		{
			m_pWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pSortedList->Get_SRV(), m_pIndirectArgs, 0, m_pCameraCB);
			m_bSortDrawing = true;
		}
	}
	else if (m_bSortDrawing && m_pWrapper)
	{
		m_pWrapper->Set_DrawSource(m_pPool->Get_SRV(), m_pAliveList->Get_SRV(), m_pIndirectArgs, 0, m_pCameraCB);
		m_bSortDrawing = false;
	}

	m_bPinSourceValid = false;
	m_pGlobalPaletteSRV = nullptr;
}

CTexture* CEffect_Emitter::Load_Cached_Texture(const wstring& strWPath)
{
	if (strWPath.empty() || m_pGameInstance == nullptr)
		return nullptr;

	wstring strPathCopy = strWPath;
	wstring strName = Get_FileNameFromPath(strPathCopy);

	CTexture* pTexture = nullptr;
	if (FAILED(m_pGameInstance->Get_Texture(strName, &pTexture)))
	{
		m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strWPath);
		m_pGameInstance->Get_Texture(strName, &pTexture);
	}
	return pTexture;
}

void CEffect_Emitter::Update_CameraCB()
{
	const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);
	const _float4* pRight = m_pGameInstance->Get_CamRight();
	const _float4* pLook = m_pGameInstance->Get_CamLook();
	const _float4* pCamPos = m_pGameInstance->Get_CamPosition();

	if (!pView || !pProj || !pRight || !pLook)
		return;

	_vector vRight = XMLoadFloat4(pRight);
	_vector vLook = XMLoadFloat4(pLook);
	_vector vUp = XMVector3NormalizeEst(XMVector3Cross(vLook, vRight));

	CamCB cb{};
	XMStoreFloat4x4(&cb.matVP, XMMatrixTranspose(XMLoadFloat4x4(pView) * XMLoadFloat4x4(pProj)));
	XMStoreFloat4(&cb.vRight, vRight);
	XMStoreFloat4(&cb.vUp, vUp);
	cb.vRight.w = m_bUseTexture ? 1.f : 0.f;
	if (pCamPos)
		cb.vCamPos = *pCamPos;
	MapCB(m_pContext, m_pCameraCB, &cb, sizeof(cb));
}

void CEffect_Emitter::Update_RenderCB()
{
	RenderCB cb{};
	cb.bFlipbook = m_tDesc.bFlipbook ? 1 : 0;
	cb.iCols = (m_tDesc.iFlipCols > 0) ? m_tDesc.iFlipCols : 1;
	cb.iRows = (m_tDesc.iFlipRows > 0) ? m_tDesc.iFlipRows : 1;
	cb.fFrameRate = m_tDesc.fFlipFrameRate;
	cb.bSeedBased = m_tDesc.bFlipSeedBased ? 1 : 0;
	cb.iFlipPlayMode = m_tDesc.iFlipPlayMode;
	cb.iFlipFixedFrame = m_tDesc.iFlipFixedFrame;
	cb.bSoft = m_tDesc.bSoftParticle ? 1 : 0;
	cb.fSoftFade = m_tDesc.fSoftFade;
	cb.fCamFar = 500.f;
	cb.iAlignment = m_tDesc.iAlignment;
	cb.fStretchScale = m_tDesc.fStretchScale;

	cb.iSpinMode = m_tDesc.iSpinMode;
	cb.vSpinAxis = m_tDesc.vSpinAxis;
	cb.vMeshScale = m_tDesc.vMeshScale;
	cb.fBasePower = m_tDesc.fBasePower;
	cb.fEmiIntensity = m_tDesc.fEmiIntensity;
	cb.fSphereHardness = m_tDesc.fSphereHardness;
	cb.fDiffuseSharpness = m_tDesc.fDiffuseSharpness;

	const _bool bBoneAttachRender = (m_bBoneEmit && m_bBoneAttach);
	cb.bLocalSpace = (!bBoneAttachRender && m_tDesc.iSimSpace == 1) ? 1 : 0;
	if (cb.bLocalSpace)
	{
		XMMATRIX matW =
			XMMatrixRotationRollPitchYaw(
				XMConvertToRadians(m_tDesc.vEmitRot.x),
				XMConvertToRadians(m_tDesc.vEmitRot.y),
				XMConvertToRadians(m_tDesc.vEmitRot.z)) *
			XMLoadFloat4x4(&m_matExtraRot) *
			XMMatrixTranslation(m_tDesc.vEmitPos.x, m_tDesc.vEmitPos.y, m_tDesc.vEmitPos.z);
		XMStoreFloat4x4(&cb.matEmitterWorld, XMMatrixTranspose(matW));
	}
	else
		XMStoreFloat4x4(&cb.matEmitterWorld, XMMatrixIdentity());

	cb.bUVDistort = m_tDesc.bUVDistort ? 1 : 0;
	cb.fUVDistortStr = m_tDesc.fUVDistortStr;
	cb.fTime = m_fEmitTime;
	cb.fBaseDensity = m_tDesc.fBaseDensity;
	cb.vUVPan = _float4(m_tDesc.vUVPanA.x, m_tDesc.vUVPanA.y, m_tDesc.vUVPanB.x, m_tDesc.vUVPanB.y);
	cb.vNoiseTile = _float4(m_tDesc.vNoiseTileA.x, m_tDesc.vNoiseTileA.y, m_tDesc.vNoiseTileB.x, m_tDesc.vNoiseTileB.y);

	cb.bMask = m_tDesc.bMask ? 1 : 0;
	cb.fMaskPower = m_tDesc.fMaskPower;
	cb.bMaskClamp = m_tDesc.bMaskClamp ? 1 : 0;
	cb.vMaskTileOffset = _float4(m_tDesc.vMaskTiling.x, m_tDesc.vMaskTiling.y, m_tDesc.vMaskOffset.x, m_tDesc.vMaskOffset.y);
	cb.fLinearOpacity = m_tDesc.bLinearOpacity ? 1.f : 0.f;

	cb.bEmissive = m_tDesc.bEmissive ? 1 : 0;
	cb.fEmisStr = m_tDesc.fEmisStr;
	cb.fEmisStrA = m_tDesc.fEmisStrA;
	cb.fEmisPower = m_tDesc.fEmisPower;
	cb.vEmisTileA = _float4(m_tDesc.vEmisTileA.x, m_tDesc.vEmisTileA.y, m_tDesc.vEmisPanA.x, m_tDesc.vEmisPanA.y);
	cb.vEmisTileB = _float4(m_tDesc.vEmisTileB.x, m_tDesc.vEmisTileB.y, m_tDesc.vEmisPanB.x, m_tDesc.vEmisPanB.y);
	cb.vEmisTint = _float4(m_tDesc.vEmisTint.x, m_tDesc.vEmisTint.y, m_tDesc.vEmisTint.z, 0.f);
	cb.vEmisExtra = _float4(m_tDesc.fEmisLerp, m_tDesc.fEmisUVNoiseStr, static_cast<_float>(m_tDesc.iEmisBlendMode), m_pEmissiveATex ? 1.f : 0.f);

	cb.bDissolve = m_tDesc.bDissolve ? 1 : 0;
	cb.fDissolveZoom = m_tDesc.fDissolveZoom;
	cb.fDissolvePower = m_tDesc.fDissolvePower;
	cb.fDissolveEdge = m_tDesc.fDissolveEdge;
	cb.vDissolveTiling = _float4(m_tDesc.vDissolveTiling.x, m_tDesc.vDissolveTiling.y, m_tDesc.fDissolveEdgeInt, m_tDesc.fDissolveThreshold);
	cb.vDissolveEdgeColor = _float4(m_tDesc.vDissolveEdgeColor.x, m_tDesc.vDissolveEdgeColor.y, m_tDesc.vDissolveEdgeColor.z, m_tDesc.bDissolveUseAge ? 1.f : 0.f);

	cb.bFresnel = m_tDesc.bFresnel ? (m_pCrackTex ? 2 : 1) : 0;
	cb.fFresPower = m_tDesc.fFresPower;
	cb.fFresOutInt = m_tDesc.fFresOutInt;
	cb.fFresInInt = m_tDesc.fFresInInt;
	cb.vFresColor = _float4(m_tDesc.vFresColor.x, m_tDesc.vFresColor.y, m_tDesc.vFresColor.z, m_tDesc.fFresInverseStr);

	cb.bScale3D = m_tDesc.bScale3D ? 1 : 0;
	cb.vEmitOrigin = m_tDesc.vEmitPos;

	cb.bDistort = m_tDesc.bDistort ? 1 : 0;
	cb.fDistortStr = m_tDesc.fDistortStr;
	cb.iDistortMode = m_tDesc.iDistortMode;
	cb.bDistortDepth = m_tDesc.bDistortDepthMask ? 1 : 0;
	cb.vDistortPan = m_tDesc.vDistortPan;
	cb.vDistortTile = m_tDesc.vDistortTile;
	cb.bDistortTex = m_pDistortNoiseTex ? 1 : 0;
	cb.vPivot = m_tDesc.vPivot;
	const _float2 vVp = m_pGameInstance->Get_ViewportDesc();
	cb.vScreenSize = _float2(vVp.x > 1.f ? vVp.x : 1.f, vVp.y > 1.f ? vVp.y : 1.f);

	MapCB(m_pContext, m_pRenderCB, &cb, sizeof(cb));
}

HRESULT CEffect_Emitter::Render()
{
	EffectCommand cmd{};
	cmd.header.etype = RENDER_TYPE::EFFECT;
	cmd.header.iCommandSize = sizeof(EffectCommand);
	cmd.header.iSortKey = 0;
	cmd.header.fDepth = 0.f;

	const _bool bMesh = (m_tDesc.iAlignment == 2);
	const _bool bWBOIT = { m_iBlendMode == 0 };

	cmd.pVIBuffer = bMesh ? static_cast<CVIBuffer*>(m_pMeshWrapper) : static_cast<CVIBuffer*>(m_pWrapper);
	cmd.pVS = bMesh ? m_pMeshVS : m_pVS;
	cmd.pPS = bMesh ? m_pMeshPS : m_pPS;
	cmd.pLayout = bMesh ? m_pMeshLayout : nullptr;
	cmd.pMaterial = nullptr;
	cmd.iBlendMode = m_iBlendMode;
	cmd.bZWrite = m_bZWrite ? 1 : 0;
	cmd.bZTest = m_bZTest ? 1 : 0;
	cmd.iAlignment = m_tDesc.iAlignment;
	XMStoreFloat4x4(&cmd.worldMatrix, XMMatrixIdentity());

	m_pGameInstance->Submit_Effect(cmd);

	return S_OK;
}

CEffect_Emitter* CEffect_Emitter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEffect_Emitter* pInstance = new CEffect_Emitter(pDevice, pContext);
	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Created : CEffect_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CEffect_Emitter::Free()
{
	__super::Free();

	Safe_Release(m_pDiffuseTex);
	Safe_Release(m_pNoiseATex);
	Safe_Release(m_pNoiseBTex);
	Safe_Release(m_pDistortNoiseTex);
	Safe_Release(m_pShapeMaskTex);
	Safe_Release(m_pEmissiveATex);
	Safe_Release(m_pEmissiveBTex);
	Safe_Release(m_pDissolveTex);
	Safe_Release(m_pCrackTex);
	Safe_Release(m_pVectorField);
	Safe_Release(m_pVFSampler);
	Safe_Release(m_pSizeLUT);
	Safe_Release(m_pColorLUT);
	Safe_Release(m_pRotLUT);
	Safe_Release(m_pScale3DLUT);
	Safe_Release(m_pSpeedLUT);
	Safe_Release(m_pBoneSrcBuf);
	Safe_Release(m_pMeshEmitBuf);
	Safe_Release(m_pSkinInBuf);
	Safe_Release(m_pSkinOutBuf);
	Safe_Release(m_pPinIndexBuf);
	Safe_Release(m_pPinCDFBuf);
	Safe_Release(m_pMeshSkinCB);
	Safe_Release(m_pDeadCountSRV);
	Safe_Release(m_pDeadCountBuf);
	Safe_Release(m_pAliveCountSRV);
	Safe_Release(m_pAliveCountBuf);
	Safe_Release(m_pSortedList);
	Safe_Release(m_pIndirectArgs);
	Safe_Release(m_pAliveList);
	Safe_Release(m_pDeadList);
	Safe_Release(m_pEmitCB);
	Safe_Release(m_pMeshIndirectArgs);
	Safe_Release(m_pMeshWrapper);
	Safe_Release(m_pRenderCB);
	Safe_Release(m_pCameraCB);
	Safe_Release(m_pSimCB);
	Safe_Release(m_pWrapper);
	Safe_Release(m_pPool);

	m_pVS = nullptr; m_pPS = nullptr; m_pPS_WBOIT = nullptr;
	m_pMeshVS = nullptr; m_pMeshPS = nullptr; m_pMeshPS_WBOIT = nullptr; m_pMeshLayout = nullptr;
	m_pSimulateCS = nullptr; m_pInitCS = nullptr; m_pEmitCS = nullptr; m_pSortCS = nullptr;
	m_pMeshSkinCS = nullptr;
}