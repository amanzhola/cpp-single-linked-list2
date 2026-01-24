// ===============================================================
// Lesson 6_9 (Task1) — SINGLE FILE WITH FULL LOGS (tail_ version)
// UPDATED: review-friendly edition (contracts + assert helpers + [[nodiscard]] demo)
//
// Filename suggestion: task1_lesson6_9_full_logs_tail_review.cpp
//
// What this file demonstrates via console logs:
//
//  0) Base sanity: empty list, begin/end, before_begin/cbefore_begin
//  1) PopFront (noexcept) + DeletionSpy destructor called exactly once
//  2) before_begin/cbefore_begin behavior: ++before_begin == begin
//  3) InsertAfter: empty list, front insertion, middle insertion
//  4) EraseAfter: erase after cbefore_begin (removes first), middle, last
//  5) Strong exception guarantee for InsertAfter using ThrowOnCopy
//     + Step table explaining what may throw and why list is unchanged
//  6) All Lesson 5_9 features still work: comparisons, swap+free swap, copy, assignment
//  7) Copy-count note: tail_ range build does ONE copy per element (no vector buffer)
//  8) [[nodiscard]] demo (safe: "bad" line is under #if 0)
//
// Build (Windows / MinGW g++):
//   g++ -std=c++17 -O2 -Wall -Wextra -pedantic task1_lesson6_9_full_logs_tail_review.cpp -o task6_logs
// Run:
//   task6_logs.exe
// ===============================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <iostream>
#include <new>       // std::bad_alloc

template <typename Type>
class SingleLinkedList {
    // ===== Internal node =====
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next) : value(val), next_node(next) {}
        Type  value{};
        Node* next_node = nullptr;
    };

public:
    // ===== Iterator (forward) =====
    template <typename ValueType>
    class BasicIterator {
        friend class SingleLinkedList;
        explicit BasicIterator(Node* node) noexcept : node_(node) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = ValueType*;
        using reference         = ValueType&;

        BasicIterator() = default;

        // Allow Iterator -> ConstIterator conversion via copying node pointer
        BasicIterator(const BasicIterator<Type>& other) noexcept : node_(other.node_) {}
        BasicIterator& operator=(const BasicIterator&) = default;

        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }
        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return !(*this == rhs);
        }
        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }
        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return !(*this == rhs);
        }

        // CONTRACT:
        //  - Complexity: O(1)
        //  - No-throw: noexcept
        //  - Precondition: node_ != nullptr (incrementing end() is UB)
        BasicIterator& operator++() noexcept {
            SingleLinkedList::AssertNotNull(node_, "++ on end() iterator is UB");
            node_ = node_->next_node;
            return *this;
        }

        BasicIterator operator++(int) noexcept {
            BasicIterator old(*this);
            ++(*this);
            return old;
        }

        // CONTRACT:
        //  - Complexity: O(1)
        //  - No-throw: noexcept
        //  - Precondition: node_ != nullptr (dereferencing end() is UB)
        [[nodiscard]] reference operator*() const noexcept {
            SingleLinkedList::AssertNotNull(node_, "* on end() iterator is UB");
            return node_->value;
        }

        [[nodiscard]] pointer operator->() const noexcept {
            SingleLinkedList::AssertNotNull(node_, "-> on end() iterator is UB");
            return &node_->value;
        }

    private:
        Node* node_ = nullptr;
    };

