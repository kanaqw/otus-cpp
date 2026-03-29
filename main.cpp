#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

unsigned long long factorial(unsigned int n) {
    return (n == 0) ? 1 : n * factorial(n - 1);
}

template<typename T, size_t N>
class CustomAllocator {
public:
    using value_type = T;

    CustomAllocator() : pool(nullptr), used(0) {
        pool = static_cast<T*>(::operator new(sizeof(T) * N));
    }

    template<class U>
    CustomAllocator(const CustomAllocator<U, N>&) noexcept {}

    ~CustomAllocator() {
        ::operator delete(pool);
    }

    T* allocate(std::size_t n) {
        if (used + n > N) {
            throw std::bad_alloc();
        }

        T* ptr = pool + used;
        used += n;
        return ptr;
    }

    void deallocate(T*, std::size_t) noexcept {}

    template<class U>
    struct rebind {
        using other = CustomAllocator<U, N>;
    };

private:
    T* pool;
    size_t used;
};

template<typename T, typename Alloc = std::allocator<T>>
class CustomContainer {

    struct Node {
        T value;
        Node* next;
    };

    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;

public:
    CustomContainer() : head(nullptr), tail(nullptr), size_(0) {}

    ~CustomContainer() {
        Node* current = head;

        while(current) {
            Node* next = current->next;
            node_alloc.deallocate(current,1);
            current = next;
        }
    }

    void push_back(const T& value) {
        Node* node = node_alloc.allocate(1);
        node->value = value;
        node->next = nullptr;

        if (!head) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }

        ++size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    size_t size() const {
        return size_;
    }

    class Iterator {
    public:
        Iterator(Node* n) : node(n) {}

        T& operator*() {
            return node->value;
        }

        Iterator& operator++() {
            node = node->next;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return node != other.node;
        }

    private:
        Node* node;
    };

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }

private:
    Node* head;
    Node* tail;
    size_t size_;

    NodeAlloc node_alloc;
};

int main() {

    //1
    std::map<int,int> test_stl_map;
    //2
    for (int i = 0; i < 10; ++i) {
        test_stl_map.emplace(i,factorial(i));
    }
    //3
    std::map<int,int, std::less<int>, CustomAllocator<std::pair<const int, int>, 10>> test_custom_map;
    //4
    for (int i = 0; i < 10; ++i) {
        test_custom_map.emplace(i,factorial(i));
    }
    //5
    for (const auto& pair : test_custom_map) {
        std::cout << pair.first << " " << pair.second << std::endl;
    }
    //6
    CustomContainer<int, std::allocator<int>> test_container_std_alloc;
    //7
    for (int i = 0; i < 10; ++i) {
        test_container_std_alloc.push_back(i);
    }
    //8
    CustomContainer<int, CustomAllocator<int, 10>> test_container_custom_alloc;
    //9
    for (int i = 0; i < 10; ++i) {
        test_container_custom_alloc.push_back(i);
    }
    //10
    for (const auto& value : test_container_custom_alloc) {
       std::cout << value << " ";
    }

    return 0;
}
