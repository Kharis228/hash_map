#pragma once

#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <memory>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    static constexpr size_t MIN_SIZE = 8;
    static constexpr double min_load_factor = 0;
    static constexpr double max_load_factor = 0.5;

    Hash hasher_;
    size_t elements_count_{0};

    using PairType = std::pair<KeyType, ValueType>;
    using PairPointer = std::unique_ptr<PairType>;
    std::vector<PairPointer> arr_;

    size_t gen_place(const KeyType &el) const {
        return hasher_(el) % arr_.size();
    }

    void resize(size_t new_size) {
        auto temp_arr = std::move(arr_);
        elements_count_ = 0;
        arr_.resize(new_size);
        for (auto &ptr: temp_arr) {
            if (ptr) {
                insert(std::move(*ptr));
                ptr.reset();
            }
        }
    }
    void try_extend() {
        if ((double) elements_count_ / arr_.size() >= max_load_factor)
            resize(2 * arr_.size());
    }
    void try_shrink() {
        if ((double) elements_count_ / arr_.size() <= min_load_factor && arr_.size() > MIN_SIZE)
            resize(arr_.size() / 2);
    }

    size_t nextIndex(size_t a) const {
        if (++a == arr_.size())
            a = 0;
        return a;
    }

    size_t distance(size_t beg, size_t end) const {
        if (end >= beg)
            return end - beg;
        return arr_.size() - beg + end;
    }

    size_t first_non_empty(size_t a) const {
        while (a != arr_.size() && !arr_[a]) {
            ++a;
        }
        return a;
    }

public:
    HashMap(Hash h = Hash()) : hasher_(h), arr_(MIN_SIZE) {
    };

    HashMap(const HashMap &a, Hash h = Hash()) : hasher_(h), arr_(MIN_SIZE) {
        for (auto &ptr: a.arr_) {
            if (ptr) {
                insert(*ptr);
            }
        }
    }

    template<class it>
    HashMap(it beg, it end, Hash h = Hash()) : hasher_(h), arr_(MIN_SIZE) {
        for (auto i = beg; i != end; ++i)
            insert(*i);
    }

    HashMap(std::initializer_list<PairType> arr, Hash h = Hash()) : hasher_(h), arr_(MIN_SIZE) {
        for (auto i = arr.begin(); i != arr.end(); ++i) {
            insert(*i);
        }
    }

    std::size_t size() const {
        return elements_count_;
    }

    bool empty() const {
        return elements_count_ == 0;
    }

    auto hash_function() const {
        return hasher_;
    }

    void insert(PairType el) {
        try_extend();

        size_t target = gen_place(el.first);
        size_t current = target;
        while (true) {
            auto &ptr = arr_[current];
            if (ptr) {
                PairType &o = *ptr;
                if (hasher_(o.first) == hasher_(el.first) && o.first == el.first) {
                    return;
                }

                size_t o_target = gen_place(o.first);
                if (distance(o_target, current) < distance(target, current)) {
                    std::swap(o, el);
                    target = o_target;
                }
                current = nextIndex(current);
            } else {
                ptr = std::make_unique<PairType>(el);
                ++elements_count_;
                break;
            }
        }
    }

    void erase(const KeyType &key) {
        try_shrink();
        size_t current = find(key).pos;
        if (current >= arr_.size())
            return;
        while (true) {
            size_t IndexNextDoor = nextIndex(current);
            auto &boyNextDoor = arr_[IndexNextDoor];
            if (boyNextDoor && gen_place(boyNextDoor->first) != IndexNextDoor) {
                std::swap(arr_[current], boyNextDoor);
                current = IndexNextDoor;
            } else {
                arr_[current].reset();
                --elements_count_;
                return;
            }
        }
    }

    class const_iterator {
    public:
        using pointer = const typename std::pair<const KeyType, ValueType> *;
        using reference = const typename std::pair<const KeyType, ValueType> &;

        const_iterator() = default;

        const_iterator(const HashMap<KeyType, ValueType, Hash> *map, size_t pos) : map(map), pos(pos) {}

        reference operator*() const { return *operator->(); }

        pointer operator->() const { return (pointer) (map->arr_[pos]).get(); }

        const_iterator &operator++() {
            ++pos;
            pos = map->first_non_empty(pos);
            return *this;
        }

        const_iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator &b) { return map == b.map && pos == b.pos; }

        bool operator!=(const const_iterator &b) { return !(*this == b); }

    private:
        const HashMap<KeyType, ValueType, Hash> *map = nullptr;
        size_t pos = 0;
    };

    class iterator {
    public:
        using pointer = typename std::pair<const KeyType, ValueType> *;
        using reference = typename std::pair<const KeyType, ValueType> &;

        iterator() = default;

        iterator(HashMap<KeyType, ValueType, Hash> *map, size_t pos) : map(map), pos(pos) {}

        reference operator*() const { return *operator->(); }

        pointer operator->() const { return (pointer) (map->arr_[pos]).get(); }

        iterator &operator++() {
            ++pos;
            pos = map->first_non_empty(pos);
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        operator const_iterator() { return const_iterator(map, pos); }

        bool operator==(const iterator &b) { return map == b.map && pos == b.pos; }

        bool operator!=(const iterator &b) { return !(*this == b); }

    private:
        HashMap<KeyType, ValueType, Hash> *map = nullptr;
        size_t pos = 0;

        friend void HashMap::erase(const KeyType &key);
    };

    iterator begin() {
        if (empty())
            return end();
        return iterator(this, first_non_empty(0));
    }

    iterator end() { return iterator(this, arr_.size()); }

    const_iterator begin() const {
        if (empty())
            return end();
        return const_iterator(this, first_non_empty(0));
    }

    const_iterator end() const { return const_iterator(this, arr_.size()); }

    iterator find(const KeyType &el) {
        size_t current = gen_place(el);
        while (true) {
            auto &ptr = arr_[current];
            if (ptr) {
                if (hasher_(ptr->first) == hasher_(el) && ptr->first == el)
                    return iterator(this, current);
                current = nextIndex(current);
            } else {
                return end();
            }
        }
    }

    const_iterator find(const KeyType &el) const {
        size_t current = gen_place(el);
        while (true) {
            auto &ptr = arr_[current];
            if (ptr) {
                if (hasher_(ptr->first) == hasher_(el) && ptr->first == el)
                    return const_iterator(this, current);
                current = nextIndex(current);
            } else {
                return end();
            }
        }
    }

    ValueType &operator[](const KeyType &el) {
        iterator it = find(el);
        if (it == end()) {
            insert({el, ValueType()});
            it = find(el);
        }
        return (*it).second;
    }

    const ValueType &at(const KeyType &el) const {
        const_iterator it = find(el);
        if (it == end()) {
            throw std::out_of_range("key nema");
        }
        return (*it).second;
    }

    void clear() {
        elements_count_ = 0;
        for (PairPointer &ptr: arr_)
            ptr.reset();
        arr_.resize(MIN_SIZE);
    }

    HashMap &operator=(const HashMap &a) {
        if (arr_ == a.arr_)
            return *this;
        arr_.clear();
        arr_.resize(a.arr_.size());
        elements_count_ = a.elements_count_;
        for (size_t i = 0; i < arr_.size(); ++i) {
            if (a.arr_[i]) {
                arr_[i] = std::make_unique<PairType>(*a.arr_[i]);
            }
        }
        return *this;
    }
};
