// ===============================================================
// Lesson 6_9 (Task1) — SINGLE FILE WITH FULL LOGS (tail_ version)
// UPDATED: review-friendly edition (contracts + assert helpers + [[nodiscard]] demo)
// PLUS: pretty emoji report output (sections + checks + tips)
//
// Filename suggestion: task1_lesson6_9_full_logs_tail_review.cpp
//
// (!)Important(!): В Windows CMD need UTF-8 to display emoji use: chcp 65001
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

// ---------- PRETTY REPORT UI ----------
namespace ui {

inline void Banner(const std::string& title) {
    std::cout << "\n🌟🌟🌟🌟🌟  " << title << "  🌟🌟🌟🌟🌟\n";
}

inline void Section(const std::string& title) {
    std::cout << "\n🧩━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━🧩\n";
    std::cout << "📌 " << title << "\n";
    std::cout << "🧩━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━🧩\n";
}

inline void Sub(const std::string& title) {
    std::cout << "\n🔸 " << title << "\n";
}

inline void Info(const std::string& msg) { std::cout << "ℹ️  " << msg << "\n"; }
inline void Tip (const std::string& msg) { std::cout << "💡 " << msg << "\n"; }
inline void Warn(const std::string& msg) { std::cout << "⚠️  " << msg << "\n"; }
inline void Boom(const std::string& msg) { std::cout << "💥 " << msg << "\n"; }

inline void Check(const std::string& id, bool actual, bool expected = true,
                  const std::string& note = "") {
    if (actual == expected) {
        std::cout << "🟢✅ " << id << "  — OK";
    } else {
        std::cout << "🔴🧯 " << id << "  — FAIL";
    }
    std::cout << "  (got=" << actual << ", expected=" << expected << ")";
    if (!note.empty()) std::cout << "  📝 " << note;
    std::cout << "\n";
}

template <typename T>
inline void CheckEq(const std::string& id, const T& got, const T& expected,
                    const std::string& note = "") {
    if (got == expected) {
        std::cout << "🟢🎯 " << id << "  — OK";
    } else {
        std::cout << "🔴📉 " << id << "  — FAIL";
    }
    std::cout << "  (got=" << got << ", expected=" << expected << ")";
    if (!note.empty()) std::cout << "  📝 " << note;
    std::cout << "\n";
}

} // namespace ui

// ===============================================================
//                        SingleLinkedList
// ===============================================================
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
template <typename T>
static void PrintList(const SingleLinkedList<T>& lst, const std::string& name) {
    std::cout << "📎 " << name << "  (size=" << lst.GetSize() << ")  ➜  [ ";
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
        << "\n📊 STEP TABLE (InsertAfter strong guarantee 🛡️):\n"
        << "STEP / LINE                                         | MAY THROW? | list state\n"
        << "----------------------------------------------------+------------+-------------------------\n"
        << "Node* new_node = new Node(value, pos->next)         | yes        | unchanged (not linked)\n"
        << "pos->next_node = new_node                           | no         | changes only after ok\n"
        << "++size_                                              | no         | changes only after ok\n"
        << "(tail_ fix)                                          | no         | metadata only\n";
}

