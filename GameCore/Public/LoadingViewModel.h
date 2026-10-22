#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

class CLoadingViewModel final : public CUIViewModelBase
{
private:
    CLoadingViewModel() = default;
    virtual ~CLoadingViewModel() = default;

public:
    void                Update_Animation(_float fTimeDelta);

public:
    void                Set_Progress(_float fProgress);
    void                Set_StateText(const _wstring& strStateText);
    void                Set_TipText(const _wstring& strTipText);
    void                Set_PhotoTexturePath(const string& strTexturePath);

public:
    _float              Get_Progress() const { return m_fProgress; }
    _uint               Get_GearFrameIndex() const { return m_iGearFrameIndex; }

    const _wstring&     Get_StateText() const { return m_strStateText; }
    const _wstring&     Get_DotsText() const { return m_strDotsText; }
    const _wstring&     Get_PercentText() const { return m_strPercentText; }
    const _wstring&     Get_TipText() const { return m_strTipText; }
    const string&       Get_PhotoTexturePath() const { return m_strPhotoTexturePath; }

private:
    _float              m_fProgress = { 0.f };

    _float              m_fGearElapsed = { 0.f };
    _float              m_fDotsElapsed = { 0.f };

    _uint               m_iGearFrameIndex = { 0 };
    _uint               m_iDotCount = { 1 };

private:
    _wstring            m_strStateText = { L"거짓말하는 중" };
    _wstring            m_strDotsText = { L"." };
    _wstring            m_strPercentText = { L"0%" };

    _wstring            m_strTipText =
    {
        L"크라트 호텔의 경영자인 안토니아는 사교계의 여왕이자 여걸로 이름높은 미인입니다.\n"
        L"병이 그녀를 갉아먹고 있지만, 그녀는 여전히 우아한 기품을 유지하고 있습니다."
    };

    string              m_strPhotoTexturePath = { "UI/Loading/Img_Loading/UIT_Img_Loading_01.dds" };

public:
    static CLoadingViewModel* Create();
    virtual void Free() override;
};

NS_END