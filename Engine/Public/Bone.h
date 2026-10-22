#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CBone final : public CBase
{
private:
	CBone();
	virtual ~CBone() = default;

public:

	_bool Compare_Name(const _char* pBoneName) 
	{
		return !strcmp(pBoneName, m_szName);
	}

	const _int Get_ParentIndex() const { return m_iParentIndex; }

	const _char* Get_Name() const { return m_szName; }

	const _float4x4* Get_TransformationMatrixPtr() const
	{
		return &m_TransformationMatrix;
	}

	const _float4x4* Get_CombinedTransformationMatrixPtr() const 
	{
		return &m_CombinedTransformationMatrix;
	}

	void Set_TransformationMatrix(_fmatrix TransformationMatrix) {
		XMStoreFloat4x4(&m_TransformationMatrix, TransformationMatrix);
	}

public:
	HRESULT				Initialize(char* szName, _int iParentIndex, _fmatrix transformMatrix);
	void				Update_CombinedTransformMatrices(const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);

private:
	_char				m_szName[MAX_PATH] = {};
	_int				m_iParentIndex = { -1 };
	_float4x4			m_TransformationMatrix = {};
	_float4x4			m_CombinedTransformationMatrix = {};


public:
	static CBone*		Create(char* szName, _int iParentIndex, _fmatrix transformMatrix);
	CBone*				Clone();
	virtual void		Free() override;
};

NS_END