C++ • Data Structures • Singly Linked List • Iterators • Exception Safety
---

# 🧩 Singly Linked List — Insert / Erase (Lesson 6_9)

## 🚨 Демонстрация `[[nodiscard]]` + разбор логов

---

## 1️⃣ Включаем демонстрацию `[[nodiscard]]`

В блоке **`0.N) [[nodiscard]] demo`** меняем:

```cpp
#if 0
    empty.begin();
#endif
```

на:

```cpp
#if 1
    empty.begin(); // намеренно игнорируем результат
#endif
```

---

## 2️⃣ Сборка и ожидаемое предупреждение (compile-time) ⚙️

Собираем проект:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic task1_lesson6_9_full_logs_tail_review.cpp -o task6_logs
```

Компилятор выводит предупреждение:

```text
warning: ignoring return value of
'SingleLinkedList<Type>::Iterator SingleLinkedList<Type>::begin()',
declared with attribute 'nodiscard' [-Wunused-result]
```

### 🔍 Что это значит

* `begin()` возвращает **итератор** и помечен `[[nodiscard]]`
* строка `empty.begin();` **выбрасывает результат**
* GCC предупреждает: *ignoring return value … nodiscard*

⚠️ Это **НЕ ошибка**, а демонстрация защиты API:
компилятор помогает поймать ситуацию, когда программист **забыл использовать итератор**.

👉 На выполнение программы warning **не влияет**.

---

## 3️⃣ Запуск программы и разбор результата ▶️

Запускаем:

```bash
task6_logs
```

Программа выводит **чеклист**, где каждый блок подтверждает корректность контейнера.

---

## 🟦 Блок 0) Empty list sanity — пустой список

* `IsEmpty -> true` — список пуст
* `size == 0` — размер корректный
* `begin == end` — у пустого контейнера нет элементов
* `before_begin == cbefore_begin` — оба указывают на dummy-head
* `++before_begin == begin` — корректная навигация
* `++cbefore_begin == const begin` — то же для const

✅ **Итог:** итераторы и dummy-узел работают корректно даже для пустого списка.

---

## 🟨 Блок 0.N) `[[nodiscard]]` demo — логовая часть

* `0.N1 stored begin() in [[maybe_unused]] variable -> OK`
  Итератор сохранён → корректный вариант
* `0.N2 ...`
  Напоминание, что warning появляется при включении `empty.begin()`

ℹ️ Сам warning появляется **на этапе компиляции**, а не во время запуска.

---

## 🟩 Блок 1) `PopFront()` + `DeletionSpy`

* Было: `[3 14 15 92 6]`
* После `PopFront()`: `[14 15 92 6]`
* Проверка равенства → `true`

### 🧪 DeletionSpy

* до удаления: `deletion_counter = 0`
* после удаления: `deletion_counter = 1`

✅ Деструктор вызван **ровно один раз**, утечек нет.

---

## 🟦 Блок 2) `before_begin / cbefore_begin` (непустой список)

* `before_begin == cbefore_begin`
* `++before_begin == begin`

✅ Можно безопасно делать `insert / erase` в голову списка.

---

## 🟪 Блок 3) `InsertAfter` — вставка элементов

### 🔹 Вставка в пустой список

`InsertAfter(before_begin, 123)` → `{123}`
Возвращаемый итератор:

* равен `begin()`
* `*it == 123`

### 🔹 Вставка в непустой список

`{1,2,3}` → `{123,1,2,3}` → `{123,555,1,2,3}`

✅ Все `expect true` подтверждают:

* правильный порядок,
* корректные возвращаемые итераторы.

---

## 🟥 Блок 4) Strong Exception Guarantee (`ThrowOnCopy`)

Ключевые строки:

* пойман `std::bad_alloc`
* `list size after throw = 3`
* `strong guarantee holds -> true`

### 💡 Смысл

* сначала создаётся `new Node(...)` (может бросить)
* только потом узел привязывается к списку

Если происходит исключение — **список остаётся неизменным**.

---

## 🟫 Блок 5) `EraseAfter` — удаление элементов

* удаление первого → `{2,3,4}`, возвращается `begin()`
* удаление середины → `{1,3,4}`, возвращается `++begin`
* удаление последнего → `{1,2,3}`, возвращается `end()`

`DeletionSpy` снова подтверждает **1 вызов деструктора**.

---

## 🟦 Блок 6) Проверка Lesson 5_9

### 🔹 Comparisons

`==`, `!=`, `<`, `>`, `<=`, `>=` работают корректно.

### 🔹 Swap

* swap **не копирует узлы**
* итераторы “переезжают” вместе со списками
* ADL `swap()` работает

### 🔹 Copy / Assignment

* copy ctor → **глубокая копия**
* `operator=` через copy-and-swap → strong guarantee

---

## 🟩 Блок 7) Почему `tail_` build лучше

* старый способ: **2 копии на элемент**
* `tail_` build:

  ```cpp
  new Node(*it, nullptr)
  ```

  → **1 копия**, без буферов

---

## ✅ Финальный вывод

```text
If all 'expect true' checks match, you demonstrated Lesson 6_9 Task1 + kept 5_9 features.
```

🎉 Это означает:

* все требования Lesson 6_9 выполнены
* функциональность Lesson 5_9 не сломана
* реализация корректна и безопасна

---
