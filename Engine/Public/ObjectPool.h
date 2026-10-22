#pragma once
#include "Base.h"
#include "IPoolable.h"


NS_BEGIN(Engine)

class IObjectPool : public CBase
{
public:
	virtual ~IObjectPool() = default;
	virtual void Receive_Object(IPoolable* pObj) PURE;
};

template<typename T>
class ITypedObjectPool :public IObjectPool
{
public:
	virtual T* Get_Object(void* pArg = nullptr) PURE;
};

/* Object Pool */
template<typename T, size_t Size>
class CObjectPool : public ITypedObjectPool<T>
{
private:
	CObjectPool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CObjectPool() = default;

	CObjectPool(const CObjectPool& rhs) = delete;
	CObjectPool& operator=(const CObjectPool& rhs) = delete;

public:
	HRESULT							Initialize();

	//pArg : 초기화 시 필요한 데이터가 있으면 전달 
	T*								Get_Object(void* pArg = nullptr) override; 	//객체 반환
	void							Receive_Object(IPoolable* pObj) override;//객체 회수

protected:
	ID3D11Device*					m_pDevice	= { nullptr };
	ID3D11DeviceContext*			m_pContext	= { nullptr };

	vector<T*>						m_Pool;
	//대기중인 오브젝트의 인덱스 리스트. Vector의 pop_back, push_back으로 캐시미스를 줄이는 방식
	vector<size_t>					m_FreeObjectIndices;
public:
	static CObjectPool<T, Size>*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void					Free() override;
};


/* Template 정의부 */

template<typename T, size_t Size>
inline CObjectPool<T, Size>::CObjectPool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

template<typename T, size_t Size>
HRESULT CObjectPool<T, Size>::Initialize()
{
	m_Pool.reserve(Size);
	m_FreeObjectIndices.reserve(Size);

	for(size_t i = 0; i < Size; ++i)
	{
		T* pObj = T::Create(m_pDevice, m_pContext);
		if (nullptr == pObj) return E_FAIL;

		pObj->Set_OwnerPool(this);
		pObj->Set_PoolIndex(i);

		m_Pool.push_back(pObj);
		m_FreeObjectIndices.push_back(i);
	}
	return S_OK;
}

template<typename T, size_t Size>
inline T* CObjectPool<T, Size>::Get_Object(void* pArg)
{
	if (m_FreeObjectIndices.empty()) return nullptr;

	size_t freeIndex = m_FreeObjectIndices.back();
	m_FreeObjectIndices.pop_back();

	T* pObj = m_Pool[freeIndex];
	Safe_AddRef(pObj);

	pObj->Activate(pArg);
	return pObj;
}

template<typename T, size_t Size>
inline void CObjectPool<T, Size>::Receive_Object(IPoolable* pObj)
{
	if (nullptr == pObj) return;
	m_FreeObjectIndices.push_back(pObj->Get_PoolIndex());
}


template<typename T, size_t Size>
CObjectPool<T, Size>* CObjectPool<T, Size>::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CObjectPool* pInstance = new CObjectPool(pDevice, pContext);
	if (FAILED(pInstance->Initialize()))
	{
		Safe_Release(pInstance);
		return nullptr;
	}
	return pInstance;
}

template<typename T, size_t Size>
inline void CObjectPool<T, Size>::Free()
{
	for (_uint i = 0; i < m_Pool.size(); ++i)
	{
		m_Pool[i]->Set_OwnerPool(nullptr);
	}

	//대기중인 오브젝트 해제
	while(!m_FreeObjectIndices.empty())
	{
		size_t iIndex = m_FreeObjectIndices.back();
		m_FreeObjectIndices.pop_back();

		Safe_Release(m_Pool[iIndex]);
	}
	//외부에 있는 나머지 오브젝트는 외부 참조가 다 종료되면 연결된 풀이 없으므로 알아서 해제됨
	//총알 등 외부에 생성된 오브젝트를 대비함 

	m_Pool.clear();
	m_FreeObjectIndices.clear();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}


NS_END
