#include "LoadingViewModel.h"

void CLoadingViewModel::Update_Animation(_float fTimeDelta)
{
    m_fGearElapsed += fTimeDelta;
    m_fDotsElapsed += fTimeDelta;

    if (m_fGearElapsed >= 0.1f)
    {
        const _uint iAdvanceCount = static_cast<_uint>(m_fGearElapsed / 0.1f);

        m_fGearElapsed -= static_cast<_float>(iAdvanceCount) * 0.1f;
        m_iGearFrameIndex = (m_iGearFrameIndex + iAdvanceCount) % 8;

        Make_Dirty(); // 4×2 스프라이트 시트에서 사용할 프레임이 변경됐다.
    }

    if (m_fDotsElapsed >= 0.4f)
    {
        const _uint iAdvanceCount = static_cast<_uint>(m_fDotsElapsed / 0.4f);

        m_fDotsElapsed -= static_cast<_float>(iAdvanceCount) * 0.4f;
        m_iDotCount = ((m_iDotCount - 1 + iAdvanceCount) % 3) + 1;
        m_strDotsText.assign(m_iDotCount, L'.');

        Make_Dirty(); // '.', '..', '...' 순서로 로딩 점을 갱신한다.
    }
}

void CLoadingViewModel::Set_Progress(_float fProgress)
{
    const _float fClampedProgress = Clamp01(fProgress);

    if (fabsf(m_fProgress - fClampedProgress) <= 0.001f)
        return;

    m_fProgress = fClampedProgress;

    const _uint iPercent = static_cast<_uint>(roundf(m_fProgress * 100.f));

    m_strPercentText = to_wstring(iPercent) + L"%";

    Make_Dirty(); // 코 길이와 진행률 문자를 함께 갱신한다.
}

void CLoadingViewModel::Set_StateText(const _wstring& strStateText)
{
    if (m_strStateText == strStateText)
        return;

    m_strStateText = strStateText;
    Make_Dirty();
}

void CLoadingViewModel::Set_TipText(const _wstring& strTipText)
{
    if (m_strTipText == strTipText)
        return;

    m_strTipText = strTipText;
    Make_Dirty();
}

void CLoadingViewModel::Set_PhotoTexturePath(const string& strTexturePath)
{
    if (strTexturePath.empty() || m_strPhotoTexturePath == strTexturePath)
        return;

    m_strPhotoTexturePath = strTexturePath;
    Make_Dirty();
}

CLoadingViewModel* CLoadingViewModel::Create()
{
    return new CLoadingViewModel;
}

void CLoadingViewModel::Free()
{
    __super::Free();
}