public:
    using Iterator      = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - Postcondition: IsEmpty()==true, GetSize()==0
    SingleLinkedList() noexcept : head_(), tail_(nullptr), size_(0) {}

    // CONTRACT:
    //  - Complexity: O(n)
    //  - Strong exception guarantee: if allocation/copy throws, *this unchanged
    SingleLinkedList(std::initializer_list<Type> values) : SingleLinkedList() {
        AssignFromRange(values.begin(), values.end());
    }

    // CONTRACT:
    //  - Complexity: O(n)
    //  - Strong exception guarantee: if allocation/copy throws, *this unchanged
    SingleLinkedList(const SingleLinkedList& other) : SingleLinkedList() {
        AssignFromRange(other.begin(), other.end());
    }

    // CONTRACT:
    //  - Complexity: O(n)
    //  - Strong exception guarantee (copy-and-swap)
    SingleLinkedList& operator=(const SingleLinkedList& rhs) {
        if (this != &rhs) {
            SingleLinkedList tmp(rhs); // may throw
            swap(tmp);                 // noexcept commit
        }
        return *this;
    }

    ~SingleLinkedList() { Clear(); }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    [[nodiscard]] size_t GetSize() const noexcept { return size_; }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    [[nodiscard]] bool IsEmpty() const noexcept { return size_ == 0; }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - Strong exception guarantee: if new/copy throws, list unchanged
    void PushFront(const Type& value) {
        Node* new_node = new Node(value, head_.next_node); // may throw
        head_.next_node = new_node;
        ++size_;
        if (tail_ == nullptr) {
            tail_ = new_node;
        }
    }

    // CONTRACT:
    //  - Complexity: O(n)
    //  - No-throw: noexcept
    //  - Postcondition: list becomes empty
    void Clear() noexcept {
        Node* cur = head_.next_node;
        while (cur) {
            Node* victim = cur;
            cur = cur->next_node;
            delete victim;
        }
        head_.next_node = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - [[nodiscard]]: ignoring begin/end is often a caller bug
    [[nodiscard]] Iterator begin() noexcept { return Iterator{head_.next_node}; }
    [[nodiscard]] Iterator end()   noexcept { return Iterator{nullptr}; }

    [[nodiscard]] ConstIterator begin() const noexcept { return ConstIterator{head_.next_node}; }
    [[nodiscard]] ConstIterator end()   const noexcept { return ConstIterator{nullptr}; }

    [[nodiscard]] ConstIterator cbegin() const noexcept { return ConstIterator{head_.next_node}; }
    [[nodiscard]] ConstIterator cend()   const noexcept { return ConstIterator{nullptr}; }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - Returns iterator to dummy node before first element
    [[nodiscard]] Iterator before_begin() noexcept { return Iterator(&head_); }

    // NOTE: const_cast is OK because ConstIterator does not allow modifying Type.
    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }
    [[nodiscard]] ConstIterator before_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - Strong exception guarantee
    //  - Precondition: pos != end() (InsertAfter(end()) is UB)
    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        AssertNotNull(pos.node_, "InsertAfter(pos=end()) is UB");
        Node* new_node = new Node(value, pos.node_->next_node); // may throw
        pos.node_->next_node = new_node;
        ++size_;
        if (tail_ == nullptr || tail_ == pos.node_) {
            tail_ = new_node;
        }
        return Iterator(new_node);
    }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - Precondition: list not empty (PopFront on empty is UB)
    void PopFront() noexcept {
        assert(size_ > 0 && "PopFront on empty list is UB");
        Node* victim = head_.next_node;
        head_.next_node = victim->next_node;
        delete victim;
        --size_;
        if (size_ == 0) {
            tail_ = nullptr;
        }
    }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - Precondition:
    //      pos != end()
    //      pos has next element (pos.node_->next_node != nullptr)
    Iterator EraseAfter(ConstIterator pos) noexcept {
        AssertHasNext(pos.node_, "EraseAfter(pos) with no next element is UB");

        Node* victim = pos.node_->next_node;
        Node* after  = victim->next_node;

        pos.node_->next_node = after;

        // Keep tail_ consistent
        if (tail_ == victim) {
            tail_ = (pos.node_ == &head_) ? nullptr : pos.node_;
        }

        delete victim;
        --size_;
        return Iterator(after);
    }

    // CONTRACT:
    //  - Complexity: O(1)
    //  - No-throw: noexcept
    //  - Swaps pointers/size only (no element moves)
    void swap(SingleLinkedList& other) noexcept {
        std::swap(head_.next_node, other.head_.next_node);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

private:
    // Build list from [first,last) with tail-building (ONE copy per element into Node::value)
    // Strong exception guarantee via temp + swap
    template <typename InputIt>
    void AssignFromRange(InputIt first, InputIt last) {
        SingleLinkedList tmp;

        Node* last_node = &tmp.head_;
        for (auto it = first; it != last; ++it) {
            Node* new_node = new Node(*it, nullptr); // may throw
            last_node->next_node = new_node;
            last_node = new_node;
            ++tmp.size_;
        }
        tmp.tail_ = (tmp.size_ == 0) ? nullptr : last_node;

        swap(tmp); // noexcept commit
    }

private:
    // Debug-contract helpers (keep UB checks in one place)
    static void AssertNotNull(const Node* p, const char* msg) noexcept {
        assert(p != nullptr && msg);
    }
    static void AssertHasNext(const Node* p, const char* msg) noexcept {
        assert(p != nullptr && "internal: node must not be null");
        assert(p->next_node != nullptr && msg);
    }

private:
    Node   head_;            // dummy
    Node*  tail_ = nullptr;  // last real node or nullptr if empty
    size_t size_ = 0;
};

