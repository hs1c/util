#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace intrhash_util {
    template <class T> T declret() noexcept;

    namespace select_type_priv {
        template <bool X, class T1, class T2>
        struct select_type_impl {
            using result_type = T1;
        };

        template <class T1, class T2>
        struct select_type_impl<false, T1, T2> {
            using result_type = T2;
        };
    }

    template <bool X, class T1, class T2>
    using select_type = typename select_type_priv::select_type_impl<X, T1, T2>::result_type;

    struct delete_ops {
        template <class T>
        static void destroy(T* t) noexcept {
            delete t;
        }

        static void destroy(void* t) noexcept {
            ::operator delete(t);
        }
    };
}

namespace intrhash_priv {
    static size_t buckets_count(size_t n) noexcept {
        static const size_t primes[] = {
            7ul, 17ul, 29ul, 53ul, 97ul,
            193ul, 389ul, 769ul, 1543ul, 3079ul,
            6151ul, 12289ul, 24593ul, 49157ul, 98317ul,
            196613ul, 393241ul, 786433ul, 1572869ul, 3145739ul,
            6291469ul, 12582917ul, 25165843ul, 50331653ul, 100663319ul,
            201326611ul, 402653189ul, 805306457ul, 1610612741ul, 3221225473ul, 4294967291ul,
        };

        static const size_t* const first_prime = primes;
        static const size_t* const last_prime = first_prime + sizeof(primes) / sizeof(*primes) - 1;

        if (n <= *first_prime) {
            return *first_prime;
        } else {
            return *std::lower_bound(first_prime, last_prime, n);
        }
    }
}

namespace oneshot_vector {
    template <class T, class V>
    class vector_ops {
    private:
        V* this_() noexcept {
            return static_cast<V*>(this);
        }

        const V* this_() const noexcept {
            return static_cast<const V*>(this);
        }

    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using iterator = pointer;
        using const_iterator = const_pointer;

    public:
        iterator begin() noexcept {
            return this_()->first();
        }

        iterator end() noexcept {
            return this_()->last();
        }

        const_iterator begin() const noexcept {
            return this_()->first();
        }

        const_iterator end() const noexcept {
            return this_()->last();
        }

        size_t size() const noexcept {
            return std::distance(begin(), end());
        }

        bool empty() const noexcept {
            return begin() == end();
        }

        reference operator[](size_t n) noexcept {
            return *(begin() + n);
        }

        const_reference operator[](size_t n) const noexcept {
            return *(begin() + n);
        }
    };

    template <class T, class A = std::allocator<T>>
    class oneshot_vector_t
        : public vector_ops<T, oneshot_vector_t<T>>
    {
    private:
        using value_type = T;
        using allocator_type = typename A::template rebind<value_type>::other;

    public:
        oneshot_vector_t() = default;

        explicit oneshot_vector_t(size_t n)
            : oneshot_vector_t(n, allocator_type())
        {}

        template <class X>
        oneshot_vector_t(size_t n, X&& allocator_param)
            : allocator_(std::forward<X>(allocator_param))
        {
            if (n) {
                first_ = allocator_.allocate(n);
                last_ = first_ + n;

                try {
                    new (first_) value_type[n];
                } catch (...) {
                    allocator_.deallocate(first_, n);
                    throw;
                }
            }
        }

        oneshot_vector_t(oneshot_vector_t&& right) noexcept
            : allocator_(std::move(right.allocator_))
            , first_(right.first_)
            , last_(right.last_)
        {
            right.last_ = right.first_ = nullptr;
        }

        ~oneshot_vector_t() noexcept {
            if (first_) {
                auto iter = last_;

                do {
                    (--iter)->~value_type();
                } while (iter != first_);

                allocator_.deallocate(first_, last_ - first_);
            }
        }

    public:
        oneshot_vector_t& operator=(oneshot_vector_t&& right) noexcept {
            oneshot_vector_t(std::move(right)).swap(*this);
            return *this;
        }

        void swap(oneshot_vector_t& right) noexcept {
            std::swap(allocator_, right.allocator_);
            std::swap(first_, right.first_);
            std::swap(last_, right.last_);
        }

    public:
        value_type* first() const noexcept {
            return first_;
        }

        value_type* last() const noexcept {
            return last_;
        }

    public:
        const allocator_type& get_allocator() const noexcept {
            return allocator_;
        }

    private:
        oneshot_vector_t(const oneshot_vector_t&) = delete;
        oneshot_vector_t& operator=(const oneshot_vector_t&) = delete;

    private:
        allocator_type allocator_;

        value_type* first_ = nullptr;
        value_type* last_ = nullptr;
    };
}

