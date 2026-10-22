#include "MonsterHUDViewModel.h"

CMonsterHUDViewModel* CMonsterHUDViewModel::Create()
{
    return new CMonsterHUDViewModel;
}

void CMonsterHUDViewModel::Set_HealthVisible(_bool bVisible)
{
    if (m_bHealthVisible == bVisible)
        return;

    m_bHealthVisible = bVisible;
    Make_Dirty();
}

void CMonsterHUDViewModel::Free()
{
    __super::Free();
}
