#include "Effect_Trail.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "VIBuffer_Trail.h"
#include "Effect_Emitter.h"
#include "Material.h"
#include "Texture.h"

CEffect_Trail::CEffect_Trail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject { pDevice, pContext }
{

}

HRESULT CEffect_Trail::Initialize(void* pArg)
{
	m_pBuffer = CVIBuffer_Trail::Create(m_pDevice, m_pContext);
	if (m_pBuffer == nullptr)
		return E_FAIL;

	m_Points.reserve(CVIBuffer_Trail::MAX_INPUT_POINTS);
	return S_OK;
}

void CEffect_Trail::Set_Params(const TrailModuleDesc& tTrail)
{
	m_fWidth = tTrail.fWidth;
	m_fMaxLength = (tTrail.fMaxLength > 0.01f) ? tTrail.fMaxLength : 0.01f;
	m_fTailWidthScale = tTrail.fTailWidthScale;
	m_fTrailLife = (tTrail.fTrailLife > 0.f) ? tTrail.fTrailLife : 0.f;
	m_fPointInterval = (tTrail.fPointInterval > 0.001f) ? tTrail.fPointInterval : 0.001f;
	m_bFadeOut = tTrail.bFadeOut;
	m_bSpline = tTrail.bSpline;
	m_fSinSpeed = tTrail.fSinSpeed;
	m_fWaveAmplitude = tTrail.fWaveAmplitude;
	m_iTexMode = tTrail.iTexMode;

	m_vBaseTile = _float2(tTrail.fBaseTileU, tTrail.fBaseTileV);
	m_vBaseSpeed = _float2(tTrail.fBaseSpeedX, tTrail.fBaseSpeedY);
	m_vNoiseTile = _float2(tTrail.fNoiseTileU, tTrail.fNoiseTileV);
	m_vNoiseSpeed = _float2(tTrail.fNoiseSpeedX, tTrail.fNoiseSpeedY);
	m_fBasePower = tTrail.fBasePower;
	m_fBaseDensity = tTrail.fBaseDensity;
	m_fNoisePower = tTrail.fNoisePower;
	m_fNoiseDensity = tTrail.fNoiseDensity;
	m_fEmiIntensity = tTrail.fEmiIntensity;
	m_bUseAlphaY = tTrail.bUseAlphaY;
	m_fAlphaYPower = tTrail.fAlphaYPower;
	m_fAlphaYPower2 = tTrail.fAlphaYPower2;
	m_fAlphaYPivot = tTrail.fAlphaYPivot;
	m_iAlphaYMode = tTrail.iAlphaYMode;
	m_iBlendMode = tTrail.iBlendMode;
	m_bUseDistortion = tTrail.bUseDistortion;
	m_fDistortion = tTrail.fDistortion;
}

void CEffect_Trail::Set_Anchor2(const _float3& vTop, const _float3& vRot)
{
	m_vAnchor = _float3((vTop.x + vRot.x) * 0.5f, (vTop.y + vRot.y) * 0.5f, (vTop.z + vRot.z) * 0.5f);
	m_vAnchorHalf = _float3((vTop.x - vRot.x) * 0.5f, (vTop.y - vRot.y) * 0.5f, (vTop.z - vRot.z) * 0.5f);
	m_bHasAnchor = true;
	m_bDualSocket = true;
}

void CEffect_Trail::Set_Anchor2_Arc(const vector<_float3>& Centers, const vector<_float3>& Halves)
{
	if (Centers.empty() || Centers.size() != Halves.size())
		return;

	m_FrameCenters = Centers;
	m_FrameHalves = Halves;

	m_vAnchor = Centers.back();
	m_vAnchorHalf = Halves.back();
	m_bHasAnchor = true;
	m_bDualSocket = true;
}

void CEffect_Trail::Set_ColorCurve(const _float4* pColors, _bool bEnabled)
{
	m_bColorCurve = bEnabled && (pColors != nullptr);
	if (m_bColorCurve)
	{
		for (_uint i = 0; i < LUT_SIZE; ++i)
			m_ColorLUT[i] = pColors[i];
	}
}

void CEffect_Trail::Set_Texture(const string& strPath)
{
	if (strPath == m_strTexturePath)
		return;

	m_strTexturePath = strPath;

	if (m_pBuffer)
		m_pBuffer->Set_DiffuseSRV(nullptr);
	Safe_Release(m_pMaterial);

	if (strPath.empty())
		return;

	m_pMaterial = CMaterial::Create(m_pDevice, m_pContext);
	if (m_pMaterial == nullptr)
		return;

	int iLen = MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, &strWPath[0], iLen);

	wstring strName = L"TrailTex";
	CTexture* pTexture = CTexture::Create(m_pDevice, m_pContext, &strName, &strWPath);
	if (pTexture)
	{
		m_pMaterial->Input_Texture(TEXTURE_TYPE::BASE_COLOR, pTexture);
		Safe_Release(pTexture);
	}

	if (m_pBuffer)
		m_pBuffer->Set_DiffuseSRV(m_pMaterial->Get_SRV(TEXTURE_TYPE::BASE_COLOR, 0));
}

