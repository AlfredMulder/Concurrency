#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>      // for cds::HP (Hazard Pointer) SMR

struct node
{
    node* m_p_next;
};

class queue
{
    node* m_p_head_ = nullptr;
    node* m_p_tail_ = nullptr;
public:
    queue() = default;

    void enqueue(node* p)
    {
        if (m_p_tail_)
        {
            m_p_tail_->m_p_next = p;
        }
        p->m_p_next = nullptr;
        m_p_tail_ = p;
        if (!m_p_head_)
        {
            m_p_head_ = p;
        }
    }

    node* dequeue()
    {
        if (!m_p_head_) return nullptr;
        node* p = m_p_head_;
        m_p_head_ = p->m_p_next;
        if (!m_p_head_)
        {
            m_p_tail_ = nullptr;
        }
        return p;
    }
};

// cds::intrusive::MSQueue
// bool enqueue(value_type& val)
// {
//     node_type* pNew = node_traits::to_node_ptr(val);
//
//     typename gc::Guard guard;
//     back_off bkoff;
//
//     node_type* t;
//     while (true)
//     {
//         t = guard.protect(m_pTail, node_to_value());
//
//         node_type* pNext = t->m_pNext.load(memory_model::memory_order_acquire);
//         if (pNext != null_ptr<node_type*>())
//         {
//             // Tail is misplaced, advance it
//             m_pTail.compare_exchange_weak(t, pNext,
//                                           memory_model::memory_order_release,
//                                           CDS_ATOMIC::memory_order_relaxed);
//             continue ;
//         }
//
//         node_type* tmp = null_ptr<node_type*>();
//         if (t->m_pNext.compare_exchange_strong(tmp, pNew,
//                                                memory_model::memory_order_release,
//                                                CDS_ATOMIC::memory_order_relaxed))
//         {
//             break ;
//         }
//         bkoff();
//     }
//     ++m_ItemCounter;
//
//     m_pTail.compare_exchange_strong(t, pNew, memory_model::memory_order_acq_rel,
//                                     CDS_ATOMIC::memory_order_relaxed);
//
//     return true;
// }
//
// value_type* dequeue()
// {
//     node_type* pNext;
//     back_off bkoff;
//     typename gc::GuardArray<2> guards;
//
//     node_type* h;
//     while (true)
//     {
//         h = guards.protect(0, m_pHead, node_to_value());
//         pNext = guards.protect(1, h->m_pNext, node_to_value());
//         if (m_pHead.load(memory_model::memory_order_relaxed) != h)
//             continue ;
//
//         if (pNext == null_ptr<node_type*>())
//             return nullptr; // empty queue
//
//         node_type* t = m_pTail.load(memory_model::memory_order_acquire);
//         if (h == t)
//         {
//             // It is needed to help enqueue
//             m_pTail.compare_exchange_strong(t, pNext,
//                                             memory_model::memory_order_release,
//                                             CDS_ATOMIC::memory_order_relaxed);
//             continue ;
//         }
//
//         if (m_pHead.compare_exchange_strong(h, pNext,
//                                             memory_model::memory_order_release,
//                                             CDS_ATOMIC::memory_order_relaxed))
//         {
//             break ;
//         }
//         bkoff();
//     }
//
//     --m_ItemCounter;
//
//     dispose_node(h);
//     return pNext;
// }


int main(int argc, char** argv)
{
    // Initialize libcds
    cds::Initialize();
    {
        // Initialize Hazard Pointer singleton
        cds::gc::HP hp_gc;
        // If main thread uses lock-free containers
        // the main thread should be attached to libcds infrastructure
        cds::threading::Manager::attachThread();
        // Now you can use HP-based containers in the main thread
        //...
    }
    // Terminate libcds
    cds::Terminate();
}
