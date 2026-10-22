#include "Target_Manager.h"
#include "RenderTarget.h"
#include "GameInstance.h"

#ifdef _DEBUG
#include "ConstantBuffer.h"
#endif // DEBUG


CTarget_Manager::CTarget_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CTarget_Manager::Add_RenderTarget(const _wstring& strTargetTag, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
    if (nullptr != Find_RenderTarget(strTargetTag))
        return E_FAIL;

    CRenderTarget* pRenderTarget = CRenderTarget::Create(m_pDevice, m_pContext, iWidth, iHeight, ePixelFormat, vClearColor);
    if (nullptr == pRenderTarget)
        return E_FAIL;

#ifdef _DEBUG
    pRenderTarget->Set_Name(strTargetTag);
#endif // _DEBUG    

    m_RenderTargets.emplace(strTargetTag, pRenderTarget);

    return S_OK;
}

void CTarget_Manager::Clear_RenderTarget(const _wstring& strTargetTag)
{
    auto pRenderTarget = Find_RenderTarget(strTargetTag);
    if (pRenderTarget) pRenderTarget->Clear();
}

ID3D11ShaderResourceView* CTarget_Manager::Get_RenderTargetSRV(const _wstring strTargetTag)
{
    CRenderTarget* pRenderTarget = Find_RenderTarget(strTargetTag);

    if (pRenderTarget != nullptr) return pRenderTarget->Get_SRV();
    else return nullptr;
}

HRESULT CTarget_Manager::Add_MRT(const _wstring& strMRTTag, const _wstring& strTargetTag)
{
    auto		pRenderTarget = Find_RenderTarget(strTargetTag);
    if (nullptr == pRenderTarget)
        return E_FAIL;

    auto		pMRTList = Find_MRT(strMRTTag);

    if (nullptr == pMRTList)
    {
        list<CRenderTarget*>		MRTList;

        MRTList.push_back(pRenderTarget);

        m_MRTs.emplace(strMRTTag, MRTList);
    }
    else
        pMRTList->push_back(pRenderTarget);

    Safe_AddRef(pRenderTarget);

    return S_OK;
}


HRESULT CTarget_Manager::Begin_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV, _bool bUseDSV, _bool bClear)
{
    auto		pMRTList = Find_MRT(strMRTTag);
    if (nullptr == pMRTList)
        return E_FAIL;

    m_pContext->OMGetRenderTargets(1, &m_pBackBufferRTV, &m_pOriginalDSV);

    ID3D11RenderTargetView* pRenderTargets[8] = { nullptr };

    _uint		iNumRenderTargets = {};

    for (auto& pRenderTarget : *pMRTList)
    {
        if (bClear)
            pRenderTarget->Clear();
        pRenderTargets[iNumRenderTargets++] = pRenderTarget->Get_RTV();
        if(iNumRenderTargets >= 8) break;
    }

    ID3D11DepthStencilView* pBindDSV = nullptr;
    if (bUseDSV)
    {
        pBindDSV = (nullptr == pDSV) ? m_pOriginalDSV : pDSV;
        if (nullptr != pDSV)
            m_pContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
    }    

    //m_pContext->OMSetRenderTargets(iNumRenderTargets, pRenderTargets, ((nullptr == pDSV) ? m_pOriginalDSV : pDSV));
    m_pContext->OMSetRenderTargets(iNumRenderTargets, pRenderTargets, pBindDSV);

    return S_OK;
}

HRESULT CTarget_Manager::Bind_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV)
{
    auto		pMRTList = Find_MRT(strMRTTag);
    if (nullptr == pMRTList)
        return E_FAIL;

    m_pContext->OMGetRenderTargets(1, &m_pBackBufferRTV, &m_pOriginalDSV);

    ID3D11RenderTargetView* pRenderTargets[8] = { nullptr };

    _uint		iNumRenderTargets = {};

    for (auto& pRenderTarget : *pMRTList)
    {
        pRenderTargets[iNumRenderTargets++] = pRenderTarget->Get_RTV();
        if (iNumRenderTargets >= 8) break;
    }

    m_pContext->OMSetRenderTargets(iNumRenderTargets, pRenderTargets, ((nullptr == pDSV) ? m_pOriginalDSV : pDSV));

    return S_OK;
}

