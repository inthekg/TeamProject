#include "LockOnViewModel.h"

CLockOnViewModel* CLockOnViewModel::Create()
{
    return new CLockOnViewModel;
}

void CLockOnViewModel::Free()
{
    __super::Free();
}
