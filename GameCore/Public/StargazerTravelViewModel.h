#pragma once
#include "UIViewModelBase.h"
#include "StargazerTravelData.h"

NS_BEGIN(GameCore)

class CStargazerTravelViewModel : public CUIViewModelBase
{
private:
    CStargazerTravelViewModel() = default;
    virtual ~CStargazerTravelViewModel() = default;

public:
    void                    Open_Travel(const STARGAZER_UI_OPEN_DESC& OpenDesc);
    void                    Close_Travel();

    _bool                   Select_ChapterBySlot(_uint iSlotIndex);
    _bool                   Select_LocationBySlot(_uint iSlotIndex);

public:
    _bool                   Is_Visible() const { return m_bVisible; }

    _uint                   Get_VisibleChapterCount() const { return static_cast<_uint>(m_VisibleChapterIndices.size()); }
    _uint                   Get_VisibleLocationCount() const { return static_cast<_uint>(m_FilteredEntries.size()); }

    _uint                   Get_SelectedChapterSlot() const { return m_iSelectedChapterSlot; }
    _uint                   Get_SelectedLocationSlot() const { return m_iSelectedLocationSlot; }

    _bool                   Is_CurrentLocation(_uint iSlotIndex) const;

    const _wstring&         Get_LocationText(_uint iSlotIndex) const;
    const _wstring&         Get_PrimaryActionText() const { return m_strPrimaryActionText; }
    const string&           Get_PreviewTexturePath() const;

    const STARGAZER_TRAVEL_ENTRY* Get_SelectedEntry() const;
    _uint64                 Get_SourceStargazerID() const { return m_OpenDesc.iSourceStargazerID; }

    _bool                   Is_HotelShortcutAvailable() const { return m_OpenDesc.bHotelShortcutAvailable; }
    _uint64                 Get_HotelStargazerID() const { return m_OpenDesc.iHotelStargazerID; }

    void                    Select_HotelShortcut();

    _bool                   Is_HotelShortcutSelected() const { return m_bHotelPreviewSelected; }

private:
    void                    Build_VisibleChapters();
    void                    Rebuild_FilteredEntries();

private:
    _bool                   m_bVisible = { false };

    STARGAZER_UI_OPEN_DESC  m_OpenDesc = {};
    vector<_uint>           m_VisibleChapterIndices = {};
    vector<STARGAZER_TRAVEL_ENTRY> m_FilteredEntries = {};

    _uint                   m_iSelectedChapterSlot = { 0 };
    _uint                   m_iSelectedLocationSlot = { 0 };

    _wstring                m_strPrimaryActionText = { L"이동" };

    _bool                   m_bHotelPreviewSelected = { false }; // 현재 우측 프리뷰가 호텔인지 표시

public:
    static CStargazerTravelViewModel* Create();
    virtual void Free() override;

};

NS_END