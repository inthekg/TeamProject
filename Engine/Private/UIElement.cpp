#include "UIElement.h"
#include "UI_Function.h"
#include "GameInstance.h"
#include "Shader.h"
#include "VIBuffer.h"
#include "Material.h"


CUIElement::CUIElement(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance { CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

void CUIElement::Set_Position(const _float2& vPosition)
{
    m_tTransform.vPosition = vPosition;
}

void CUIElement::Set_Size(const _float2& vSize)
{
    m_tTransform.vSize = vSize;
}

void CUIElement::Set_Pivot(const _float2& vPivot)
{
    m_tTransform.vPivot = vPivot;
}

void CUIElement::Set_Anchor(UI_ANCHOR eAnchor)
{
    m_tTransform.eAnchor = eAnchor;
}

void CUIElement::Set_Rotation(_float fRotation)
{
    m_tTransform.fRotation = fRotation;
}

void CUIElement::Set_Scale(const _float2& vScale)
{
    m_tTransform.vScale = vScale;
}

void CUIElement::Set_Color(const _float4& vColor)
{
    m_tRenderData.vColor = vColor;
}

void CUIElement::Set_UVRect(const _float4& vUVRect)
{
    m_tRenderData.vUVRect = vUVRect;
}

void CUIElement::Set_RevealType(UI_REVEAL_TYPE eRevealType)
{
    m_tRenderData.eRevealType = eRevealType;
}

void CUIElement::Set_RevealProgress(_float fProgress)
{	
    // Reveal 진행률은 유효 범위 안으로 제한한다.
    m_tRenderData.fRevealProgress = fClamp(fProgress, 0.f, 1.f);
}

void CUIElement::Set_RevealSoftness(_float fSoftness)
{
    // 음수 Softness가 셰이더 계산에 들어가지 않도록 막는다.
    m_tRenderData.fRevealSoftness = max(fSoftness, 0.f);
}

void CUIElement::Set_RevealCenter(const _float2& vCenter)
{
    m_tRenderData.vRevealCenter.x = fClamp(vCenter.x, 0.f, 1.f);
    m_tRenderData.vRevealCenter.y = fClamp(vCenter.y, 0.f, 1.f);
}

void CUIElement::Set_SortOrder(_uint iSortOrder)
{
    m_tRenderData.iSortOrder = iSortOrder;
}

void CUIElement::Set_Visible(_bool bVisible)
{
    m_tRenderData.bVisible = bVisible;
}

void CUIElement::Set_UseTexture(_bool bUseTexture)
{
    m_tRenderData.bUseTexture = bUseTexture;
}

void CUIElement::Set_Shader(CShader* pShader)
{
    Safe_AddRef(pShader);
    Safe_Release(m_tRenderData.pShader);
    m_tRenderData.pShader = pShader;
}

void CUIElement::Set_VIBuffer(CVIBuffer* pVIBuffer)
{
    Safe_AddRef(pVIBuffer);
    Safe_Release(m_tRenderData.pVIBuffer);
    m_tRenderData.pVIBuffer = pVIBuffer;
}

void CUIElement::Set_Material(CMaterial* pMaterial)
{
    Safe_AddRef(pMaterial);
    Safe_Release(m_tRenderData.pMaterial);
    m_tRenderData.pMaterial = pMaterial;
}

void CUIElement::Set_ElementID(_uint iElementID)
{
    m_iElementID = iElementID;
}

void CUIElement::Set_RuntimeElementTag(const _wstring& strRuntimeElementTag)
{
    m_strRuntimeElementTag = strRuntimeElementTag;
}

const UITransform& CUIElement::Get_UITransform() const
{
    return m_tTransform;
}

const UIRenderData& CUIElement::Get_RenderData() const
{
    return m_tRenderData;
}

_bool CUIElement::Is_Visible() const
{
    return m_tRenderData.bVisible;
}

CUIElement* CUIElement::Get_Parent() const
{
    return m_pParent;
}

const vector<CUIElement*>& CUIElement::Get_Children() const
{
    return m_Children;
}

_uint CUIElement::Get_ElementID() const
{
    return m_iElementID;
}

const _wstring& CUIElement::Get_RuntimeElementTag() const
{
    return m_strRuntimeElementTag;
}

_float CUIElement::Get_HierarchyAlpha() const
{
    _float fHierarchyAlpha = 1.f;

    // 자신부터 루트까지 올라가며 Alpha를 누적한다.
    for (const CUIElement* pElement = this; pElement != nullptr; pElement = pElement->m_pParent)
    {
        const _float fElementAlpha = min(1.f, max(0.f, pElement->m_tRenderData.vColor.w));
        fHierarchyAlpha *= fElementAlpha;
    }

    return min(1.f, max(0.f, fHierarchyAlpha));
}

_bool CUIElement::Is_PointInside(const _float2& vUIPoint) const
{
    // 자신이나 부모가 숨겨진 경우 입력 영역으로 사용하지 않는다.
    if (!Is_HierarchyVisible())
        return false;

    if (m_tTransform.vSize.x <= 0.f || m_tTransform.vSize.y <= 0.f)
        return false;

    _float4x4 WorldMatrix = {};
    Make_WorldMatrix(&WorldMatrix);

    _vector vDeterminant = {};
    const _matrix InverseWorld = XMMatrixInverse(&vDeterminant, XMLoadFloat4x4(&WorldMatrix));

    // Scale이 0이면 정상적인 역행렬을 만들 수 없다.
    if (fabsf(XMVectorGetX(vDeterminant)) <= FLT_EPSILON)
        return false;

    // 마우스 UI 좌표를 Element의 로컬 좌표로 변환한다.
    const _vector vLocalPoint = XMVector3TransformCoord(XMVectorSet(vUIPoint.x, vUIPoint.y, 0.f, 1.f), InverseWorld);

    const _float fLocalX = XMVectorGetX(vLocalPoint);
    const _float fLocalY = XMVectorGetY(vLocalPoint);

    // UI 사각형 버텍스의 로컬 범위는 중앙 기준 -0.5~+0.5다.
    return fLocalX >= -0.5f && fLocalX <= 0.5f && fLocalY >= -0.5f && fLocalY <= 0.5f;
}

HRESULT CUIElement::Add_Child(CUIElement* pChild)
{
    if (pChild == nullptr || pChild == this)
        return E_FAIL;

    // 이미 다른 부모에 연결된 엘리먼트는 바로 옮기지 않음.
    if (pChild->m_pParent != nullptr)
        return E_FAIL;

    for (CUIElement* pAncestor = this; pAncestor != nullptr; pAncestor = pAncestor->m_pParent)
    {
        if (pAncestor == pChild)
            return E_FAIL;
    }

    // 동일한 자식 중복 등록 방지
    const auto iter = find(m_Children.begin(), m_Children.end(), pChild);
    if (iter != m_Children.end())
        return E_FAIL;

    m_Children.push_back(pChild);
    pChild->m_pParent = this;

    return S_OK;
}

void CUIElement::Remove_Child(CUIElement* pChild)
{
    if (pChild == nullptr)
        return;

    const auto iter = find(m_Children.begin(), m_Children.end(), pChild);

    if (iter == m_Children.end())
        return;

    if (pChild->m_pParent == this)
        pChild->m_pParent = nullptr;

    m_Children.erase(iter);
}


HRESULT CUIElement::Initialize()
{
    return S_OK;
}

void CUIElement::Priority_Update(_float fTimeDelta)
{
}

void CUIElement::Update(_float fTimeDelta)
{
}

void CUIElement::Late_Update(_float fTimeDelta)
{
}

void CUIElement::Submit_RenderCommand()
{
    Submit_RenderCommand(0);
}

void CUIElement::Submit_RenderCommand(_uint iObjectSortOrder)
{
    if (Is_HierarchyVisible() == false || m_tRenderData.pShader == nullptr || m_tRenderData.pVIBuffer == nullptr)
        return;

    const _uint iFinalSortOrder = iObjectSortOrder * UI_ELEMENT_SORT_RANGE + m_tRenderData.iSortOrder;

    UICommand command = {};

    command.header.etype = RENDER_TYPE::UI;
    command.header.iCommandSize = sizeof(UICommand);
    command.header.iSortKey = iFinalSortOrder;

    Make_WorldMatrix(&command.worldMatrix);

    command.pVIBuffer = m_tRenderData.pVIBuffer;
    command.pVS = m_tRenderData.pShader->Get_VertexShader();
    command.pPS = m_tRenderData.pShader->Get_PixelShader();
    command.pLayout = m_tRenderData.pShader->Get_InputLayout();
    command.pMaterial = m_tRenderData.pMaterial;

    _float4 vFinalColor = m_tRenderData.vColor;

    // RGB는 자신의 색상을 유지하고 Alpha만 부모 계층을 반영한다.
    vFinalColor.w = Get_HierarchyAlpha();

    command.vColor = vFinalColor;
    command.vColorLT = vFinalColor;
    command.vColorRT = vFinalColor;
    command.vColorRB = vFinalColor;
    command.vColorLB = vFinalColor;

    command.vUVRect = m_tRenderData.vUVRect;
    command.fFillAmount = 1.f;
    command.iUseTexture = m_tRenderData.bUseTexture ? 1 : 0;
    command.iBlendMode = 0;

    command.fRevealProgress = m_tRenderData.fRevealProgress;
    command.fRevealSoftness = m_tRenderData.fRevealSoftness;
    command.vRevealCenter = m_tRenderData.vRevealCenter;
    command.iRevealType = static_cast<_int>(m_tRenderData.eRevealType);

    m_pGameInstance->Submit_UI(command);
}

_float2 CUIElement::Get_AnchoredPosition() const
{
    // 루트는 부모가 없으므로 포지션을 그대로 사용
    if (m_pParent == nullptr)
        return m_tTransform.vPosition;

    const UITransform& ParentTransform = m_pParent->Get_UITransform();

    const _float2 vAnchorOffset = Calc_UIAnchorOffset(m_tTransform.eAnchor, ParentTransform.vSize, ParentTransform.vPivot);

    return _float2(m_tTransform.vPosition.x + vAnchorOffset.x, m_tTransform.vPosition.y + vAnchorOffset.y);
}

_matrix CUIElement::Get_ParentWorldLayoutMatrix() const
{
    if (m_pParent == nullptr)
        return XMMatrixIdentity();

    return m_pParent->Get_WorldLayoutMatrix();
}

_matrix CUIElement::Get_WorldLayoutMatrix() const
{
    const _float2 vAnchoredPosition = Get_AnchoredPosition();

    const _matrix LocalLayoutMatrix =
        XMMatrixScaling(m_tTransform.vScale.x, m_tTransform.vScale.y, 1.f) *
        XMMatrixRotationZ(m_tTransform.fRotation) *
        XMMatrixTranslation(vAnchoredPosition.x, vAnchoredPosition.y, 0.f);

    return LocalLayoutMatrix * Get_ParentWorldLayoutMatrix();
}

_bool CUIElement::Is_HierarchyVisible() const
{
    for (const CUIElement* pElement = this; pElement != nullptr; pElement = pElement->m_pParent)
    {
        if (pElement->m_tRenderData.bVisible == false)
            return false;
    }

    return true;
}


void CUIElement::Make_WorldMatrix(_float4x4* pOutWorldMatrix) const
{
    const _float fWidth = m_tTransform.vSize.x * m_tTransform.vScale.x;
    const _float fHeight = m_tTransform.vSize.y * m_tTransform.vScale.y;

    const _float fPivotX = (m_tTransform.vPivot.x - 0.5f) * fWidth;
    const _float fPivotY = (0.5f - m_tTransform.vPivot.y) * fHeight;

    const _float2 vAnchoredPosition = Get_AnchoredPosition();

    XMStoreFloat4x4(
        pOutWorldMatrix,
        XMMatrixScaling(fWidth, fHeight, 1.f) *
        XMMatrixTranslation(-fPivotX, -fPivotY, 0.f) *
        XMMatrixRotationZ(m_tTransform.fRotation) *
        XMMatrixTranslation(vAnchoredPosition.x, vAnchoredPosition.y, 0.f) *
        Get_ParentWorldLayoutMatrix());
}

CUIElement* CUIElement::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUIElement* pInstance = new CUIElement(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CUIElement");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CUIElement::Free()
{
    __super::Free();

    Safe_Release(m_tRenderData.pMaterial);
    Safe_Release(m_tRenderData.pVIBuffer);
    Safe_Release(m_tRenderData.pShader);

    m_pParent = nullptr;
    m_Children.clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
