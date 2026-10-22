#include "BossHUDViewModel.h"

CBossHUDViewModel* CBossHUDViewModel::Create()
{
    return new CBossHUDViewModel;
}

void CBossHUDViewModel::Free()
{
    __super::Free();
}