HRESULT CTarget_Manager::End_MRT()
{
    ID3D11RenderTargetView* pRenderTargets[8] = {
        m_pBackBufferRTV,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    };

    m_pContext->OMSetRenderTargets(8, pRenderTargets, m_pOriginalDSV);

    Safe_Release(m_pBackBufferRTV);
    Safe_Release(m_pOriginalDSV);

    return S_OK;
}

HRESULT CTarget_Manager::Bind_ShaderResource(const _wstring& strTargetTag, _uint iRegisterSlot)
{
    auto		pRenderTarget = Find_RenderTarget(strTargetTag);
    if (nullptr == pRenderTarget)
    {
        // CS_추가
        MSG_BOX("Bind ShaderResource Failed : RenderTarget Not Found");
        return E_FAIL;
    }
    return pRenderTarget->Bind_ShaderResource(iRegisterSlot);
}

HRESULT CTarget_Manager::Copy_Resource(const _wstring& strTargetTag, ID3D11Texture2D* pOut)
{
    auto		pRenderTarget = Find_RenderTarget(strTargetTag);
    if (nullptr == pRenderTarget)
        return E_FAIL;

    return pRenderTarget->Copy_Resource(pOut);
}


HRESULT CTarget_Manager::Clear_RTVBind(CRenderTarget* pRenderTarget)
{
    if (!m_pContext || !pRenderTarget) return E_FAIL;
    if (pRenderTarget->Get_RTV() == nullptr) return S_OK;

    ID3D11RenderTargetView* pRTV = pRenderTarget->Get_RTV();

    ID3D11RenderTargetView* boundRTVs[8] = { nullptr, };
    ID3D11DepthStencilView* pboundDSV = { nullptr };

    m_pContext->OMGetRenderTargets(8, boundRTVs, &pboundDSV);

    _bool bChanged = { false };
    for (int i = 0; i < 8; ++i)
    {
        //바인딩 되어있으면 
        if (boundRTVs[i] == pRTV)
        {
            Safe_Release(boundRTVs[i]);
            bChanged = true;

        }
        //Get으로 인해 증가한 RefCount를 다시 감소시킨다. null초기화 X 
        else if(boundRTVs[i] != nullptr) boundRTVs[i]->Release();
    }

    //변경됐으면 변경된 배열로 다시 바인딩 
    if (bChanged)
    {
        m_pContext->OMSetRenderTargets(8, boundRTVs, pboundDSV);
    }

    if (pboundDSV != nullptr) pboundDSV->Release();

    return S_OK;
}

HRESULT CTarget_Manager::Clear_SRVBind(CRenderTarget* pRenderTarget)
{
    if (!m_pContext || !pRenderTarget) return E_FAIL;
    if (pRenderTarget->Get_SRV() == nullptr) return S_OK;

    ID3D11ShaderResourceView* pSRV = pRenderTarget->Get_SRV();
    //주로 CRenderTarget클래스는 Albedo 슬롯에 꽂으니, Albedo 영역에만 있다고 가정 
    ID3D11ShaderResourceView* boundSRVs[g_iMaxTextureStack] = { nullptr, };
    m_pContext->PSGetShaderResources(0, g_iMaxTextureStack, boundSRVs);
    _bool isChanged = false;

    for (UINT i = 0; i < g_iMaxTextureStack; ++i)
    {
        if (boundSRVs[i] == pSRV)
        {
            Safe_Release(boundSRVs[i]);
            isChanged = true;
        }
        else if (boundSRVs[i] != nullptr) boundSRVs[i]->Release();
    }

    if (isChanged)
    {
        m_pContext->PSSetShaderResources(0, g_iMaxTextureStack, boundSRVs);
    }
    return S_OK;
}

