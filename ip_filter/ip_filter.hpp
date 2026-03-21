#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#include <sstream>
#include <vector>


class IpFilter {
    public:
        IpFilter(std::string filepath) : filepath_(filepath){};
        int filter_ip_addresses();

    private:
        std::string filepath_{};

};
