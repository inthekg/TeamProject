#include "UILoadingController.h"
#include "GameInstance.h"
#include "UIImage.h"
#include "UIText.h"
#include "LoadingViewModel.h"

namespace
{
    const wchar_t* g_LoadingTips[] =
    {
        {
            L"적의 공격을 정확한 순간에 가드하면 퍼펙트 가드가 발동합니다.\n적의 공격 패턴을 관찰하면 전투를 더욱 유리하게 이끌 수 있습니다.",
        },
        {
            L"별바라기를 활성화하면 해당 지역의 이동 지점으로 사용할 수 있습니다.\n새로운 지역을 탐색할 때는 별바라기를 먼저 확인해 보세요.",
        },
        {
            L"무기의 손잡이와 칼날을 조합하면 나만의 무기를 제작할 수 있습니다.\n상성에 맞는 조합을 찾아내면 전투가 한결 수월해집니다.",
        },
        {
            L"그로기 가능한 적에게 강한 공격을 적중시키면 치명적인 공격 기회가 열립니다.\n적의 체력 표시와 상태 변화를 주의 깊게 살펴보세요.",
        },
        {
            L"장비 화면에서는 현재 무기와 리전 암, 코스튬을 확인할 수 있습니다.\n획득한 장비에 따라 표시되는 항목이 달라집니다.",
        }
    };

    constexpr _uint g_iLoadingTipsCount = static_cast<_uint>(size(g_LoadingTips));
}

void CUILoadingController::Begin_LoadingSession()
{
    if (m_pViewModel == nullptr)
        return;

    _uint iTipIndex = static_cast<_uint>(Random(0, static_cast<_int>(g_iLoadingTipsCount) - 1));

    if (g_iLoadingTipsCount > 1 && iTipIndex == m_iLastTipIndex)
        iTipIndex = (iTipIndex + 1) % g_iLoadingTipsCount;

    m_iLastTipIndex = iTipIndex;

    const wchar_t* strTip = g_LoadingTips[iTipIndex];
    
    _uint iPathIndex = static_cast<_uint>(Random(0, 40));
    if (iPathIndex == m_iLastTipIndex)
        iPathIndex = (iPathIndex + 1) % 40;

    m_iLastPathIndex = iPathIndex;

    string strBasePath = "UI/Loading/Img_Loading/UIT_Img_Loading_";
    string strNum = to_string(iPathIndex);
    strNum = string(2 - strNum.length(), '0') + strNum;

    strBasePath += strNum + ".dds";

    m_pViewModel->Set_Progress(0.f);
    m_pViewModel->Set_TipText(strTip);
    m_pViewModel->Set_PhotoTexturePath(strBasePath);
}

void CUILoadingController::Set_LoadingProgress(_float fProgress)
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Set_Progress(fProgress); 
}

HRESULT CUILoadingController::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return E_FAIL;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_pViewModel = CLoadingViewModel::Create();

    if (m_pViewModel == nullptr)
        return E_FAIL;

    m_pViewModel->Set_Progress(0.f);

    return S_OK;
}

HRESULT CUILoadingController::Bind_UI()
{
    if (FAILED(__super::Bind_UI()))
        return E_FAIL;

    m_pLoadingPhoto = Find_ElementAs<CUIImage>(L"LoadingPhoto");
    m_pLoadingGear = Find_ElementAs<CUIImage>(L"LoadingGear");
    m_pLoadingNose = Find_ElementAs<CUIImage>(L"LoadingNose");

    m_pLoadingTipText = Find_ElementAs<CUIText>(L"LoadingTipText");
    m_pLoadingStateText = Find_ElementAs<CUIText>(L"LoadingStateText");
    m_pLoadingDotsText = Find_ElementAs<CUIText>(L"LoadingDotsText");
    m_pLoadingPercentText = Find_ElementAs<CUIText>(L"LoadingPercentText");

    if (m_pLoadingPhoto == nullptr || m_pLoadingGear == nullptr || m_pLoadingNose == nullptr)
        return E_FAIL;

    if (m_pLoadingTipText == nullptr || m_pLoadingStateText == nullptr || m_pLoadingDotsText == nullptr || m_pLoadingPercentText == nullptr)
        return E_FAIL;

    // 에디터에서 조절한 코의 최종 크기를 그대로 보존한다.
    m_vLoadingNoseMaxSize = m_pLoadingNose->Get_UITransform().vSize;

    return S_OK;
}

void CUILoadingController::Update_ViewModel(_float fTimeDelta)
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Update_Animation(fTimeDelta);
}

void CUILoadingController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    m_pLoadingTipText->Set_Text(m_pViewModel->Get_TipText());
    m_pLoadingStateText->Set_Text(m_pViewModel->Get_StateText());
    m_pLoadingDotsText->Set_Text(m_pViewModel->Get_DotsText());
    m_pLoadingPercentText->Set_Text(m_pViewModel->Get_PercentText());

    const _uint iFrameIndex = m_pViewModel->Get_GearFrameIndex();
    const _uint iColumn = iFrameIndex % 4;
    const _uint iRow = iFrameIndex / 4;

    const _float fLeft = static_cast<_float>(iColumn) * 0.25f;
    const _float fTop = static_cast<_float>(iRow) * 0.5f;

    // 256×128 텍스처를 4열×2행으로 나눈 현재 프레임의 UV다.
    m_pLoadingGear->Set_UVRect(_float4(fLeft, fTop, fLeft + 0.25f, fTop + 0.5f));

    const _float fProgress = m_pViewModel->Get_Progress();

    if (fProgress <= 0.f)
    {
        m_pLoadingNose->Set_Visible(false);
    }
    else
    {
        // NineSlice 오른쪽 끝 6.4픽셀이 찌그러지지 않도록 최소 폭을 보장한다.
        const _float fNoseWidth = max(6.4f, m_vLoadingNoseMaxSize.x * fProgress);

        m_pLoadingNose->Set_Visible(true);
        m_pLoadingNose->Set_Size(_float2(fNoseWidth, m_vLoadingNoseMaxSize.y));
    }

    const string& strPhotoTexturePath = m_pViewModel->Get_PhotoTexturePath();

    if (!strPhotoTexturePath.empty() && m_strAppliedPhotoTexturePath != strPhotoTexturePath)
    {
        if (SUCCEEDED(m_pGameInstance->Set_UIImageTexture(m_hUI, L"LoadingPhoto", strPhotoTexturePath)))
        {
            m_pLoadingPhoto->Set_UseTexture(true);
            m_strAppliedPhotoTexturePath = strPhotoTexturePath;
        }
    }

    m_pViewModel->Clear_Dirty();
}

CUILoadingController* CUILoadingController::Create()
{
    return new CUILoadingController;
}

void CUILoadingController::Free()
{
    Safe_Release(m_pViewModel);

    m_pLoadingPhoto = nullptr;
    m_pLoadingGear = nullptr;
    m_pLoadingNose = nullptr;

    m_pLoadingTipText = nullptr;
    m_pLoadingStateText = nullptr;
    m_pLoadingDotsText = nullptr;
    m_pLoadingPercentText = nullptr;

    __super::Free();
}