template <class T>
class intrhash_item_t {
    template <class, class, class> friend class intrhash_t;

public:
    bool linked() const noexcept {
        return next_;
    }

private:
    using node_type = T;
    using item_type = intrhash_item_t<node_type>;

    node_type* node() noexcept {
        return static_cast<node_type*>(this);
    }

    const node_type* node() const noexcept {
        return static_cast<const node_type*>(this);
    }

    item_type* next() noexcept {
        return next_;
    }

    const item_type* next() const noexcept {
        return next_;
    }

    item_type** next_ptr() noexcept {
        return &next_;
    }

    const item_type* const* next_ptr() const noexcept {
        return &next_;
    }

    void set_next(item_type* _next) noexcept {
        next_ = _next;
    }

private:
    item_type* next_ = nullptr;
};

struct generic_intrhash_ops {
    template <class K>
    static size_t hash(const K& key) {
        return std::hash<K>()(key);
    }

    template <class T1, class T2>
    static bool equal_to(const T1& first, const T2& second) {
        return first == second;
    }

    template <class T>
    static T& extract_value(T& node) noexcept {
        return node;
    }
};

template <class T, class O, class A = std::allocator<T>>
class intrhash_t {
protected:
    using item_type = intrhash_item_t<T>;
    using node_type = typename item_type::node_type;

    using allocator_type = A;

private:
    template <bool X>
    class context_base_t {
    public:
        using item_type = intrhash_util::select_type<X, const typename intrhash_t::item_type, typename intrhash_t::item_type>;
        using node_type = intrhash_util::select_type<X, const typename intrhash_t::node_type, typename intrhash_t::node_type>;

        using item_ptr = intrhash_util::select_type<X, item_type* const, item_type*>;

        template <bool _X>
        context_base_t(const context_base_t<_X>& right) noexcept
            : ptr_(right.ptr())
        {}

        context_base_t(item_ptr* _ptr) noexcept
            : ptr_(_ptr)
        {}

        item_ptr* ptr() const noexcept {
            return ptr_;
        }

        item_type* item() const noexcept {
            return *ptr_;
        }

        node_type* node() const noexcept {
            return item()->node();
        }

    private:
        item_ptr* ptr_ = nullptr;
    };

    using context_type = context_base_t<false>;
    using const_context_type = context_base_t<true>;

private:
    static constexpr uintptr_t bucket_flag_ = 1ul;

    template <class C>
    static bool ctx_bucket_(C ctx) noexcept {
        return reinterpret_cast<uintptr_t>(*ctx.ptr()) & bucket_flag_;
    }

    template <class C>
    static bool ctx_item_(C ctx) noexcept {
        return !ctx_bucket_(ctx);
    }

    template <class C>
    static C next_ctx_bucket_(C ctx) noexcept {
        return {reinterpret_cast<typename C::item_ptr*>(reinterpret_cast<uintptr_t>(*ctx.ptr()) & ~bucket_flag_)};
    }

    template <class C>
    static C next_ctx_item_(C ctx) noexcept {
        return {ctx.item()->next_ptr()};
    }

    template <class C>
    static C item_ctx_(C ctx) noexcept {
        while (ctx_bucket_(ctx)) {
            ctx = next_ctx_bucket_(ctx);
        }
        return ctx;
    }

    template <class C, class K>
    static bool ctx_relative_(C ctx, const K& key) noexcept {
        return O::equal_to(O::extract_key(*ctx.node()), key);
    }

    static void push_item_(context_type ctx, item_type* item) noexcept {
        item->set_next(ctx.item());
        *ctx.ptr() = item;
    }

    static item_type* pop_item_(context_type ctx) noexcept {
        item_type* const item = ctx.item();
        *ctx.ptr() = item->next();
        item->set_next(nullptr);
        return item;
    }

private:
    using buckets_type = oneshot_vector::oneshot_vector_t<item_type*, A>;

    static void init_buckets_(buckets_type* bkts) noexcept {
        if (!bkts->empty()) {
            auto bucket = bkts->begin();
            auto last = bkts->end();

            --last;

            while (bucket != last) {
                const auto current = bucket;
                *current = reinterpret_cast<item_type*>(reinterpret_cast<uintptr_t>(&*++bucket) | bucket_flag_);
            }

            *bucket = nullptr;
        }
    }

private:
    template <class C, class B, class K>
    static C base_ctx_(B& bkts, const K& key) noexcept {
        return &bkts[(O::hash(key) % (bkts.size() - 1))];
    }

