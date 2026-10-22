#pragma once

#include "Transform.h"

#ifdef _DEBUG
#include "Engine_Reflection.h"
#endif

NS_BEGIN(Engine)

class ENGINE_DLL CGameObject abstract : public CBase
{
#ifdef _DEBUG
	DECLARE_REFLECTION_BASE(CGameObject)
#endif

public:
	typedef struct tagGameObjectDesc : public CTransform::TRANSFORM_DESC
	{
	}GAMEOBJECT_DESC;

protected:
	CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CGameObject(const CGameObject& Prototype);
	virtual ~CGameObject() = default;

public:
	class CComponent*		Find_Component(const _wstring strComponentTag);
	class CTransform*		Get_Transform() { return m_pTransformCom; }

public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);
	virtual void			Priority_Update(_float fTimeDelta);
	virtual void			Update(_float fTimeDelta);
	//TODO : 프레임워크 수정사항. 물리 연산 Update 함수 추가 
	virtual void 			Physical_Update(_float fFixedTime) {};
	virtual void			Late_Update(_float fTimeDelta);
	virtual HRESULT			Render();
	virtual HRESULT			Render_Shadow() { return S_OK; };

public:
	//ObjectPool 재활성화 시 초기화 용도
	virtual HRESULT         Clear_Object(void* pArg) { return S_OK; };

public:
	//Object Update 상태 관리 
	_bool					Is_InActive() { return m_eObjectState == OBJECT_STATE::INACTIVE; }
	virtual void			Set_InActive() { m_eObjectState = OBJECT_STATE::INACTIVE; }
	void					Set_Active() { m_eObjectState = OBJECT_STATE::ACTIVE; }
	virtual _bool			Is_Dead() const { return false; }
public:
	unordered_map<_wstring, class CComponent*>* Get_ComponentMap();

	const _float4x4*		Get_WorldTransform();
	_wstring&				Get_Name() { return m_strObjectName; }
	// const GameObject에서도 이름을 읽을 수 있도록 읽기 전용 오버로드를 제공한다.
	const _wstring&			Get_Name() const { return m_strObjectName; }
	void					Set_State(STATE eState, _fvector vState);

	void					Set_EmissiveAmount(_float fAmount) { m_fEmissiveAmount = fAmount; }
	void					Set_RimProfile(_int	iProfile) { m_iRimProfile = iProfile; }
	void					Set_RimAmount(_float fAmount) { m_fRimAmount = fAmount; }
public:
	//충돌시 호출되는 함수 
	//virtual void            Collision(CollisionResult& result) {}
	virtual _bool			Take_Damage(const HitInfo& hitInfo) { return true; }

	//Rigid Contact 
	virtual void			Begin_Contact(CGameObject* pOther, _fvector vNormal) {}

	//Trigger Overlap 
	virtual void			Begin_Overlap(CGameObject* pOther) {}
	virtual void			End_Overlap(CGameObject* pOther) {}

public:
    _float                  Get_ViewDist() const { return m_fViewDist; }
    //Render 직전에서 호출하기 
    void                    Calculate_ViewDist();

protected:
	HRESULT					Add_Component(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag,
		const _wstring& strComponentTag, CComponent** ppOut, void* pArg = nullptr);

	//TODO : 프레임워크 수정사항. Clone이 아닌 원본 컴포넌트를 삽입하는 함수 오버로딩 추가
	HRESULT					Add_Component(CComponent * pComp, const _wstring& strComponentTag);

	void					Set_Name(_wstring strName);

protected:
	ID3D11Device*			m_pDevice			= { nullptr };
	ID3D11DeviceContext*	m_pContext			= { nullptr };
	class CGameInstance*	m_pGameInstance		= { nullptr };

protected:
	unordered_map<_wstring, class CComponent*>		m_Components;

	class CTransform*		m_pTransformCom		= { nullptr };

protected:
	//렌더러 정렬키 
	_uint					m_iSortKey			= {};
	_wstring				m_strObjectName		= {};
    
    //카메라까지의 거리. 블랜딩하면 계산
    _float                  m_fViewDist         = {};

	_int					m_iRimProfile		= { 0 };
	_float					m_fEmissiveAmount	= { 0.f };
	_float					m_fRimAmount		= { 0.f };

	OBJECT_STATE			m_eObjectState		= { OBJECT_STATE::END };

public:
	virtual CGameObject*	Clone(void* pArg) { return nullptr; };
	virtual void			Free();
};

NS_END
