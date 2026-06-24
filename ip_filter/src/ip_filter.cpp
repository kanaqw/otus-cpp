#include "ip_filter.hpp"
#include <algorithm>
#include <iterator>


std::vector<std::string> split(const std::string &str, char d) {
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

template<typename Predicate>
std::vector<std::vector<std::string>> filter_ips(const std::vector<std::vector<std::string>>& ip_pool,
                                                Predicate pred) 
{
    std::vector<std::vector<std::string>> result;

    std::copy_if(
        ip_pool.begin(),
        ip_pool.end(),
        std::back_inserter(result),
        pred);

    return result;
}

int IpFilter::filter_ip_addresses() {
    try
    {
        std::vector<std::vector<std::string>> ip_pool;

            std::string line;
            while (std::getline(std::cin, line)) {
                if (line.empty()) {
                    break;
                }
                std::vector<std::string> v = split(line, '\t');
                ip_pool.push_back(split(v.at(0), '.'));
            }
        // TODO reverse lexicographically sort
        std::sort(ip_pool.begin(), ip_pool.end(),[](const auto& lhs, const auto& rhs) {
                for (size_t i = 0; i < 4; ++i)
                {
                    int left = std::stoi(lhs[i]);
                    int right = std::stoi(rhs[i]);

                    if (left != right)
                        return left > right; 
                }

                return false;
            });
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

        auto filter1 = filter_ips( ip_pool, [](const auto& ip){return ip[0] == "1";});
        print_ip(filter1);

        // 1.231.69.33
        // 1.87.203.225
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first and second bytes and output
        // ip = filter(46, 70)
        auto filter1 = filter_ips( ip_pool, [](const auto& ip){return ip[0] == "46" && ip[1] == "70";});
        print_ip(filter2);

        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76

        // TODO filter by any byte and output
        // ip = filter_any(46)

        auto filter1 = filter_ips( ip_pool, [](const auto& ip){return part == "46";});
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
