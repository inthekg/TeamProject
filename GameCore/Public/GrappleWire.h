#pragma once
#include "Base.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CGameInstance;
class CVIBuffer_Trail;
struct EMITTER_SHADER_BUNDLE;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CGrappleWire final : public CBase
{
public:
	typedef struct tagWireDesc
	{
		_float4 	vColor			= { 0.08f, 0.08f, 0.09f, 1.0f };
		//로프의 마디 개수 
		_uint		iSegments		= { 25 };
		_float		fWidth			= { 0.035f };
		//전체 제약 교정의 반복 횟수 
		_uint		iSolverCount	= { 3 };
		//고정 프레임당 시뮬레이션 시간
		_float		fSubStepTime	= { 0.01f };
	}WIRE_DESC;

	typedef struct tagWireProfile
	{
		_float		fSlack			= { 1.05f };
		_float		fGravityScale	= { 1.f };
		_float		fDamping		= { 0.98f }; //감쇄
	} WIRE_PROFILE;

private:
	CGrappleWire();
	virtual ~CGrappleWire() = default;

public:
	HRESULT							Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const WIRE_DESC& Desc);
	void							Update(_float fTimeDelta);
	HRESULT							Render();
public:
	//로프 전체 포인트 초기화 후 활성화   
	void							Activate(_fvector vStart, _fvector vEnd);
	void							Hide() { m_bVisible = false; }
	_bool							Is_Visible() const { return m_bVisible; }

public:
	void							Set_Endpoints(_fvector vStart, _fvector vEnd);
	void							Set_Profile(const WIRE_PROFILE& Profile) { m_Profile = Profile; }
private:
	//Verlet베를레 적분으로 포인트 위치 갱신 
	void							Update_Points(_float fTimeDelta);
	void							Solve_Constraints();
private:
	static constexpr _float			MAX_TIMEDELTA	= { 0.05f };
private:
	CGameInstance*					m_pGameInstance = { nullptr };

	WIRE_PROFILE					m_Profile		= {};
	WIRE_DESC						m_Desc			= {};
	_bool							m_bVisible		= { false };
	_float							m_fSubStepAccum	= { 0.f };

	_float3							m_vStart		= {};
	_float3							m_vEnd			= {};

	//로프 마디별 데이터 
	vector<_float3>					m_CurPos;
	vector<_float3>					m_PrevPos;
private:
	CVIBuffer_Trail*				m_pBuffer		= { nullptr };
	const EMITTER_SHADER_BUNDLE*	m_pShaderBundle	= { nullptr };

public:
	static CGrappleWire*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const WIRE_DESC& Desc);
	virtual void					Free() override;
};

NS_END