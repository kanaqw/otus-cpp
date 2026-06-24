#include <iostream>
#include "ip_filter.hpp"

int main(){
        IpFilter ip_filter;
        std::cout << "Enter string of ip's in next sequence: text1 \\t text2 \\t text3" << std::endl;
        std::cout << "type empty line to finish" << std::endl;
        ip_filter.filter_ip_addresses();
        return 0;
}
