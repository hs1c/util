#pragma once

#include "intrhash.h"
#include "nodeallc.h"

namespace intrhash_set_priv {
    template <class T, class O, class A>
    struct impl {
        using value_type = T;

        struct node_t
            : public intrhash_item_t<node_t>
        {
            node_t(const value_type& value)
                : value_(value)
            {}

            const value_type value_;
        };

        struct ops
            : public O
        {
            static const value_type& extract_key(const node_t& node) noexcept {
                return node.value_;
            }

            static const value_type& extract_value(node_t& node) noexcept {
                return node.value_;
            }

            static const value_type& extract_value(const node_t& node) noexcept {
                return node.value_;
            }
        };

        using allc_type = nodeallc_t<node_t, A>;
        using impl_type = intrhash_t<node_t, opts_t, A>;
    };
}

template <class T, class O = generic_intrhash_ops, class A = std::allocator<T>>
class intrhash_set_t
    : private intrhash_set_priv::impl<T, O, A>::allc_type
    , private intrhash_set_priv::impl<T, O, A>::impl_type
{
private:
    using priv_impl = typename intrhash_set_priv::impl<T, O, A>;

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
        return this->find_or_push(value, [this, &value](){ return this->new_node(value); });
    }

    size_t erase(iterator iter) {
        if (node_type *const node = pop(iter.node())) {
            this->delete_node(node);
            return 1;
        } else {
            return 0;
        }
    }

    template <class K>
    size_t erase(const K& key) {
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
    intrhash_set_t() = default;

    explicit intrhash_set_t(size_t n)
        : impl_type(n)
    {}

    template <class X>
    explicit intrhash_set_t(size_t n, X&& allocator_param)
        : allc_type(std::forward<X>(allocator_param))
        , impl_type(n, allc_type::get_allocator())
    {}

    intrhash_set_t(intrhash_set_t&& right)
        : allc_type(std::move(right))
        , impl_type(std::move(right))
    {}

    intrhash_set_t(const intrhash_set_t& right)
        : allc_type(right)
        , impl_type(right, [this](const node_type* node){ return this->new_node(*node); })
    {}

    ~intrhash_set_t() noexcept {
        clear();
    }

public:
    void swap(intrhash_set_t& right) noexcept {
        static_cast<allc_type*>(this)->swap(right);
        static_cast<impl_type*>(this)->swap(right);
    }

    intrhash_set_t& operator=(const intrhash_set_t& right) {
        intrhash_set_t(right).swap(*this);
        return *this;
    }

    intrhash_set_t& operator=(intrhash_set_t&& right) {
        intrhash_set_t(std::move(right)).swap(*this);
        return *this;
    }
};

template <class T, class O = common_intrhash_ops, class A = std::allocator<T>>
class intrhash_multiset_t
    : private intrhash_set_priv::impl<T, O, A>::allc_type
    , private intrhash_set_priv::impl<T, O, A>::impl_type
{
private:
    using priv_impl = typename intrhash_set_priv::impl<T, O, A>;

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
        return this->push(this->new_node(value));
    }

    size_t erase(iterator iter) {
        if (node_type *const node = pop(iter.node())) {
            this->delete_node(node);
            return 1;
        } else {
            return 0;
        }
    }

    template <class K>
    size_t erase(const K& key) {
        size_t result = 0;
        this->pop_all(key, [this, &result](node_type* node){ this->delete_node(node); ++result; });
        return result;
    }

    void clear() {
        this->decompose([this](node_type* node){ this->delete_node(node); });
    }

public:
    intrhash_multiset_t() = default;

    explicit intrhash_multiset_t(size_t n)
        : impl_type(n)
    {}

    explicit intrhash_multiset_t(const A& allocator)
        : intrhash_multiset_t(0, allocator)
    {}

    template <class X>
    explicit intrhash_multiset_t(size_t n, X&& allocator_param)
        : allc_type(std::forward<X>(allocator_param))
        , impl_type(n, allc_type::get_allocator())
    {}

    intrhash_multiset_t(intrhash_multiset_t&& right) noexcept
        : allc_type(std::move(right))
        , impl_type(std::move(right))
    {}

    intrhash_multiset_t(const intrhash_multiset_t& right)
        : allc_type(right)
        , impl_type(right, [this](const node_type* node){ return this->new_node(*node); })
    {}

    ~intrhash_multiset_t() noexcept {
        clear();
    }

public:
    void swap(intrhash_multiset_t& right) noexcept {
        static_cast<allc_type*>(this)->swap(right);
        static_cast<impl_type*>(this)->swap(right);
    }

    intrhash_multiset_t& operator=(const intrhash_multiset_t& right) {
        intrhash_multiset_t(right).swap(*this);
        return *this;
    }

    intrhash_multiset_t& operator=(intrhash_multiset_t&& right) noexcept {
        intrhash_multiset_t(std::move(right)).swap(*this);
        return *this;
    }
};
