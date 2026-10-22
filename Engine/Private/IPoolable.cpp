#include "IPoolable.h"
#include "ObjectPool.h"

_uint IPoolable::Return_ToPool()
{
	InActivate();
	m_pOwnerPool->Receive_Object(this);
	return 0;
}
