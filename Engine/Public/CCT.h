#pragma once
#include "BasePhysic.h"

/* CCT는 위치 = 콜라이더의 정중앙. 캐릭터를 콜라이더의 발 위치에 Set하는 보정이 필요함 */
/* 추가 Shape에 다른 필터링 줘야하면 Desc에 추가하기  */
NS_BEGIN(Engine)
class ENGINE_DLL CCT : public CBasePhysic
{
public:
	typedef struct tagControllerDesc : public PHYSIC_DESC
	{
		CCT_TYPE	eControllerType		= { CCT_TYPE::END };	// CCT Shape Type
		_float		fRadius				= { 0.5f };				// 반지름 (캡슐)
		_float		fHeight				= { 1.f };				// 높이 (캡슐)
		_float3		vBoxHalf			= { 0.5f, 0.5f, 0.5f };	// 박스
		_float		fSlopeLimit			= { 0.f };				// 경사로 최대 각도 
		_float		fStepOffset			= { 0.f };				// 계단 높이 한계
		_float		fContactOffset		= { 0.1f };				// 충돌 오프셋 보통 0.1f 정도로 설정

	} CONTROLLER_DESC;

private:
	CCT(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCT(const CCT& Prototype);
	virtual ~CCT() = default;	

public:
	virtual HRESULT		Initialize_Prototype() override;
	virtual HRESULT		Initialize(void* pArg) override;

public:
	PxController*		Get_Controller() const { return m_pController; }
	_float				Get_Radius() const { return m_fRadius; }
	void				Set_Active(_bool bActive);
	_bool				Is_PhysicsActive() const { return m_bPhysicsActive; }
	void				InvalidateCache();
public:
	CCTResult			Move(_fvector vDeltaPos, _float fFixedTime);

	void				Set_FootPosition(_fvector vPosition);
	_vector				Get_FootPosition() const;
	void				Resize(_float fHeight);
	void				Resize_Radius(_float fRadius);

private:
	PxControllerFilters	m_ControllerFilters;
	PxFilterData		m_FilterData;
	_bool				m_bPhysicsActive		= { true };

	PxController*		m_pController			= { nullptr };
	CCT_TYPE			m_eControllerType		= { CCT_TYPE::END };
	//캡슐이면 radius, 박스면 halfSideExtent
	_float 				m_fRadius = { 0.f };
public:
	static CCT*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg) override;
	virtual void		Free() override;
	
};

NS_END