static void PrintStepTable_CopyAndSwap() {
    std::cout
        << "\n📊 STEP TABLE (copy-and-swap for operator= 🌀):\n"
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

    ui::Banner("Lesson 6_9 Task1 — FULL CHECKLIST 🎛️ (tail_ + contracts + review)");

    ui::Info("This program prints a detailed emoji report 🧾✨");
    ui::Info("Goal: demonstrate Lesson 6_9 Task1 + keep Lesson 5_9 features ✅");
    ui::Tip ("tail_ version avoids vector buffer (no double copy) 🚀");

    // --- 0) Empty sanity ---
    ui::Section("0) Empty list sanity 🧪");
    SingleLinkedList<int> empty;
    const auto& cempty = empty;

    ui::Check("0.1 empty.IsEmpty()", empty.IsEmpty(), true);
    ui::Check("0.2 empty.GetSize()==0", empty.GetSize() == 0, true);
    ui::Check("0.3 empty.begin()==empty.end()", empty.begin() == empty.end(), true, "empty range");

    ui::Check("0.4 before_begin()==cbefore_begin()",
              empty.before_begin() == empty.cbefore_begin(),
              true,
              "both on dummy-head");

    ui::Check("0.5 ++before_begin()==begin()",
              ++empty.before_begin() == empty.begin(),
              true,
              "dummy-head.next == first");

    ui::Check("0.6 ++cbefore_begin()==const begin()",
              ++empty.cbefore_begin() == cempty.begin(),
              true);

    // --- 0.N) [[nodiscard]] demo (INTENTIONAL WARNING) ---
    ui::Section("0.N) [[nodiscard]] demo 🚨 (intentional warning)");
    ui::Tip("The line below is INTENTIONAL (see README): it shows compile-time warning if return value is ignored.");
    [[maybe_unused]] auto it_ok = empty.begin();
    ui::Check("0.N1 begin() stored in [[maybe_unused]]", true, true, "no warning here");

    // Intentionally ignored return value => should trigger warning with -Wall/-Wextra:
    empty.begin(); // <--- INTENTIONAL WARNING DEMO

    ui::Warn("If you saw a compiler warning above — GOOD ✅ ([[nodiscard]] works).");

    // --- 1) PopFront + DeletionSpy ---
    ui::Section("1) PopFront 🧹 (noexcept) + DeletionSpy ☠️");
    {
        SingleLinkedList<int> numbers{3, 14, 15, 92, 6};
        PrintList(numbers, "1.0 numbers BEFORE");
        numbers.PopFront();
        PrintList(numbers, "1.1 numbers AFTER PopFront");

        ui::Check("1.2 numbers == {14,15,92,6}",
                  numbers == SingleLinkedList<int>{14, 15, 92, 6},
                  true);
    }
    {
        SingleLinkedList<DeletionSpy> list;
        list.PushFront(DeletionSpy{});
        int deletion_counter = 0;
        list.begin()->deletion_counter_ptr = &deletion_counter;

        ui::CheckEq("1.3 deletion_counter BEFORE", deletion_counter, 0);
        list.PopFront();
        ui::CheckEq("1.4 deletion_counter AFTER", deletion_counter, 1, "exactly one node destroyed");
    }

    // --- 2) before_begin / cbefore_begin on non-empty ---
    ui::Section("2) before_begin / cbefore_begin 🧭 (non-empty)");
    {
        SingleLinkedList<int> numbers{1, 2, 3, 4};
        const auto& cnumbers = numbers;

        ui::Check("2.1 before_begin()==cbefore_begin()",
                  numbers.before_begin() == numbers.cbefore_begin(), true);

        ui::Check("2.2 ++before_begin()==begin()",
                  ++numbers.before_begin() == numbers.begin(), true);

        ui::Check("2.3 ++cbefore_begin()==const begin()",
                  ++numbers.cbefore_begin() == cnumbers.begin(), true);
    }

    // --- 3) InsertAfter examples ---
    ui::Section("3) InsertAfter 🧷 (order + returned iterator)");
    {   // insert into empty: after before_begin
        SingleLinkedList<int> lst;
        auto inserted = lst.InsertAfter(lst.before_begin(), 123);
        PrintList(lst, "3.1 InsertAfter(before_begin, 123) on EMPTY");

        ui::Check("3.2 lst == {123}", lst == SingleLinkedList<int>{123}, true);
        ui::Check("3.3 inserted == begin", inserted == lst.begin(), true);
        ui::Check("3.4 *inserted == 123", *inserted == 123, true);
    }
    {   // insert into non-empty
        SingleLinkedList<int> lst{1, 2, 3};
        PrintList(lst, "3.5 lst BEFORE");

        auto inserted1 = lst.InsertAfter(lst.before_begin(), 123);
        PrintList(lst, "3.6 InsertAfter(before_begin, 123)");

        ui::Check("3.7 inserted1 == begin", inserted1 == lst.begin(), true);
        ui::Check("3.8 lst == {123,1,2,3}",
                  lst == SingleLinkedList<int>{123, 1, 2, 3}, true);

        auto inserted2 = lst.InsertAfter(lst.begin(), 555); // after 123
        PrintList(lst, "3.9 InsertAfter(begin, 555)  (after first element)");

        auto second_it = lst.begin();
        ++second_it;

        ui::Check("3.10 inserted2 == second element", inserted2 == second_it, true);
        ui::Check("3.11 *inserted2 == 555", *inserted2 == 555, true);
        ui::Check("3.12 lst == {123,555,1,2,3}",
                  lst == SingleLinkedList<int>{123, 555, 1, 2, 3}, true);
    }

    // --- 4) Strong exception guarantee ---
    ui::Section("4) Strong exception guarantee 🛡️ (InsertAfter + ThrowOnCopy)");
    {
        bool exception_was_thrown = false;

        for (int max_copy_counter = 10; max_copy_counter >= 0; --max_copy_counter) {
            SingleLinkedList<ThrowOnCopy> list{ThrowOnCopy{}, ThrowOnCopy{}, ThrowOnCopy{}};

            try {
                int copy_counter = max_copy_counter;
                list.InsertAfter(list.cbegin(), ThrowOnCopy(copy_counter));

                if (list.GetSize() != 4u) {
                    ui::Boom("Unexpected: size != 4 after successful insert 🤯");
                    assert(false);
                }
            } catch (const std::bad_alloc&) {
                exception_was_thrown = true;
                ui::Warn("Caught std::bad_alloc as planned 🎯");
                ui::Info("max_copy_counter=" + std::to_string(max_copy_counter));
                ui::CheckEq("4.2 list size after throw", list.GetSize(), size_t{3}, "must stay unchanged");
                ui::Check("4.3 strong guarantee holds", list.GetSize() == 3u, true);
                break;
            }
        }

        ui::Check("4.4 exception_was_thrown", exception_was_thrown, true);
        PrintStepTable_InsertAfter();
        ui::Tip("Key: list links new_node only AFTER successful allocation/copy ✅");
    }

    // --- 5) EraseAfter examples ---
    ui::Section("5) EraseAfter ✂️ (+ DeletionSpy)");
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        const auto& clst = lst;

        PrintList(lst, "5.0 lst BEFORE");
        auto after_erased = lst.EraseAfter(clst.cbefore_begin()); // remove first (1)
        PrintList(lst, "5.1 EraseAfter(cbefore_begin) => removed FIRST");

        ui::Check("5.2 lst == {2,3,4}", lst == SingleLinkedList<int>{2, 3, 4}, true);
        ui::Check("5.3 returned == begin", after_erased == lst.begin(), true);
    }
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        PrintList(lst, "5.4 lst BEFORE");

        auto after_erased = lst.EraseAfter(lst.cbegin()); // remove 2
        PrintList(lst, "5.5 EraseAfter(cbegin) => removed 2");

        auto expected_it = lst.begin();
        ++expected_it;

        ui::Check("5.6 lst == {1,3,4}", lst == SingleLinkedList<int>{1, 3, 4}, true);
        ui::Check("5.7 returned == ++begin", after_erased == expected_it, true);
    }
    {
        SingleLinkedList<int> lst{1, 2, 3, 4};
        PrintList(lst, "5.8 lst BEFORE");

        auto it = lst.cbegin();
        ++it; // 2
        ++it; // 3
        auto after_erased = lst.EraseAfter(it); // erase after 3 => remove 4 (last)
        PrintList(lst, "5.9 EraseAfter(it@3) => removed LAST");

        ui::Check("5.10 lst == {1,2,3}", lst == SingleLinkedList<int>{1, 2, 3}, true);
        ui::Check("5.11 returned == end()", after_erased == lst.end(), true);
    }
    {
        SingleLinkedList<DeletionSpy> list{DeletionSpy{}, DeletionSpy{}, DeletionSpy{}};

        auto second = list.begin();
        ++second;

        int deletion_counter = 0;
        second->deletion_counter_ptr = &deletion_counter;

        ui::CheckEq("5.12 deletion_counter BEFORE", deletion_counter, 0);
        list.EraseAfter(list.cbegin()); // erase 2nd element
        ui::CheckEq("5.13 deletion_counter AFTER", deletion_counter, 1, "erased node destroyed exactly once");
    }

    // --- 6) Lesson 5_9 features re-check ---
    ui::Section("6) Lesson 5_9 re-check 🔁 (comparisons / swap / copy / assign)");
    {
        using IntList = SingleLinkedList<int>;
        IntList a{1, 2, 3};
        IntList b{1, 2, 3};
        IntList c{1, 2, 3, 1};
        IntList d{1, 3};
        IntList e{1, 3, 0};

        ui::Check("6.1 a==b", a == b, true);
        ui::Check("6.2 a!=c", a != c, true);
        ui::Check("6.3 a<c",  a < c,  true);
        ui::Check("6.4 a<d",  a < d,  true);
        ui::Check("6.5 d>e",  d > e,  false, "d=[1,3], e=[1,3,0] => d<e");
        ui::Check("6.6 d<e",  d < e,  true);
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

        PrintList(first,  "6.7 first BEFORE swap");
        PrintList(second, "6.8 second BEFORE swap");

        first.swap(second);

        PrintList(first,  "6.9 first AFTER member swap");
        PrintList(second, "6.10 second AFTER member swap");

        ui::Check("6.11 second.begin()==old_first_begin", second.begin() == old_first_begin, true);
        ui::Check("6.12 first.begin()==old_second_begin", first.begin() == old_second_begin, true);
        ui::Check("6.13 sizes swapped",
                  (second.GetSize() == old_first_size && first.GetSize() == old_second_size),
                  true);

        ui::Sub("6.14 ADL free swap (no node copy) 🔄");
        {
            using std::swap;
            swap(first, second);

            ui::Check("first.begin()==old_first_begin", first.begin() == old_first_begin, true);
            ui::Check("second.begin()==old_second_begin", second.begin() == old_second_begin, true);
            ui::Check("sizes restored",
                      (first.GetSize() == old_first_size && second.GetSize() == old_second_size),
                      true);
        }
    }
    {
        SingleLinkedList<int> src{7, 8, 9};
        SingleLinkedList<int> copy(src);

        PrintList(src,  "6.15 src");
        PrintList(copy, "6.16 copy");

        ui::Check("6.17 copy==src", copy == src, true);
        ui::Check("6.18 deep copy: begin pointers differ", copy.begin() != src.begin(), true);

        SingleLinkedList<int> dst{1, 1, 1};
        dst = src;

        PrintList(dst, "6.19 dst AFTER operator=");
        ui::Check("6.20 dst==src", dst == src, true);
        ui::Check("6.21 dst.begin()!=src.begin", dst.begin() != src.begin(), true);

        PrintStepTable_CopyAndSwap();
        ui::Tip("copy-and-swap: swap() commits only if copying succeeded ✅");
    }

    // --- 7) Copy-count note ---
    ui::Section("7) Copy-count note 🧠 (buffer vs tail_)");
    ui::Info("Old PushFront-only range build with vector buffer:");
    std::cout
        << "   🧱 copy #1: push_back(*it) into vector\n"
        << "   🧱 copy #2: PushFront(*rit) into Node::value\n"
        << "   ⚠️ Up to TWO copies per element (+ possible realloc moves)\n\n";
    ui::Info("tail_ range build:");
    std::cout
        << "   🚀 copy #1: new Node(*it, nullptr) directly into Node::value\n"
        << "   ✅ ONE copy per element, same external behavior\n";

    ui::Banner("DONE 🏁");
    ui::Tip("If all checks are 🟢 — you're golden ✨");
    ui::Info("You demonstrated Lesson 6_9 Task1 + kept Lesson 5_9 features ✅");
    return 0;
}
