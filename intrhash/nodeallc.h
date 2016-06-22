#pragma once

template <class T, class A>
class nodeallc_t {
public:
    using node_type = T;
    using allocator_type = typename A::template rebind<node_type>::other;

    nodeallc_t() = default;

    nodeallc_t(const allocator_type& allocator)
        : allocator_(allocator)
    {}

    void swap(nodeallc_t& right) noexcept {
        std::swap(allocator_, right.allocator_);
    }

public:
    node_type* allocate_node() {
        return allocator_.allocate(1);
    }

    void deallocate_node(node_type* node) {
        allocator_.deallocate(node, 1);
    }

    template <class... X>
    node_type* new_node(X&&... params) {
        node_type* const ptr = allocate_node();

        try {
            return new (ptr) node_type(std::forward<X>(params)...);
        } catch (...) {
            deallocate_node(ptr);
            throw;
        }
    }

    void delete_node(node_type* node) {
        node->~node_type();
        deallocate_node(node);
    }

public:
    const allocator_type& get_allocator() const noexcept {
        return allocator_;
    }

private:
    allocator_type allocator_;
};
