#pragma once
#include "Component.h"

/* 모델 Dissolve 클래스 
- Dissolve 소멸 연출의 타이밍을 담당하는 컴포넌트.
- 필요한 오브젝트만 Component 생성해서 사용함.
- 대기(StartDelay) -> Dissolve(Duration) -> 대기 (EndDelay)의 순서를 가짐.
- 상술했듯이, 이 Component는 진행도와 Dissolve 색만을 제공함. 실제 렌더의 반영은 소유자가 Shader를 통해서 진행한다.
- Get_Amount()와 Get_Color()를 읽어 자기 자신의 Set_Dissolve로 Dissolve를 진행하는 형태.
*/

NS_BEGIN(Engine)

class ENGINE_DLL CDissolve final : public CComponent
{
public:
	enum class DISSOLVE_MODE { DISSOLVE_OUT, DISSOLVE_IN };

	enum class DISSOLVE_PHASE { NONE, DELAY, RUN, HOLD, DONE };

	typedef struct tagDissolveDesc
	{
		_float fStartDelay = { 2.f };	// Dissolve 시작까지 대기할 시간
		_float fDuration = { 1.5f };	// Dissolve를 진행할 시간
		_float fEndDelay = { 2.f };		// Dissolve 완료 후 유지할 시간
		_float4	vEdgeColor = { 1.f, 1.f, 1.f, 1.f };	// 디졸브 경계 발광 색, 색(rgb) + 세기(a)
	} DISSOLVE_DESC;

private:
	CDissolve();
	virtual ~CDissolve() = default;

public:
	virtual HRESULT		Initialize(void* pArg) override;

public:
	void				Begin(DISSOLVE_MODE eMode = DISSOLVE_MODE::DISSOLVE_OUT);					// Dissolve 시퀀스 시작
	void				Update(_float fTimeDelta);	// 진행도 업데이트
	void				Reset();					// 상태 원상 복구

	_bool				Is_Playing() const { return m_bPlaying; }
	_bool				Is_Finished() const { return m_bFinished; }
	_float				Get_Amount() const { return (m_eMode == DISSOLVE_MODE::DISSOLVE_OUT) ? m_fProgress : (1.f - m_fProgress); }
	const _float4&		Get_Color() const { return m_tDesc.vEdgeColor; }
	DISSOLVE_MODE		Get_Mode() const { return m_eMode; }
	DISSOLVE_PHASE		Get_Phase() const;

	void				Set_Timing(_float fStartDelay, _float fDuration, _float fEndDelay);
	void				Set_Color(const _float4& vColor) { m_tDesc.vEdgeColor = vColor; }

private:
	DISSOLVE_DESC		m_tDesc = {};
	DISSOLVE_MODE		m_eMode = { DISSOLVE_MODE::DISSOLVE_OUT };

	_bool				m_bPlaying = { false };
	_bool				m_bFinished = { false };
	_float				m_fTimer = { 0.f };
	_float				m_fProgress = { 0.f };

public:
	static CDissolve*	Create(void* pArg = nullptr);
	virtual void		Free() override;
};

NS_END