void CEffect_Trail::Set_NoiseTexture(const string& strPath)
{
	if (strPath == m_strNoiseTexPath)
		return;
	m_strNoiseTexPath = strPath;

	if (m_pBuffer)
		m_pBuffer->Set_NoiseSRV(nullptr);
	Safe_Release(m_pNoiseMaterial);

	if (strPath.empty())
		return;

	m_pNoiseMaterial = CMaterial::Create(m_pDevice, m_pContext);
	if (m_pNoiseMaterial == nullptr)
		return;

	int iLen = MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, &strWPath[0], iLen);

	wstring strName = L"TrailNoiseTex";
	CTexture* pTexture = CTexture::Create(m_pDevice, m_pContext, &strName, &strWPath);
	if (pTexture)
	{
		m_pNoiseMaterial->Input_Texture(TEXTURE_TYPE::BASE_COLOR, pTexture);
		Safe_Release(pTexture);
	}

	if (m_pBuffer)
		m_pBuffer->Set_NoiseSRV(m_pNoiseMaterial->Get_SRV(TEXTURE_TYPE::BASE_COLOR, 0));
}

void CEffect_Trail::Set_SceneSRV(ID3D11ShaderResourceView* pSRV)
{
	if (m_pBuffer)
		m_pBuffer->Set_SceneSRV(pSRV);
}

void CEffect_Trail::Activate(void* pArg)
{
	Reset();
	__super::Activate(pArg);
}

void CEffect_Trail::InActivate()
{
	Reset();
	__super::InActivate();
}

void CEffect_Trail::Reset()
{
	m_Points.clear();
	m_Half.clear();
	m_PointAge.clear();
	m_FrameCenters.clear();
	m_FrameHalves.clear();
	m_bHasAnchor = false;
	m_fTime = 0.f;
	m_bFading = false;
	m_bDualSocket = false;
	m_bColorCurve = false;
}

void CEffect_Trail::Trim_ToMaxLength()
{
	auto SegLen = [&](size_t i) -> _float
		{
			const _float3& a = m_Points[i];
			const _float3& b = m_Points[i + 1];
			const _float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
			return sqrtf(dx * dx + dy * dy + dz * dz);
		};

	if (m_Points.size() < 2)
		return;

	_float fTotal = 0.f;

	for (size_t i = 0; i + 1 < m_Points.size(); ++i)
		fTotal += SegLen(i);
	
	while (fTotal > m_fMaxLength && m_Points.size() > 2)
	{
		fTotal -= SegLen(0);
		m_Points.erase(m_Points.begin());
		if (!m_Half.empty())
			m_Half.erase(m_Half.begin());
		if (!m_PointAge.empty())
			m_PointAge.erase(m_PointAge.begin());
	}

	while (m_Points.size() > CVIBuffer_Trail::MAX_INPUT_POINTS)
	{
		m_Points.erase(m_Points.begin());
		if (!m_Half.empty())
			m_Half.erase(m_Half.begin());
		if (!m_PointAge.empty())
			m_PointAge.erase(m_PointAge.begin());
	}
}

