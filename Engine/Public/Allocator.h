#pragma once
#include "Base.h"

NS_BEGIN(Engine)
class CAllocator final : public CBase
{
private:
    CAllocator(_uint capacity);
    ~CAllocator() = default;

public:
    void* Allocate(_uint size);
    void Clear() { m_iOffset = 0; }
    uint8_t* GetStart() { return m_pBuffer; }
    _uint GetOffset() { return m_iOffset; }

private:
    uint8_t* m_pBuffer = { nullptr };
    _uint m_iCapacity = {};
    _uint m_iOffset = {};

public:
    static CAllocator* Create(_uint capacity);
    virtual void Free() override;

};
NS_END