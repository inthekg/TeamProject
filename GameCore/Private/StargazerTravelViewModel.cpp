#include "StargazerTravelViewModel.h"
#include "UIStargazerTravelController.h"

void CStargazerTravelViewModel::Open_Travel(const STARGAZER_UI_OPEN_DESC& OpenDesc)
{
    if (m_bVisible)
        return;

    m_OpenDesc = OpenDesc;

    Build_VisibleChapters();

    m_iSelectedChapterSlot = 0;
    m_iSelectedLocationSlot = 0;

    // 현재 별바라기가 속한 Chapter 탭을 기본 선택한다.
    for (_uint iSlotIndex = 0; iSlotIndex < m_VisibleChapterIndices.size(); ++iSlotIndex)
    {
        for (const STARGAZER_TRAVEL_ENTRY& Entry : m_OpenDesc.TravelEntries)
        {
            if (Entry.iStargazerID != m_OpenDesc.iSourceStargazerID)
                continue;

            if (m_VisibleChapterIndices[iSlotIndex] == Entry.iChapterIndex)
                m_iSelectedChapterSlot = iSlotIndex;

            break;
        }
    }

    Rebuild_FilteredEntries();

    // 현재 상호작용한 별바라기 지역을 기본 선택한다.
    for (_uint iSlotIndex = 0; iSlotIndex < m_FilteredEntries.size(); ++iSlotIndex)
    {
        if (m_FilteredEntries[iSlotIndex].iStargazerID == m_OpenDesc.iSourceStargazerID)
        {
            m_iSelectedLocationSlot = iSlotIndex;
            break;
        }
    }

    m_bHotelPreviewSelected = false;
    m_bVisible = true;
    Make_Dirty();
}

void CStargazerTravelViewModel::Close_Travel()
{
    if (!m_bVisible)
        return;

    m_bHotelPreviewSelected = false;
    m_bVisible = false;

    m_OpenDesc = {};
    m_VisibleChapterIndices.clear();
    m_FilteredEntries.clear();

    m_iSelectedChapterSlot = 0;
    m_iSelectedLocationSlot = 0;

    Make_Dirty();
}

_bool CStargazerTravelViewModel::Select_ChapterBySlot(_uint iSlotIndex)
{
    if (iSlotIndex >= m_VisibleChapterIndices.size())
        return false;

    m_bHotelPreviewSelected = false;
    m_iSelectedChapterSlot = iSlotIndex;
    m_iSelectedLocationSlot = 0;

    Rebuild_FilteredEntries();
    Make_Dirty();

    return true;
}

_bool CStargazerTravelViewModel::Select_LocationBySlot(_uint iSlotIndex)
{
    if (iSlotIndex >= m_FilteredEntries.size())
        return false;

    m_bHotelPreviewSelected = false;
    m_iSelectedLocationSlot = iSlotIndex;
    Make_Dirty();

    return true;
}

_bool CStargazerTravelViewModel::Is_CurrentLocation(_uint iSlotIndex) const
{
    if (iSlotIndex >= m_FilteredEntries.size())
        return false;

    return m_FilteredEntries[iSlotIndex].iStargazerID == m_OpenDesc.iSourceStargazerID;
}

const _wstring& CStargazerTravelViewModel::Get_LocationText(_uint iSlotIndex) const
{
    static const _wstring strEmpty = {};

    if (iSlotIndex >= m_FilteredEntries.size())
        return strEmpty;

    return m_FilteredEntries[iSlotIndex].strLocationName;
}

const string& CStargazerTravelViewModel::Get_PreviewTexturePath() const
{
    static const string strEmpty = {};

    if (m_bHotelPreviewSelected)
        return m_OpenDesc.strHotelPreviewTexturePath;

    const STARGAZER_TRAVEL_ENTRY* pSelectedEntry = Get_SelectedEntry();
    return nullptr != pSelectedEntry ? pSelectedEntry->strPreviewTexturePath : strEmpty;
}

const STARGAZER_TRAVEL_ENTRY* CStargazerTravelViewModel::Get_SelectedEntry() const
{
    if (m_iSelectedLocationSlot >= m_FilteredEntries.size())
        return nullptr;

    return &m_FilteredEntries[m_iSelectedLocationSlot];
}

void CStargazerTravelViewModel::Select_HotelShortcut()
{
    if (!m_OpenDesc.bHotelShortcutAvailable)
        return;

    m_bHotelPreviewSelected = true;
    Make_Dirty();
}

void CStargazerTravelViewModel::Build_VisibleChapters()
{
    m_VisibleChapterIndices.clear();

    for (const STARGAZER_TRAVEL_ENTRY& Entry : m_OpenDesc.TravelEntries)
    {
        const auto Iter = find(m_VisibleChapterIndices.begin(), m_VisibleChapterIndices.end(), Entry.iChapterIndex);

        if (Iter == m_VisibleChapterIndices.end())
            m_VisibleChapterIndices.push_back(Entry.iChapterIndex);
    }
}

void CStargazerTravelViewModel::Rebuild_FilteredEntries()
{
    m_FilteredEntries.clear();

    if (m_iSelectedChapterSlot >= m_VisibleChapterIndices.size())
        return;

    const _uint iSelectedChapterIndex = m_VisibleChapterIndices[m_iSelectedChapterSlot];

    for (const STARGAZER_TRAVEL_ENTRY& Entry : m_OpenDesc.TravelEntries)
    {
        if (Entry.iChapterIndex == iSelectedChapterIndex)
            m_FilteredEntries.push_back(Entry);
    }
}

CStargazerTravelViewModel* CStargazerTravelViewModel::Create()
{
    return new CStargazerTravelViewModel;
}

void CStargazerTravelViewModel::Free()
{
    m_VisibleChapterIndices.clear();
    m_FilteredEntries.clear();
    m_OpenDesc = {};

    __super::Free();
}