#pragma once
#include "Core_Defines.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL IItemable
{
protected:
	IItemable() = default;
	virtual  ~IItemable() = default;

public:
	void Set_ItemID(_uint64 iItemID) { m_iItemID = iItemID; }
	_uint64 Get_ItemID() const { return m_iItemID; }

protected:
	_uint64 m_iItemID = { 0 };
};

NS_END
