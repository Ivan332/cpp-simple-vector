#pragma once

#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        :
        capacity_to_reserve_(capacity_to_reserve) {}

    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {

public:
    using Array = ArrayPtr<Type>;
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) {
        SimpleVector<Type> tmp(size, std::move(Type{}));
        this->swap(tmp);
    }


    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        size_ = size;
        capacity_ = size;
        Array tmp(size_);
        for (size_t i = 0; i < size_; i++)
        {
            tmp[i] = value;
        }
        array_.swap(tmp);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        Array array_temp(init.size());
        std::copy(init.begin(), init.end(), array_temp.Get());

        this->array_.swap(array_temp);

        size_ = init.size();
        capacity_ = init.size();
    }

    SimpleVector(const SimpleVector& other) {
        Array tmp(other.size_);
        size_ = other.size_;
        capacity_ = other.capacity_;
        std::copy(other.begin(), other.end(), &tmp[0]);
        array_.swap(tmp);
    }

    SimpleVector(SimpleVector&& other) {
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        array_.swap(other.array_);
    }

    SimpleVector(ReserveProxyObj reserved_vector) {
        Reserve(reserved_vector.capacity_to_reserve_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this == &rhs) {
            return *this;
        }
        if (!rhs.size_)
        {
            Clear();
            return *this;
        }
        SimpleVector<Type> temp(rhs);
        swap(temp);
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
            array_.swap(rhs.array_);
        }
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !size_;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return array_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return array_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_)
        {
            throw std::out_of_range("");
        }
        return array_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_)
        {
            throw std::out_of_range("");
        }
        return array_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_)
        {
            size_ = new_size;
            return;
        }

        if (new_size < capacity_)
        {
            Array tmp(size_ - new_size);
            std::move(&tmp[0], &tmp[size_ - new_size], &array_[size_]);
            size_ = std::move(new_size);
            return;
        }

        Array tmp(new_size);
        std::move(begin(), end(), &tmp[0]);
        this->array_.swap(tmp);
        size_ = std::move(new_size);
        capacity_ = std::move(new_size);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_)
        {
            Array tmp(new_capacity);
            std::copy(begin(), end(), tmp.Get());
            this->array_.swap(tmp);
            capacity_ = new_capacity;
            return;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ > size_) {
            array_[size_++] = item;
            return;
        }
        if (capacity_) {
            Array tmp(capacity_ *= 2);
            std::copy(begin(), end(), &tmp[0]);
            array_.swap(tmp);
            array_[size_++] = item;
        }
        else {
            Array tmp(++capacity_);
            std::copy(begin(), end(), &tmp[0]);
            array_.swap(tmp);
            array_[size_++] = item;
            return;
        }
    }

    void PushBack(Type&& item) {
        if (capacity_ > size_) {
            array_[size_++] = std::move(item);
            return;
        }
        if (capacity_) {
            Array tmp(capacity_ *= 2);
            std::move(begin(), end(), &tmp[0]);
            array_.swap(tmp);
            array_[size_++] = std::move(item);
        }
        else {
            Array tmp(++capacity_);
            std::move(begin(), end(), &tmp[0]);
            array_.swap(tmp);
            array_[size_++] = std::move(item);
            return;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        if (begin() > pos || end() < pos) {
            throw std::out_of_range("");
        }
        if (capacity_ == 0) {
            Array tmp(1);
            tmp[0] = value;
            array_.swap(tmp);
            capacity_ = 1;
            size_ = 1;
            return begin();
        }

        size_t new_capacity = size_ == capacity_ ? capacity_ * 2 : capacity_;
        Array tmp(new_capacity);
        Iterator pos_ = const_cast<Iterator>(pos);
        std::copy(begin(), pos_, tmp.Get());
        size_t dist = std::distance(begin(), pos_);
        tmp[dist] = value;
        std::copy_backward(pos_, end(), tmp.Get() + 1 + size_);
        array_.swap(tmp);
        ++size_;
        capacity_ = new_capacity;

        return begin() + dist;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        if (begin() > pos || end() < pos) {
            throw std::out_of_range("");
        }
        if (capacity_ == 0) {
            Array tmp(1);
            tmp[0] = std::move(value);
            array_.swap(tmp);
            capacity_ = 1;
            size_ = 1;
            return begin();
        }

        size_t new_capacity = size_ == capacity_ ? capacity_ * 2 : capacity_;
        Array tmp(new_capacity);
        Iterator pos_ = const_cast<Iterator>(pos);
        std::move(begin(), pos_, tmp.Get());
        size_t dist = std::distance(begin(), pos_);
        tmp[dist] = std::move(value);
        std::move_backward(pos_, end(), tmp.Get() + 1 + size_);
        array_.swap(tmp);
        ++size_;
        capacity_ = new_capacity;

        return begin() + dist;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(capacity_ != 0);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        if (begin() > pos || end() < pos) {
            throw std::out_of_range("");
        }
        size_t dist = std::distance(cbegin(), pos);
        Array tmp(capacity_);
        for (size_t i = 0; i < dist; ++i)
            tmp[i] = std::move(array_[i]);
        for (size_t i = dist + 1; i < size_; ++i)
            tmp[i - 1] = std::move(array_[i]);
        array_.swap(tmp);
        --size_;
        return begin() + dist;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        array_.swap(other.array_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return capacity_ != 0 ? &array_[0] : nullptr;
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return capacity_ != 0 ? &array_[size_] : nullptr;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return capacity_ != 0 ? &array_[0] : nullptr;
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return capacity_ != 0 ? &array_[size_] : nullptr;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return capacity_ != 0 ? &array_[0] : nullptr;
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return capacity_ != 0 ? &array_[size_] : nullptr;
    }

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    Array array_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs) && !(rhs < lhs);
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs < rhs || lhs == rhs;
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs > lhs || rhs == lhs;
}
