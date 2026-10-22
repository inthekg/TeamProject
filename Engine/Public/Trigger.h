#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

//박스 Shape고정. 크기는 vHalfExtents로. 위치 회전은 TransformDesc으로 적용 
class CStaticRigidBody;
class ENGINE_DLL CTrigger : public CGameObject
{
public:
	typedef struct tagTriggerDesc : public::CGameObject::GAMEOBJECT_DESC
	{
		_float3 vHalfExtents = { 0.5f, 0.5f, 0.5f }; // 박스 크기 
		_uint	iTargetFilter = ETOUI(FILTER_GROUP::PLAYER);
		_bool	bOnceTrigger = { false }; // 일회성 여부 
		CGameObject* pLogicalOwner = { nullptr }; // Trigger 소유자 
		function<void(CGameObject*)>	OnBeginOverlap = { nullptr };
		function<void(CGameObject*)>	OnEndOverlap = { nullptr };
	} TRIGGER_DESC;

private:
	CTrigger(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTrigger(const CTrigger& Prototype);
	virtual ~CTrigger() = default;

public:
	virtual HRESULT					Initialize_Prototype() override;
	virtual HRESULT					Initialize(void* pArg) override;

public:
	// Trigger 이벤트 바인딩
	void							Bind_OnBeginOverlap(function<void(CGameObject*)> func) { m_Func_OnBeginOverlap = func; }
	void							Bind_OnEndOverlap(function<void(CGameObject*)> func) { m_Func_OnEndOverlap = func; }

	// 외부 Trigger에 연결된 실제 논리 오너를 반환한다.
	CGameObject*					Get_LogicalOwner() const { return m_pLogicalOwner; }

	// Trigger 이벤트
	// 특정 기능을 가진 트리거가 필요하면 바인딩 대신 아예 상속받아 하위에서 기능 정의하기 
	virtual void					Begin_Overlap(CGameObject* pOther) override;
	virtual void					End_Overlap(CGameObject* pOther) override;
private:
	HRESULT							Ready_Components(TRIGGER_DESC* pDesc);

private:
	class CStaticRigidBody*			m_pRigidStaticBody = { nullptr };
	_bool							m_bOnce = { false };

	// MapTriggerManager가 TargetSpawnID로 찾아 연결한 오너다.
	CGameObject*					m_pLogicalOwner = { nullptr };

	function<void(CGameObject*)>	m_Func_OnBeginOverlap;
	function<void(CGameObject*)>	m_Func_OnEndOverlap;
public:
	static CTrigger*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*			Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END

