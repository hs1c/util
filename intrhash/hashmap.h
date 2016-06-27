#pragma once

#include "intrhash.h"
#include "nodeallc.h"

namespace intrhash_map_priv {
    template <class K, class T, class O, class A>
    struct impl {
        using value_type = std::pair<const K, T>;

        struct node_t
            : public value_type
            , public intrhash_item_t<node_t>
        {
            node_t(const value_type& value)
                : value_type(value)
            {}

            template <class _K>
            node_t(const _K& key)
                : value_type(key, T())
            {}
        };

        struct ops: public O {
            static const K& extract_key(const value_type& node) noexcept {
                return node.first;
            }

            static value_type& extract_value(node_t& node) noexcept {
                return node;
            }

            static const value_type& extract_value(const node_t& node) noexcept {
                return node;
            }
        };

        using allc_type = nodeallc_t<node_t, A>;
        using impl_type = intrhash_t<node_t, ops, A>;
    };
}

template <class K, class T, class O = generic_intrhash_ops, class A = std::allocator<T>>
class intrhash_map_t
    : private intrhash_map_priv::impl<K, T, O, A>::allc_type
    , private intrhash_map_priv::impl<K, T, O, A>::impl_type
{
private:
    using priv_impl = typename intrhash_map_priv::impl<K, T, O, A>;

    using allc_type = typename priv_impl::allc_type;
    using impl_type = typename priv_impl::impl_type;

    using item_type = typename impl_type::item_type;
    using node_type = typename impl_type::node_type;

public:
    using value_type = typename priv_impl::value_type;

public:
    using iterator = typename impl_type::iterator;
    using const_iterator = typename impl_type::const_iterator;

public:
    using allc_type::get_allocator;

    using impl_type::begin;
    using impl_type::end;
    using impl_type::cbegin;
    using impl_type::cend;

    using impl_type::find;
    using impl_type::has;
    using impl_type::equal_range;
    using impl_type::count;

    using impl_type::size;
    using impl_type::empty;

public:
    std::pair<iterator, bool> insert(const value_type& value) {
        return this->find_or_push(priv_impl::ops::extract_key(value), [this, &value](){ return this->new_node(value); });
    }

    size_t erase(iterator iter) {
        if (node_type *const node = this->pop(iter.node())) {
            this->delete_node(node);
            return 1;
        } else {
            return 0;
        }
    }

    template <class _K>
    size_t erase(const _K& key) {
        if (node_type* const node = this->pop_one(key)) {
            this->delete_node(node);
            return 1;
        } else {
            return 0;
        }
    }

    void clear() {
        this->decompose([this](node_type* node){ this->delete_node(node); });
    }

public:
    template <class _K>
    T& operator[](const _K& key) {
        return this->find_or_push(key, [this, &key](){ return this->new_node(key); }).first->second;
    }

public:
    intrhash_map_t() = default;

    explicit intrhash_map_t(size_t n)
        : impl_type(n)
    {}

    template <class X>
    explicit intrhash_map_t(size_t n, X&& allocator_param)
        : allc_type(std::forward<X>(allocator_param))
        , impl_type(n, allc_type::get_allocator())
    {}

    intrhash_map_t(intrhash_map_t&& right)
        : allc_type(std::move(right))
        , impl_type(std::move(right))
    {}

    intrhash_map_t(const intrhash_map_t& right)
        : allc_type(right)
        , impl_type(right, [this](const node_type* node){ return this->new_node(*node); })
    {}

    ~intrhash_map_t() noexcept {
        clear();
    }

public:
    void swap(intrhash_map_t& right) noexcept {
        static_cast<allc_type*>(this)->swap(right);
        static_cast<impl_type*>(this)->swap(right);
    }

    intrhash_map_t& operator=(const intrhash_map_t& right) {
        intrhash_map_t(right).swap(*this);
        return *this;
    }

    intrhash_map_t& operator=(intrhash_map_t&& right) {
        intrhash_map_t(std::move(right)).swap(*this);
        return *this;
    }
};

template <class K, class T, class O = generic_intrhash_ops, class A = std::allocator<T>>
class intrhash_multimap_t
    : private intrhash_map_priv::impl<K, T, O, A>::allc_type
    , private intrhash_map_priv::impl<K, T, O, A>::impl_type
{
private:
    using priv_impl = typename intrhash_map_priv::impl<K, T, O, A>;

    using allc_type = typename priv_impl::allc_type;
    using impl_type = typename priv_impl::impl_type;

    using item_type = typename impl_type::item_type;
    using node_type = typename impl_type::node_type;

public:
    using value_type = typename priv_impl::value_type;

public:
    using iterator = typename impl_type::iterator;
    using const_iterator = typename impl_type::const_iterator;

public:
    using allc_type::get_allocator;

    using impl_type::begin;
    using impl_type::end;
    using impl_type::cbegin;
    using impl_type::cend;

    using impl_type::find;
    using impl_type::has;
    using impl_type::equal_range;
    using impl_type::count;

    using impl_type::size;
    using impl_type::empty;

public:
    iterator insert(const value_type& value) {
        return {this->push(this->new_node(value))};
    }

    size_t erase(iterator iter) {
        if (node_type *const node = pop(iter.node())) {
            delete_node(node);
            return 1;
        } else {
            return 0;
        }
    }

    template <class _K>
    size_t erase(const _K& key) {
        size_t result = 0;
        this->pop_all(key, [this, &result](node_type* node){ this->delete_node(node); ++result; });
        return result;
    }

    void clear() {
        this->decompose([this](node_type* node){ this->delete_node(node); });
    }

public:
    intrhash_multimap_t() = default;

    explicit intrhash_multimap_t(size_t n)
        : impl_type(n)
    {}

    template <class X>
    explicit intrhash_multimap_t(size_t n, X&& allocator_param)
        : allc_type(std::forward<X>(allocator_param))
        , impl_type(n, allc_type::get_allocator())
    {}

    intrhash_multimap_t(intrhash_multimap_t&& right) noexcept
        : allc_type(std::move(right))
        , impl_type(std::move(right))
    {}

    intrhash_multimap_t(const intrhash_multimap_t& right)
        : allc_type(right)
        , impl_type(right, [this](const node_type* node){ return this->new_node(*node); })
    {}

    ~intrhash_multimap_t() noexcept {
        clear();
    }

public:
    void swap(intrhash_multimap_t& right) noexcept {
        static_cast<allc_type*>(this)->swap(right);
        static_cast<impl_type*>(this)->swap(right);
    }

    intrhash_multimap_t& operator=(const intrhash_multimap_t& right) {
        intrhash_multimap_t(right).swap(*this);
        return *this;
    }

    intrhash_multimap_t& operator=(intrhash_multimap_t&& right) noexcept {
        intrhash_multimap_t(std::move(right)).swap(*this);
        return *this;
    }
};