// ===== Free swap for ADL =====
template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

// ===== Comparisons (Lesson 5_9), review-friendly formatting =====
template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    // Optional micro-optimization (usually not needed, but correct and harmless):
    if (&lhs == &rhs) {
        return true;
    }

    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs < rhs);
}

// ---------- LOG HELPERS ----------
static void PrintTitle(const std::string& s) {
    std::cout << "\n========== " << s << " ==========\n";
}

template <typename T>
static void PrintList(const SingleLinkedList<T>& lst, const std::string& name) {
    std::cout << name << " size=" << lst.GetSize() << " [ ";
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << "]\n";
}

// ---------- TEST TYPES ----------
struct DeletionSpy {
    ~DeletionSpy() {
        if (deletion_counter_ptr) {
            ++(*deletion_counter_ptr);
        }
    }
    int* deletion_counter_ptr = nullptr;
};

// ThrowOnCopy for strong guarantee demo
struct ThrowOnCopy {
    ThrowOnCopy() = default;

    explicit ThrowOnCopy(int& copy_counter) noexcept : countdown_ptr(&copy_counter) {}

    ThrowOnCopy(const ThrowOnCopy& other) : countdown_ptr(other.countdown_ptr) {
        if (countdown_ptr) {
            if (*countdown_ptr == 0) {
                throw std::bad_alloc();
            }
            --(*countdown_ptr);
        }
    }

    ThrowOnCopy& operator=(const ThrowOnCopy&) = delete;
    int* countdown_ptr = nullptr;
};

static void PrintStepTable_InsertAfter() {
    std::cout
        << "\nSTEP TABLE (InsertAfter strong guarantee):\n"
        << "STEP / LINE                                         | MAY THROW? | list state\n"
        << "----------------------------------------------------+------------+-------------------------\n"
        << "Node* new_node = new Node(value, pos->next)         | yes        | unchanged (not linked)\n"
        << "pos->next_node = new_node                           | no         | changes only after ok\n"
        << "++size_                                              | no         | changes only after ok\n"
        << "(tail_ fix)                                          | no         | metadata only\n";
}

static void PrintStepTable_CopyAndSwap() {
    std::cout
        << "\nSTEP TABLE (copy-and-swap for operator=):\n"
        << "STEP / LINE                             | MAY THROW? | tmp state            | dst state\n"
        << "----------------------------------------+------------+----------------------+-------------------------\n"
        << "SingleLinkedList tmp(rhs)               | yes        | building copy        | unchanged (no swap yet)\n"
        << "  - inside tmp: new Node(*it)           | yes        | may be partial        | unchanged\n"
        << "exception thrown                        |            | tmp destroyed (RAII)  | unchanged\n"
        << "swap(tmp)                               | no         | commit               | changed only if reached\n";
}

