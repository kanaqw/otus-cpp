#include "ip_filter.hpp"


std::vector<std::string> split(const std::string &str, char d)
{
    std::vector<std::string> r;

    auto start = 0;
    auto stop = str.find_first_of(d);
    while(stop != std::string::npos)
    {
        r.push_back(str.substr(start, stop - start));

        start = stop + 1;
        stop = str.find_first_of(d, start);
    }

    r.push_back(str.substr(start));

    return r;
}

std::vector<int> ipToNumbers(const std::string& ip) {
    std::vector<int> parts;
    std::stringstream ss(ip);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
        parts.push_back(std::stoi(segment));
    }
    return parts;
}

void print_ip(std::vector<std::vector<std::string>> ip_pool){
    for(auto ip = ip_pool.cbegin(); ip != ip_pool.cend(); ++ip)
    {
        for(auto ip_part = ip->cbegin(); ip_part != ip->cend(); ++ip_part)
        {
            if (ip_part != ip->cbegin())
            {
                std::cout << ".";

            }
            std::cout << *ip_part;
        }
        std::cout << std::endl;
    }
}

int IpFilter::filter_ip_addresses()
{
    try
    {
        std::vector<std::vector<std::string>> ip_pool;

        if (filepath_.empty()){
            std::cerr << "filepath is empty, reading from cin " << std::endl;

            std::string line;
            while (std::getline(std::cin, line)) {
                std::vector<std::string> v = split(line, '\t');
                ip_pool.push_back(split(v.at(0), '.'));
            }
        } else {
            std::string file_path = filepath_;
            std::ifstream file(filepath_); 

            if (!file.is_open()) {
                std::cerr << "Error opening file: " << filepath_ << std::endl;
                return 1;
            }

            for(std::string line; std::getline(file, line);) {
                std::vector<std::string> v = split(line, '\t');
                ip_pool.push_back(split(v.at(0), '.'));
            }
        }


        // TODO reverse lexicographically sort
        for (auto i = ip_pool.begin(); i != ip_pool.end(); ++i) {
            for (auto j = ip_pool.begin(); std::next(j) != ip_pool.end(); ++j) {
                auto next_it = std::next(j);
                
                bool less = false;
                for(size_t k = 0; k < 4; ++k) {
                    int left = std::stoi((*j)[k]);
                    int right = std::stoi((*next_it)[k]);
                    if (left < right) { less = true; break; }
                    if (left > right) { break; }
                }

                if (less) std::swap(*j, *next_it);
            }
        }

        print_ip(ip_pool);

        // 222.173.235.246
        // 222.130.177.64
        // 222.82.198.61
        // ...
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first byte and output
        // ip = filter(1)

        std::vector<std::vector<std::string>> filter1;
        for(const auto& ip : ip_pool) {
            if (ip[0] == "1") filter1.push_back(ip);
        }
        print_ip(filter1);

        // 1.231.69.33
        // 1.87.203.225
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first and second bytes and output
        // ip = filter(46, 70)
        std::vector<std::vector<std::string>> filter2;
        for(const auto& ip : ip_pool) {
            if (ip[0] == "46" && ip[1] == "70") filter2.push_back(ip);
        }
        print_ip(filter2);

        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76

        // TODO filter by any byte and output
        // ip = filter_any(46)

        std::vector<std::vector<std::string>> filter3;
        for(const auto& ip : ip_pool) {
            bool found = false;
            for(const auto& part : ip) {
                if (part == "46") { found = true; break; }
            }
            if (found) filter3.push_back(ip);
        }
        print_ip(filter3);

        // 186.204.34.46
        // 186.46.222.194
        // 185.46.87.231
        // 185.46.86.132
        // 185.46.86.131
        // 185.46.86.131
        // 185.46.86.22
        // 185.46.85.204
        // 185.46.85.78
        // 68.46.218.208
        // 46.251.197.23
        // 46.223.254.56
        // 46.223.254.56
        // 46.182.19.219
        // 46.161.63.66
        // 46.161.61.51
        // 46.161.60.92
        // 46.161.60.35
        // 46.161.58.202
        // 46.161.56.241
        // 46.161.56.203
        // 46.161.56.174
        // 46.161.56.106
        // 46.161.56.106
        // 46.101.163.119
        // 46.101.127.145
        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76
        // 46.55.46.98
        // 46.49.43.85
        // 39.46.86.85
        // 5.189.203.46
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