HRESULT CTarget_Manager::PopRenderTarget_InMRT(const _wstring& strMRTTag, const _wstring& strTargetTag)
{
    CRenderTarget* pRenderTarget =  Find_RenderTarget(strTargetTag);
    if (pRenderTarget == nullptr) return E_FAIL;

    auto iterMRT = m_MRTs.find(strMRTTag);
    if (iterMRT == m_MRTs.end()) return E_FAIL;
    
    auto& RTVList = iterMRT->second;

    auto iterRTVList = find(RTVList.begin(), RTVList.end(), pRenderTarget);
    if (iterRTVList == RTVList.end()) return E_FAIL;

    RTVList.erase(iterRTVList);
    //Add_MRT에서 AddRef했기때문에 감소시킴 
    Safe_Release(pRenderTarget);

    return S_OK;
}

HRESULT CTarget_Manager::Remove_RenderTarget(const _wstring& strTargetTag)
{
    CRenderTarget* pRenderTarget = Find_RenderTarget(strTargetTag);
    if (pRenderTarget == nullptr)
    {
        MSG_BOX("CTargetManager : Try Remove Empty RenderTarget");
        return E_FAIL;
    }

    //RTV ,SRV 바인딩 해제 
    if (FAILED(Clear_RTVBind(pRenderTarget))) return E_FAIL;
    if (FAILED(Clear_SRVBind(pRenderTarget))) return E_FAIL;


    auto iter = m_RenderTargets.find(strTargetTag);
    if (iter != m_RenderTargets.end()) m_RenderTargets.erase(iter);

    Safe_Release(pRenderTarget);

    return S_OK;
}


CRenderTarget* CTarget_Manager::Find_RenderTarget(const _wstring& strTargetTag)
{
    auto	iter = m_RenderTargets.find(strTargetTag);
    if (iter == m_RenderTargets.end())
        return nullptr;

    return iter->second;
}
list<class CRenderTarget*>* CTarget_Manager::Find_MRT(const _wstring& strMRTTag)
{
    auto	iter = m_MRTs.find(strMRTTag);
    if (iter == m_MRTs.end())
        return nullptr;

    return &iter->second;
}

#ifdef _DEBUG

HRESULT CTarget_Manager::Ready_Debug(const _wstring& strTargetTag, _float fX, _float fY, _float fSizeX, _float fSizeY)
{
    auto		pRenderTarget = Find_RenderTarget(strTargetTag);
    if (nullptr == pRenderTarget)
        return E_FAIL;

    return pRenderTarget->Ready_Debug(fX, fY, fSizeX, fSizeY);
}

HRESULT CTarget_Manager::Render_Debug(const _wstring& strMRTTag, CShader* pShader, CVIBuffer_Rect* pVIBuffer, CConstantBuffer* pObjectBuffer)
{
    auto		pMRTList = Find_MRT(strMRTTag);
    if (nullptr == pMRTList)
        return E_FAIL;

    if (pObjectBuffer == nullptr) return E_FAIL;

    for (auto& pRenderTarget : *pMRTList)
    {
        PerObjectData objectData{};
        _float4x4 WorldMatrix = pRenderTarget->Get_WorldMatrix();
        XMStoreFloat4x4(&objectData.WorldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&WorldMatrix)));

        pObjectBuffer->Update_Data(&objectData, sizeof(PerObjectData));

        

        pRenderTarget->Render_Debug(pShader, pVIBuffer);
    }

    return S_OK;
}

#endif


CTarget_Manager* CTarget_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    return new CTarget_Manager(pDevice, pContext);
}

void CTarget_Manager::Free()
{
    __super::Free();

    for (auto& Pair : m_MRTs)
    {
        for (auto& pRenderTarget : Pair.second)
            Safe_Release(pRenderTarget);
        Pair.second.clear();
    }

    for (auto& Pair : m_RenderTargets)
        Safe_Release(Pair.second);

    m_RenderTargets.clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