void CEffect_Trail::Update(_float fTimeDelta)
{
	if (!m_bHasAnchor || m_pBuffer == nullptr)
		return;

	m_fTime += fTimeDelta;

	for (auto& fAge : m_PointAge)
		fAge += fTimeDelta;
	
	if (!m_bFading)
	{
		if (m_Points.empty())
		{
			m_Points.push_back(m_vAnchor);
			m_Points.push_back(m_vAnchor);
			m_Half.push_back(m_vAnchorHalf);
			m_Half.push_back(m_vAnchorHalf);
			m_PointAge.push_back(0.f);
			m_PointAge.push_back(0.f);
		}
		else if (!m_FrameCenters.empty())
		{
			for (size_t k = 0; k < m_FrameCenters.size(); ++k)
			{
				const _float3& c = m_FrameCenters[k];
				const _float3& h = m_FrameHalves[k];
				const _float3& last = m_Points.back();
				const _float dx = c.x - last.x;
				const _float dy = c.y - last.y;
				const _float dz = c.z - last.z;
				const _bool bLast = (k + 1 == m_FrameCenters.size());
				if (sqrtf(dx * dx + dy * dy + dz * dz) >= m_fPointInterval || bLast)
				{
					m_Points.push_back(c);
					m_Half.push_back(h);
					m_PointAge.push_back(0.f);
				}
			}
		}
		else
		{
			m_Points.back() = m_vAnchor;
			m_Half.back() = m_vAnchorHalf;
			m_PointAge.back() = 0.f;

			const _float3& prev = m_Points[m_Points.size() - 2];
			const _float dx = m_vAnchor.x - prev.x;
			const _float dy = m_vAnchor.y - prev.y;
			const _float dz = m_vAnchor.z - prev.z;
			if (sqrtf(dx * dx + dy * dy + dz * dz) >= m_fPointInterval)
			{
				m_Points.push_back(m_vAnchor);
				m_Half.push_back(m_vAnchorHalf);
				m_PointAge.push_back(0.f);
			}
		}
	}

	m_FrameCenters.clear();
	m_FrameHalves.clear();

	if (m_fTrailLife > 0.f)
	{
		const size_t iMinPoints = m_bFading ? 0 : 2;
		while (m_Points.size() > iMinPoints && !m_PointAge.empty() && m_PointAge.front() > m_fTrailLife)
		{
			m_Points.erase(m_Points.begin());
			if (!m_Half.empty())
				m_Half.erase(m_Half.begin());
			m_PointAge.erase(m_PointAge.begin());
		}
	}
	else if (m_bFading)
	{
		m_Points.clear();
		m_Half.clear();
		m_PointAge.clear();
	}

	Trim_ToMaxLength();

	const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);
	const _float4* pCamPos = m_pGameInstance->Get_CamPosition();

	if (pView == nullptr || pProj == nullptr || pCamPos == nullptr)
		return;

	TRAIL_DRAW_DESC desc{};
	desc.vColor = m_vColor;
	desc.fWidth = m_fWidth;
	desc.fTailWidthScale = m_fTailWidthScale;
	desc.vCamPos = _float3(pCamPos->x, pCamPos->y, pCamPos->z);
	desc.bFadeOut = m_bFadeOut;
	desc.bSpline = m_bSpline;
	desc.fSinSpeed = m_fSinSpeed;
	desc.fWaveAmplitude = m_fWaveAmplitude;
	desc.iTexMode = m_iTexMode;
	desc.fTime = m_fTime;
	desc.pColorLUT = m_bColorCurve ? m_ColorLUT : nullptr;
	desc.iColorLUTSize = m_bColorCurve ? LUT_SIZE : 0;
	desc.bColorCurve = m_bColorCurve;
	desc.bDualSocket = m_bDualSocket;
	desc.pHalf = m_bDualSocket ? &m_Half : nullptr;
	desc.vBaseTile = m_vBaseTile;
	desc.vBaseSpeed = m_vBaseSpeed;
	desc.vNoiseTile = m_vNoiseTile;
	desc.vNoiseSpeed = m_vNoiseSpeed;
	desc.fBasePower = m_fBasePower;
	desc.fBaseDensity = m_fBaseDensity;
	desc.fNoisePower = m_fNoisePower;
	desc.fNoiseDensity = m_fNoiseDensity;
	desc.fEmiIntensity = m_fEmiIntensity;
	desc.bUseAlphaY = m_bUseAlphaY;
	desc.fAlphaYPower = m_fAlphaYPower;
	desc.fAlphaYPower2 = m_fAlphaYPower2;
	desc.fAlphaYPivot = m_fAlphaYPivot;
	desc.iAlphaYMode = m_iAlphaYMode;
	desc.bUseDistortion = m_bUseDistortion;
	desc.fDistortion = m_fDistortion;
	const _float2 vVp = m_pGameInstance->Get_ViewportDesc();
	desc.vScreen = _float2(vVp.x > 1.f ? vVp.x : 1.f, vVp.y > 1.f ? vVp.y : 1.f);
	desc.pView = pView;
	desc.pProj = pProj;

	m_pBuffer->Update_Ribbon(m_Points, desc);
}

HRESULT CEffect_Trail::Render()
{
	if (m_pBuffer == nullptr || m_pBundle == nullptr)
		return S_OK;
	if (m_pBundle->pTrailVS == nullptr || m_pBundle->pTrailPS == nullptr)
		return S_OK;
	if (m_Points.size() < 2)
		return S_OK;

	EffectCommand cmd{};
	cmd.header.etype = RENDER_TYPE::EFFECT;
	cmd.header.iCommandSize = sizeof(EffectCommand);
	cmd.header.iSortKey = 0;
	cmd.header.fDepth = 0.f;

	const _bool bWBOIT = (m_iBlendMode == 0) && (m_pBundle->pTrailPS_WBOIT != nullptr);

	cmd.pVIBuffer = m_pBuffer;
	cmd.pVS = m_pBundle->pTrailVS;
	cmd.pPS = m_pBundle->pTrailPS;
	cmd.pLayout = m_pBundle->pTrailLayout;
	cmd.pMaterial = nullptr;
	cmd.iBlendMode = static_cast<_uint>(m_iBlendMode);
	cmd.bZWrite = 0;
	cmd.bZTest = 1;
	cmd.iAlignment = 2;
	XMStoreFloat4x4(&cmd.worldMatrix, XMMatrixIdentity());

	m_pGameInstance->Submit_Effect(cmd);

	return S_OK;
}

CEffect_Trail* CEffect_Trail::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEffect_Trail* pInstance = new CEffect_Trail(pDevice, pContext);
	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Created : CEffect_Trail");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CEffect_Trail::Free()
{
	Safe_Release(m_pMaterial);
	Safe_Release(m_pNoiseMaterial);
	Safe_Release(m_pBuffer);
	__super::Free();
}
