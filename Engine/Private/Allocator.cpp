#include "Allocator.h"

CAllocator::CAllocator(_uint capacity)
	: m_iCapacity(capacity)
{
	m_pBuffer = new uint8_t[m_iCapacity];
}

void* CAllocator::Allocate(_uint size)
{
	//용량없음
	if (m_iOffset + size > m_iCapacity) return nullptr; 
	void* AllocatedPtr = m_pBuffer + m_iOffset;
	m_iOffset += size;
	return AllocatedPtr;
}

CAllocator* CAllocator::Create(_uint capacity)
{
	return new CAllocator(capacity);
}

void CAllocator::Free()
{
	Safe_Delete_Array(m_pBuffer);
}