// ---------- MAIN DEMO ----------
int main() {
    std::cout << std::boolalpha;

    PrintTitle("Lesson 6_9 Task1 FULL CHECKLIST (tail_ + review contracts)");
    std::cout
        << "We demonstrate EVERYTHING added in this lesson:\n"
        << "  1) PopFront (noexcept)\n"
        << "  2) before_begin / cbefore_begin\n"
        << "  3) InsertAfter (strong exception guarantee)\n"
        << "  4) EraseAfter (noexcept)\n"
        << "Plus we re-check Lesson 5_9 features still work:\n"
        << "  comparisons, swap+free swap, initializer_list, copy ctor, operator=\n"
        << "And we highlight: tail_ version avoids vector buffer (no double copy)\n";

    // --- 0) Empty sanity ---
    PrintTitle("0) Empty list sanity");
    SingleLinkedList<int> empty;
    const auto& cempty = empty;

    std::cout << "0.1 IsEmpty -> " << empty.IsEmpty() << " (expect true)\n";
    std::cout << "0.2 size==0 -> " << (empty.GetSize() == 0) << "\n";
    std::cout << "0.3 begin==end -> " << (empty.begin() == empty.end()) << "\n";

    // before_begin on empty
    std::cout << "0.4 before_begin == cbefore_begin -> "
              << (empty.before_begin() == empty.cbefore_begin()) << " (expect true)\n";

    // NOTE: ++before_begin() is valid because before_begin points to dummy head_
    std::cout << "0.5 ++before_begin == begin -> "
              << (++empty.before_begin() == empty.begin()) << " (expect true)\n";
    std::cout << "0.6 ++cbefore_begin == const begin -> "
              << (++empty.cbefore_begin() == cempty.begin()) << " (expect true)\n";

    // --- NODISCARD demo (safe) ---
    PrintTitle("0.N) [[nodiscard]] demo (compile-time warning if you ignore results)");
    [[maybe_unused]] auto it_ok = empty.begin();
#if 1
    // Uncomment this line to see a [[nodiscard]]-related warning on some compilers:
    empty.begin();
#endif
    std::cout << "0.N1 stored begin() in [[maybe_unused]] variable -> OK\n";
    std::cout << "0.N2 see #if 0 block to force a warning (kept disabled for clean build)\n";

    // --- 1) PopFront + DeletionSpy ---
    PrintTitle("1) PopFront (noexcept) + DeletionSpy");
    {
        SingleLinkedList<int> numbers{3, 14, 15, 92, 6};
        PrintList(numbers, "1.0 numbers(before)");
        numbers.PopFront();
        PrintList(numbers, "1.1 numbers(after PopFront)");

        std::cout << "1.2 numbers == {14,15,92,6} -> "
                  << (numbers == SingleLinkedList<int>{14, 15, 92, 6}) << " (expect true)\n";
    }
    {
        SingleLinkedList<DeletionSpy> list;
        list.PushFront(DeletionSpy{});
        int deletion_counter = 0;
        list.begin()->deletion_counter_ptr = &deletion_counter;

        std::cout << "1.3 deletion_counter(before) = " << deletion_counter << " (expect 0)\n";
        list.PopFront();
        std::cout << "1.4 deletion_counter(after)  = " << deletion_counter << " (expect 1)\n";
    }

    // --- 2) before_begin / cbefore_begin on non-empty ---
    PrintTitle("2) before_begin / cbefore_begin on non-empty");
    {
        SingleLinkedList<int> numbers{1, 2, 3, 4};
        const auto& cnumbers = numbers;

        std::cout << "2.1 before_begin == cbefore_begin -> "
                  << (numbers.before_begin() == numbers.cbefore_begin()) << " (expect true)\n";
        std::cout << "2.2 ++before_begin == begin -> "
                  << (++numbers.before_begin() == numbers.begin()) << " (expect true)\n";
        std::cout << "2.3 ++cbefore_begin == const begin -> "
                  << (++numbers.cbefore_begin() == cnumbers.begin()) << " (expect true)\n";
    }

    // --- 3) InsertAfter: empty and non-empty ---
    PrintTitle("3) InsertAfter examples (order + returned iterator)");
    {   // insert into empty: after before_begin
        SingleLinkedList<int> lst;
        auto inserted = lst.InsertAfter(lst.before_begin(), 123);
        PrintList(lst, "3.1 lst(after InsertAfter before_begin, 123)");

        std::cout << "3.2 lst == {123} -> " << (lst == SingleLinkedList<int>{123}) << " (expect true)\n";
        std::cout << "3.3 inserted == begin -> " << (inserted == lst.begin()) << " (expect true)\n";
        std::cout << "3.4 *inserted == 123 -> " << (*inserted == 123) << " (expect true)\n";
    }
    {   // insert into non-empty
        SingleLinkedList<int> lst{1, 2, 3};
        PrintList(lst, "3.5 lst(before)");

        auto inserted1 = lst.InsertAfter(lst.before_begin(), 123);
        PrintList(lst, "3.6 lst(after InsertAfter before_begin, 123)");

        std::cout << "3.7 inserted1 == begin -> " << (inserted1 == lst.begin()) << " (expect true)\n";
        std::cout << "3.8 lst == {123,1,2,3} -> "
                  << (lst == SingleLinkedList<int>{123, 1, 2, 3}) << " (expect true)\n";

        auto inserted2 = lst.InsertAfter(lst.begin(), 555); // after 123
        PrintList(lst, "3.9 lst(after InsertAfter begin, 555)");

        auto second_it = lst.begin();
        ++second_it;

        std::cout << "3.10 inserted2 is second element -> "
                  << (inserted2 == second_it) << " (expect true)\n";
        std::cout << "3.11 *inserted2 == 555 -> " << (*inserted2 == 555) << " (expect true)\n";
        std::cout << "3.12 lst == {123,555,1,2,3} -> "
                  << (lst == SingleLinkedList<int>{123, 555, 1, 2, 3}) << " (expect true)\n";
    }

    // --- 4) Strong exception guarantee: InsertAfter + ThrowOnCopy ---
    PrintTitle("4) InsertAfter strong exception guarantee (ThrowOnCopy)");
    {
        bool exception_was_thrown = false;

        for (int max_copy_counter = 10; max_copy_counter >= 0; --max_copy_counter) {
            SingleLinkedList<ThrowOnCopy> list{ThrowOnCopy{}, ThrowOnCopy{}, ThrowOnCopy{}};

            try {
                int copy_counter = max_copy_counter;
                // value passed by const& into InsertAfter will be copied into Node::value
                list.InsertAfter(list.cbegin(), ThrowOnCopy(copy_counter));

                // if it didn't throw, size must become 4
                if (list.GetSize() != 4u) {
                    std::cout << "4.X Unexpected: size != 4 after successful insert\n";
                    assert(false);
                }
            } catch (const std::bad_alloc&) {
                exception_was_thrown = true;
                std::cout << "4.1 caught std::bad_alloc at max_copy_counter=" << max_copy_counter << "\n";
                std::cout << "4.2 list size after throw = " << list.GetSize() << " (expect 3)\n";
                std::cout << "4.3 strong guarantee holds -> " << (list.GetSize() == 3u) << " (expect true)\n";
                break;
            }
        }

        std::cout << "4.4 exception_was_thrown -> " << exception_was_thrown << " (expect true)\n";
        PrintStepTable_InsertAfter();
    }

    // --- 5) EraseAfter examples + DeletionSpy ---
    PrintTitle("5) EraseAfter examples (+ DeletionSpy)");
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        const auto& clst = lst;

        PrintList(lst, "5.0 lst(before)");
        auto after_erased = lst.EraseAfter(clst.cbefore_begin()); // remove first (1)
        PrintList(lst, "5.1 lst(after EraseAfter cbefore_begin) => removed first");

        std::cout << "5.2 lst == {2,3,4} -> " << (lst == SingleLinkedList<int>{2, 3, 4}) << " (expect true)\n";
        std::cout << "5.3 returned == begin -> " << (after_erased == lst.begin()) << " (expect true)\n";
    }
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        PrintList(lst, "5.4 lst(before)");

        auto after_erased = lst.EraseAfter(lst.cbegin()); // remove 2
        PrintList(lst, "5.5 lst(after EraseAfter cbegin) => removed 2");

        auto expected_it = lst.begin();
        ++expected_it;

        std::cout << "5.6 lst == {1,3,4} -> " << (lst == SingleLinkedList<int>{1, 3, 4}) << " (expect true)\n";
        std::cout << "5.7 returned == ++begin -> " << (after_erased == expected_it) << " (expect true)\n";
    }
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        PrintList(lst, "5.8 lst(before)");

        auto it = lst.cbegin();
        ++it; // points to 2
        ++it; // points to 3
        auto after_erased = lst.EraseAfter(it); // erase after 3 => remove 4 (last)

        PrintList(lst, "5.9 lst(after EraseAfter it(3)) => removed last");

        std::cout << "5.10 lst == {1,2,3} -> " << (lst == SingleLinkedList<int>{1, 2, 3}) << " (expect true)\n";
        std::cout << "5.11 returned == end -> " << (after_erased == lst.end()) << " (expect true)\n";
    }
    {
        SingleLinkedList<DeletionSpy> list{DeletionSpy{}, DeletionSpy{}, DeletionSpy{}};

        auto second = list.begin();
        ++second;

        int deletion_counter = 0;
        second->deletion_counter_ptr = &deletion_counter;

        std::cout << "5.12 deletion_counter(before) = " << deletion_counter << " (expect 0)\n";
        list.EraseAfter(list.cbegin()); // erase 2nd element
        std::cout << "5.13 deletion_counter(after)  = " << deletion_counter << " (expect 1)\n";
    }

    // --- 6) Lesson 5_9 features still OK (comparisons + swap + copy + assignment) ---
    PrintTitle("6) Lesson 5_9 features re-check (comparisons/swap/copy/assign)");
    {
        using IntList = SingleLinkedList<int>;
        IntList a{1, 2, 3};
        IntList b{1, 2, 3};
        IntList c{1, 2, 3, 1};
        IntList d{1, 3};
        IntList e{1, 3, 0};

        std::cout << "6.1 a==b -> " << (a == b) << " (expect true)\n";
        std::cout << "6.2 a!=c -> " << (a != c) << " (expect true)\n";
        std::cout << "6.3 a<c  -> " << (a < c)  << " (expect true)\n";
        std::cout << "6.4 a<d  -> " << (a < d)  << " (expect true)\n";
        std::cout << "6.5 d>e  -> " << (d > e)  << " (expect false: d=[1,3], e=[1,3,0] => d<e)\n";
        std::cout << "6.6 d<e  -> " << (d < e)  << " (expect true)\n";
    }
    {
        SingleLinkedList<int> first;
        first.PushFront(1);
        first.PushFront(2);

        SingleLinkedList<int> second;
        second.PushFront(10);
        second.PushFront(11);
        second.PushFront(15);

        const auto old_first_begin  = first.begin();
        const auto old_second_begin = second.begin();
        const auto old_first_size   = first.GetSize();
        const auto old_second_size  = second.GetSize();

        PrintList(first,  "6.7 first(before swap)");
        PrintList(second, "6.8 second(before swap)");

        first.swap(second);

        PrintList(first,  "6.9 first(after member swap)");
        PrintList(second, "6.10 second(before swap)");

        std::cout << "6.11 second.begin() == old_first_begin -> "
                  << (second.begin() == old_first_begin) << " (expect true)\n";
        std::cout << "6.12 first.begin()  == old_second_begin -> "
                  << (first.begin() == old_second_begin) << " (expect true)\n";
        std::cout << "6.13 sizes swapped -> "
                  << (second.GetSize() == old_first_size && first.GetSize() == old_second_size) << " (expect true)\n";

        // ADL free swap check
        {
            using std::swap;
            swap(first, second); // should call our free swap (ADL)

            std::cout << "6.14 ADL free swap restored iterators (no node copy):\n";
            std::cout << "     first.begin()==old_first_begin -> " << (first.begin() == old_first_begin) << "\n";
            std::cout << "     second.begin()==old_second_begin -> " << (second.begin() == old_second_begin) << "\n";
            std::cout << "     sizes restored -> "
                      << (first.GetSize() == old_first_size && second.GetSize() == old_second_size) << "\n";
        }
    }
    {
        SingleLinkedList<int> src{7, 8, 9};
        SingleLinkedList<int> copy(src);

        PrintList(src,  "6.15 src");
        PrintList(copy, "6.16 copy");

        std::cout << "6.17 copy==src -> " << (copy == src) << " (expect true)\n";
        std::cout << "6.18 begin pointers differ (deep copy) -> " << (copy.begin() != src.begin()) << " (expect true)\n";

        SingleLinkedList<int> dst{1, 1, 1};
        dst = src;

        PrintList(dst, "6.19 dst(after operator=)");
        std::cout << "6.20 dst==src -> " << (dst == src) << " (expect true)\n";
        std::cout << "6.21 dst.begin()!=src.begin -> " << (dst.begin() != src.begin()) << " (expect true)\n";

        PrintStepTable_CopyAndSwap();
    }

    // --- 7) Copy-count note (buffer vs tail_) ---
    PrintTitle("7) Copy-count note: buffer vs tail_ (why no double-copy here)");
    std::cout
        << "In old PushFront-only range build with std::vector buffer:\n"
        << "  copy #1: push_back(*it) into vector\n"
        << "  copy #2: PushFront(*rit) into Node::value\n"
        << "So up to TWO copies per element (plus possible realloc moves).\n\n"
        << "In this tail_ range build:\n"
        << "  copy #1: new Node(*it, nullptr) directly into Node::value\n"
        << "ONE copy per element, same external behavior.\n";

    PrintTitle("DONE");
    std::cout << "If all 'expect true' checks match, you demonstrated Lesson 6_9 Task1 + kept 5_9 features.\n";
    return 0;
}
