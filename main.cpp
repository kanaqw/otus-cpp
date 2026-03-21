#include <iostream>
#include "ip_filter/ip_filter.hpp"

int main(int argc, char* argv[]){
    if(argc > 2) {
        std::cout << "Use ./deb_name <filepath_to_tsv/tsv_name.tsv>" << std::endl;
    } else {
        IpFilter ip_filter(argv[1]);
        ip_filter.filter_ip_addresses();
    }
    return 0;
}
