#include "BT_Builder.h"

CBT_Builder::CBT_Builder()
{
}

CBT_Builder& CBT_Builder::Selector(const _wstring& strName)
{
    Push(CBT_Selector::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::Sequence(const _wstring& strName)
{
    Push(CBT_Sequence::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::ReactiveSelector(const _wstring& strName)
{
    Push(CBT_ReactiveSelector::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::ReactiveSequence(const _wstring& strName)
{
    Push(CBT_ReactiveSequence::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::Parallel(const _wstring& strName, CBT_Parallel::Parallel_Policy eSuccess, CBT_Parallel::Parallel_Policy eFailure)
{
    Push(CBT_Parallel::Create(strName, eSuccess, eFailure));
    return *this;
}

CBT_Builder& CBT_Builder::RandomSelector(const _wstring& strName)
{
    Push(CBT_RandomSelector::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::Inverter(const _wstring& strName)
{
    Push(CBT_Inverter::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::Repeat(const _wstring& strName, _int iCount)
{
    Push(CBT_Repeat::Create(strName, nullptr, iCount));
    return *this;
}

CBT_Builder& CBT_Builder::Cooldown(const _wstring& strName, _float fSeconds)
{
    Push(CBT_Cooldown::Create(strName, nullptr, fSeconds));
    return *this;
}

CBT_Builder& CBT_Builder::ForceSuccess(const _wstring& strName)
{
    Push(CBT_ForceSuccess::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::ForceFailure(const _wstring& strName)
{
    Push(CBT_ForceFailure::Create(strName));
    return *this;
}

CBT_Builder& CBT_Builder::Service(const _wstring& strName, ServiceFunc func, _float fInterval)
{
    Push(CBT_Service::Create(strName, nullptr, func, fInterval));
    return *this;
}

CBT_Builder& CBT_Builder::Task(const _wstring& strName, TaskFunc func)
{
    Leaf(CBT_Task::Create(strName, func));
    return *this;
}

CBT_Builder& CBT_Builder::Condition(const _wstring& strName, ConditionFunc func)
{
    Leaf(CBT_Condition::Create(strName, func));
    return *this;
}

CBT_Builder& CBT_Builder::Wait(const _wstring& strName, _float fSeconds)
{
    Leaf(CBT_Wait::Create(strName, fSeconds));
    return *this;
}

CBT_Builder& CBT_Builder::End()
{
    if (false == m_Stack.empty())
        m_Stack.pop_back();

    return *this;
}

void CBT_Builder::Attach(CBT_Node* pNode)
{
    if (nullptr == pNode) return;

    if (m_Stack.empty())
    {
        m_pRoot = pNode;
        return;
    }

    CBT_Node* pParent = m_Stack.back();
    if (nullptr != pParent)
        pParent->Attach_Child(pNode);
}

void CBT_Builder::Push(CBT_Node* pNode)
{
    Attach(pNode);
    m_Stack.push_back(pNode);
}

void CBT_Builder::Leaf(CBT_Node* pNode)
{
    Attach(pNode);
}

CBT_Builder* CBT_Builder::Create()
{
    CBT_Builder* pInstance = new CBT_Builder();
    if (nullptr == pInstance)
    {
        MSG_BOX("Failed to Created : CBT_Builder");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CBT_Builder::Free()
{
    m_Stack.clear();

    __super::Free();
}