    template <class K>
    context_type base_ctx_(const K& key) noexcept {
        return base_ctx_<context_type>(buckets_, key);
    }

    template <class K>
    const_context_type base_ctx_(const K& key) const noexcept {
        return base_ctx_<const_context_type>(buckets_, key);
    }

    template <class C, class B, class K>
    static std::pair<C, bool> find_ctx_(B& bkts, const K& key) noexcept {
        C ctx = base_ctx_<C>(bkts, key);

        for (; ctx_item_(ctx); ctx = next_ctx_item_(ctx)) {
            if (ctx_relative_(ctx, key)) {
                return {ctx, true};
            }
        }

        return {ctx, false};
    }

    template <class K>
    std::pair<context_type, bool> find_ctx_(const K& key) noexcept {
        return find_ctx_<context_type>(buckets_, key);
    }

    template <class K>
    std::pair<const_context_type, bool> find_ctx_(const K& key) const noexcept {
        return find_ctx_<const_context_type>(buckets_, key);
    }

    template <class I, class X, class K>
    static std::pair<I, I> equal_range_impl_(X* ths, const K& key) noexcept {
        const auto found_ctx = ths->find_ctx_(key);

        if (!found_ctx.second) {
            return {ths->end(), ths->end()};
        }

        auto last = found_ctx.first;

        do {
            last = next_ctx_item_(last);
        } while (ctx_item_(last) && ctx_relative_(last, key));

        return {found_ctx.first.item(), item_ctx_(last).item()};
    }

private:
    template <bool X>
    class iterator_base_t {
    private:
        using context_type = context_base_t<X>;

        using item_type = typename context_type::item_type;
        using node_type = typename context_type::node_type;

    public:
        using value_type = typename std::remove_reference<decltype(O::extract_value(intrhash_util::declret<node_type&>()))>::type;

        using reference = value_type&;
        using pointer = value_type*;

        // using iterator_category = typename std::forward_iterator_tag;
        // using difference_type = typename std::ptrdiff_t;

        typedef typename std::forward_iterator_tag iterator_category;
        typedef typename std::ptrdiff_t difference_type;

    public:
        iterator_base_t() = default;

        template <bool _X>
        iterator_base_t(const iterator_base_t<_X>& right) noexcept
            : item_(right.item())
        {}

        iterator_base_t(item_type* _item) noexcept
            : item_(_item)
        {}

        item_type* item() const noexcept {
            return item_;
        }

        node_type* node() const noexcept {
            return item_->node();
        }

        void next() noexcept {
            item_ = item_ctx_(context_type(item_->next_ptr())).item();
        }

    public:
        iterator_base_t& operator=(const iterator_base_t& right) noexcept {
            item_ = right.item_;
            return *this;
        }

        value_type* operator->() const noexcept {
            return &O::extract_value(*node());
        }

        value_type& operator*() const noexcept {
            return O::extract_value(*node());
        }

        template <bool _X>
        bool operator==(const iterator_base_t<_X>& right) const noexcept {
            return item() == right.item();
        }

        template <bool _X>
        bool operator!=(const iterator_base_t<_X>& right) const noexcept {
            return item() != right.item();
        }

        iterator_base_t operator++() noexcept {
            next();
            return *this;
        }

        iterator_base_t operator++(int) noexcept {
            const iterator_base_t iter(*this);
            next();
            return iter;
        }

    private:
        item_type* item_;
    };

public:
    using iterator = iterator_base_t<false>;
    using const_iterator = iterator_base_t<true>;

public:
    iterator begin() noexcept {
        return {item_ctx_<context_type>(&*buckets_.begin()).item()};
    }

    iterator end() noexcept {
        return {*(buckets_.end() - 1)};
    }

    const_iterator begin() const noexcept {
        return {item_ctx_<const_context_type>(&*buckets_.begin()).item()};
    }

