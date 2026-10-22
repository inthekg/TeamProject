#pragma once
#include "Engine_Defines.h"
#include "Engine_Utility.h"

NS_BEGIN(Engine)

class CVIBuffer;
class CMaterial;
struct RenderCommand;

//랜더 커맨드의 헤더. 커맨드의 타입을 명시함 
struct alignas(16) RenderHeader
{
	RENDER_TYPE		etype			= { RENDER_TYPE::END };
	_uint			iCommandSize	= {};
    //정렬에 필요한 데이터들만 올림 
	_uint			iSortKey		= {}; 
	_float			fDepth			= {};

public:
	static _uint MakeSortKey(const _uint& iShaderID, const _uint&  iTextureID)
	{
		return Combine_ID(iShaderID, iTextureID);
	}

	//Texture - Shader 순 정렬 
	static bool RenderSort(const RenderCommand* pA, const RenderCommand* pB);

	//깊이 우선 정렬 
	static bool RenderSort_Depth(const RenderCommand* pA, const RenderCommand* pB);
};

struct alignas(16) RenderCommand
{
	RenderHeader				header;							//16
	XMFLOAT4X4					worldMatrix		= {};			//64

	CVIBuffer*					pVIBuffer		= { nullptr };
	ID3D11VertexShader*			pVS				= { nullptr };	//16
	ID3D11PixelShader*			pPS				= { nullptr };
	ID3D11InputLayout*			pLayout			= { nullptr };	//16
	//MaterialInstData			MaterialInstData = {};			//32
	CMaterial*					pMaterial		= { nullptr };
	_int						iAlphaClip		= { 0 };
	_float						fEmissiveAmount = { 0.f };		// 48

	_int						iRimProfile		= { 0 };
	_float						fRimAmount		= { 0.f };
	_float						fDissolveAmount = { 0.f };
	XMFLOAT4					vDissolveColor = { 0.f, 0.f, 0.f, 0.f };
};

struct alignas(16) StaticCommand : public RenderCommand
{

};


struct alignas(16) EffectCommand : public RenderCommand
{
    _float  fEffectTime = {};
    _float3  vColor = {0.f,0.f, 0.f};

	_uint iBlendMode = {};
	_uint bZWrite = {};
	_uint bZTest = { 1 };
	_uint iAlignment = {};
};


struct alignas(16) SkinnedCommand : public RenderCommand
{	
	ID3D11ShaderResourceView*	pBoneMatrixSRV = { nullptr };
	_uint						iBoneOffset = {};
	_uint						iBoneCount = {};
	_uint						iPadding = {};
};

struct alignas(16) SkinningInfoData
{
	_uint iBoneOffset = {};
	_uint iBoneCount = {};
	_uint iPadding0 = {};
	_uint iPadding1 = {};
};

// UI용 Command 추가
struct alignas(16) UICommand : public RenderCommand
{
	XMFLOAT4    vColor = { 1.f, 1.f, 1.f, 1.f };

	XMFLOAT4    vColorLT = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT4    vColorRT = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT4    vColorRB = { 1.f, 1.f, 1.f, 1.f };
	XMFLOAT4    vColorLB = { 1.f, 1.f, 1.f, 1.f };

	XMFLOAT4    vUVRect = { 0.f, 0.f, 1.f, 1.f };

	_float      fFillAmount = { 1.f };
	_int        iUseTexture = { 1 };
	_int        iBlendMode = { 0 };

	_float      fRevealProgress = { 1.f };
	_float      fRevealSoftness = { 0.02f };
	XMFLOAT2    vRevealCenter = { 0.5f, 0.5f };

	_int        iRevealType = { 0 };
};

// 스프라이트폰트로 그릴 Text 전용 렌더 커맨드
struct alignas(16) UITextCommand : public RenderCommand
{
	const SpriteFont* pFont = { nullptr };

	const wchar_t* pText = { nullptr };

	// DirectXTK 화면 좌표: 좌측 상단 원점, 아래쪽이 +Y
	XMFLOAT2 vPosition = {};

	// 회전과 Scale의 기준이 되는 글자 내부 원점
	XMFLOAT2 vOrigin = {};

	// 가로·세로 Text 배율
	XMFLOAT2 vScale = { 1.f, 1.f };
	XMFLOAT4 vColor = { 1.f, 1.f, 1.f, 1.f };

	_float fRotation = { 0.f };
	_float fLayerDepth = { 0.f };

	XMFLOAT2 vPadding = {};
};

inline bool RenderHeader::RenderSort(const RenderCommand* pA, const RenderCommand* pB)
{
	return pA->header.iSortKey < pB->header.iSortKey;
}

//Depth - Texture - Shader 순 정렬 . 카메라와의 거리가 멀수록 우선순위 
inline  bool RenderHeader::RenderSort_Depth(const RenderCommand* pA, const RenderCommand* pB)
{
    if (pA->header.fDepth > pB->header.fDepth)
        return true;

    if (pA->header.fDepth < pB->header.fDepth)
        return false;

    //동일한 경우 기존 정렬
    return RenderSort(pA, pB);

}

struct RenderUnit
{
	class CMesh* pMesh			= { nullptr };
	class CMaterial* pMaterial	= { nullptr };
};

NS_END