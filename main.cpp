#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <tuple>
#include <type_traits>

template <typename T>
struct is_container {
    static constexpr bool value = false;
};

template <typename... Args>
struct is_container<std::vector<Args...>> { static constexpr bool value = true; };

template <typename... Args>
struct is_container<std::list<Args...>> { static constexpr bool value = true; };

template <typename T>
struct is_tuple : std::false_type {};

template <typename... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type {};

template<typename T>
void print_ip(T input_val) {
    if constexpr (std::is_same_v<T, std::string>) {
        std::cout << input_val;
    }
    else if constexpr (std::is_integral_v<T>) {
        for (int i = sizeof(T) - 1; i >= 0; --i) {
            std::cout << ((input_val >> (i * 8)) & 0xFF);
            if (i > 0) std::cout << ".";
        }
    }
    else if constexpr (is_container<T>::value) {
        bool first = true;
        for (const auto& val : input_val) {
            if (!first) std::cout << ".";
            std::cout << val;
            first = false;
        }
    }
    else if constexpr (is_tuple<T>::value) {
        std::apply([](auto&&... args) {
            size_t n = 0;
            ((std::cout << (n++ ? "." : "") << args), ...);
        }, input_val);
    }

    std::cout << std::endl;
}

int main() {
    print_ip( int8_t{-1} );                                 // 255 
    print_ip( int16_t{0} );                                 // 0.0 
    print_ip( int32_t{2130706433} );                        // 127.0.0.1 
    print_ip( int64_t{8875824491850138409} );               // 123.45.67.89.101.112.131.41 
    print_ip( std::string{"Hello, World!"} );              // Hello, World! 
    print_ip( std::vector<int>{100, 200, 300, 400} );       // 100.200.300.400 
    print_ip( std::list<short>{400, 300, 200, 100} );       // 400.300.200.100 
    print_ip( std::make_tuple(123, 456, 789, 0) );          // 123.456.789.0

    return 0;
}