    const_iterator end() const noexcept {
        return {*(buckets_.end() - 1)};
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator cend() const noexcept {
        return end();
    }

public:
    template <class F>
    void decompose(F&& cbk) {
        if (nitems_) {
            for (context_type ctx = &*buckets_.begin(), end_ctx = &*(buckets_.end() - 1); ctx.ptr() != end_ctx.ptr();) {
                if (ctx_item_(ctx)) {
                    --nitems_;
                    cbk(pop_item_(ctx)->node());
                } else {
                    ctx = next_ctx_bucket_(ctx);
                }
            }
        }
    }

    void decompose() noexcept {
        decompose([](node_type*){});
    }

    void resize(size_t n) {
        if (n > buckets_.size() - 1) {
            const size_t nbuckets = intrhash_priv::buckets_count(n) + 1;

            if (nbuckets > buckets_.size()) {
                buckets_type buckets(nbuckets, buckets_.get_allocator());
                size_t nitems = 0;

                init_buckets_(&buckets);
                decompose([this, &buckets, &nitems](item_type* item){
                    push_item_(base_ctx_<context_type>(buckets, O::extract_key(*item->node())), item);
                    ++nitems;
                });
                buckets_.swap(buckets);
                nitems_ = nitems;
            }
        }
    }

public:
    template <class K>
    iterator find(const K& key) noexcept {
        const auto found_ctx = find_ctx_(key);
        return found_ctx.second ? iterator(found_ctx.first.item()) : end();
    }

    template <class K>
    const_iterator find(const K& key) const noexcept {
        const auto found_ctx = find_ctx_(key);
        return found_ctx.second ? const_iterator(found_ctx.first.item()) : end();
    }

    template <class K>
    node_type* find_ptr(const K& key) noexcept {
        const auto found_ctx = find_ctx_(key);
        return found_ctx.second ? found_ctx.first.node() : nullptr;
    }

    template <class K>
    const node_type* find_ptr(const K& key) const noexcept {
        const auto found_ctx = find_ctx_(key);
        return found_ctx.second ? found_ctx.first.node() : nullptr;
    }

    template <class K>
    bool has(const K& key) const noexcept {
        return find_ctx_(key).second;
    }

    template <class K>
    std::pair<iterator, iterator> equal_range(const K& key) noexcept {
        return equal_range_impl_<iterator>(this, key);
    }

    template <class K>
    std::pair<const_iterator, const_iterator> equal_range(const K& key) const noexcept {
        return equal_range_impl_<const_iterator>(this, key);
    }

    template <class K>
    size_t count(const K& key) const noexcept {
        const auto found_ctx = find_ctx_(key);

        if (!found_ctx.second) {
            return 0;
        }

        size_t result = 0;

        do {
            ++result;
            found_ctx.first = next_ctx_item_(found_ctx.first);
        } while (ctx_item_(found_ctx.first) && ctx_relative_(found_ctx.first, key));

        return result;
    }

public:
    size_t size() const noexcept {
        return nitems_;
    }

    bool empty() const noexcept {
        return !size();
    }

public:
    node_type* push_no_resize(node_type* node) noexcept {
        const auto ctx = find_ctx_(O::extract_key(*node)).first;
        push_item_(ctx, node);
        ++nitems_;
        return ctx.node();
    }

    node_type* push(node_type* node) noexcept {
        resize(nitems_ + 1);
        return push_no_resize(node);
    }

    node_type* pop(node_type* node) noexcept {
        for (auto ctx = base_ctx_(O::extract_key(*node)); ctx_item_(ctx); ctx = next_ctx_item_(ctx)) {
            if (ctx.node() == node) {
                --nitems_;
                return pop_item_(ctx)->node();
            }
        }

        return nullptr;
    }

    node_type* pop(iterator iter) noexcept {
        return pop(iter.node());
    }

    template <class F>
    void pop(node_type* first, node_type* last, F&& cbk) {
        for (auto ctx = base_ctx_(O::extract_key(*first)); ctx_item_(ctx); ctx = next_ctx_item_(ctx)) {
            if (ctx.node() == first) {
                const context_type end_ctx = &*(buckets_.end() - 1);

                do {
                    if (ctx_item_(ctx)) {
                        if (ctx.node() == last) {
                            break;
                        } else {
                            --nitems_;
                            cbk(pop_item_(ctx)->node());
                        }
                    } else {
                        ctx = next_ctx_bucket_(ctx);
                    }
                } while (ctx.ptr() != end_ctx.ptr());

                return;
            }
        }
    }

    template <class F>
    void pop(iterator first, iterator last, F&& cbk) {
        pop(first.node(), last.node(), std::forward<F>(cbk));
    }

    template <class K>
    node_type* pop_one(const K& key) noexcept {
        const auto found_ctx = find_ctx_(key);

        if (found_ctx.second) {
            --nitems_;
            return pop_item_(found_ctx.first)->node();
        } else {
            return nullptr;
        }
    }

    template <class K, class F>
    void pop_all(const K& key, F&& cbk) {
        const auto found_ctx = find_ctx_(key);

        if (found_ctx.second) {
            do {
                --nitems_;
                cbk(pop_item_(found_ctx.first)->node());
            } while (ctx_item_(found_ctx.first) && ctx_relative_(found_ctx.first, key));
        }
    }

    template <class K>
    void pop_all(const K& key) noexcept {
        pop_all(key, [](node_type*){});
    }

    template <class K, class F>
    std::pair<iterator, bool> find_or_push_no_resize(const K& key, const F& gen) {
        const auto found_ctx = find_ctx_(key);

        if (!found_ctx.second) {
            push_item_(found_ctx.first, gen());
            ++nitems_;
            return {found_ctx.first.item(), true};
        }

        return {found_ctx.first.item(), false};
    }

    template <class K, class F>
    std::pair<iterator, bool> find_or_push(const K& key, const F& gen) {
        resize(nitems_ + 1);
        return find_or_push_no_resize(key, gen);
    }

private:
    intrhash_t(const intrhash_t&) = delete;
    intrhash_t& operator=(const intrhash_t&) = delete;

public:
    intrhash_t()
        : intrhash_t(0)
    {}

    explicit intrhash_t(size_t n)
        : buckets_(intrhash_priv::buckets_count(n) + 1, allocator_type())
    {
        init_buckets_(&buckets_);
    }

    explicit intrhash_t(const allocator_type& allocator)
        : intrhash_t(0, allocator)
    {}

    template <class X>
    explicit intrhash_t(size_t n, X&& allocator_param)
        : buckets_(intrhash_priv::buckets_count(n) + 1, std::forward<X>(allocator_param))
    {
        init_buckets_(&buckets_);
    }

    intrhash_t(intrhash_t&& right) {
        buckets_.swap(right.buckets_);
        std::swap(nitems_, right.nitems_);
    }

    ~intrhash_t() noexcept {
        decompose();
    }

public:
    void swap(intrhash_t& right) noexcept {
        buckets_.swap(right.buckets_);
        std::swap(nitems_, right.nitems_);
    }

    auto get_allocator() const noexcept -> decltype(intrhash_util::declret<buckets_type>().get_allocator()) {
        return buckets_.get_allocator();
    }

protected:
    template <class F>
    intrhash_t(const intrhash_t& right, F gen)
        : buckets_(right.buckets_.size(), right.buckets_.get_allocator())
        , nitems_(right.nitems_)
    {
        init_buckets_(&buckets_);

        for (size_t i = 0; i != buckets_.size() - 1; ++i) {
            const_context_type ctx(&right.buckets_[i]);
            context_type ins(&buckets_[i]);

            while (!ctx_bucket_(ctx)) {
                push_item_(ins, gen(ctx.node()));
                ctx = next_ctx_item_(ctx);
            }
        }
    }

private:
    buckets_type buckets_;
    size_t nitems_ = 0;
};

template <class T, class O, class D = intrhash_util::delete_ops, class A = std::allocator<T>>
class ownintrhash_t
    : public intrhash_t<T, O, A>
{
private:
    using base_type = intrhash_t<T, O, A>;

    using item_type = typename base_type::item_type;
    using node_type = typename base_type::node_type;

public:
    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;

public:
    ownintrhash_t() = default;

    ownintrhash_t(size_t n)
        : base_type(n)
    {}

    template <class X>
    ownintrhash_t(size_t n, X&& allocator_param)
        : base_type(n, std::forward<X>(allocator_param))
    {}

    ownintrhash_t(ownintrhash_t&& right) noexcept
        : base_type(std::move(right))
    {}

    ~ownintrhash_t() noexcept {
        clear();
    }

public:
    size_t erase(item_type* item) noexcept {
        if (item = pop(item)) {
            D::destroy(item);
            return 1;
        } else {
            return 0;
        }
    }

    size_t erase(iterator iter) noexcept {
        return erase(iter.node());
    }

    template <class K>
    size_t erase(const K& key) noexcept {
        if (node_type* const node = this->pop(key)) {
            D::destroy(node);
            return 1;
        } else {
            return 0;
        }
    }

    template <class K>
    size_t erase_all(const K& key) noexcept {
        size_t result = 0;
        this->pop_all(key, [&result](node_type* node){ D::destroy(node); ++result; });
        return result;
    }

    void clear() noexcept {
        this->decompose([](node_type* node){ D::destroy(node); });
    }
};
