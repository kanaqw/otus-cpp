#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "common.hpp"

namespace {

void test_split_basic() {
    auto tokens = split("a,b,c", ',');
    assert(tokens.size() == 3);
    assert(tokens[0] == "a");
    assert(tokens[1] == "b");
    assert(tokens[2] == "c");
}

void test_extract_price_well_formed_row() {
    std::string line =
        "2539,Clean quiet apt,2787,John,Brooklyn,Kensington,"
        "40.64749,-73.97237,Private room,149,1,9,2018-10-19,0.21,6,365";
    auto price = extract_price(line);
    assert(price.has_value());
    assert(std::abs(*price - 149.0) < 1e-9);
}

void test_extract_price_row_with_extra_commas_in_name() {
    // test for malformed string
    std::string line =
        "9935095,\"Entire pvt. house for rent ,2 floors 3 bdrms., 2.5  baths. "
        "Newly renovated,avail. From Nov (Phone number hidden by Airbnb) thru "
        "April (Phone number hidden by Airbnb) / month\",51024536,Nadia,"
        "Brooklyn,Sheepshead Bay,40.59195,-73.94639,Private room,500,30,0,,,2,173";
    auto price = extract_price(line);
    assert(price.has_value());
    assert(std::abs(*price - 500.0) < 1e-9);
}

void test_extract_price_malformed_short_row() {
    auto price = extract_price("too,short");
    assert(!price.has_value());
}

}  // namespace

int main() {
    test_split_basic();
    test_extract_price_well_formed_row();
    test_extract_price_row_with_extra_commas_in_name();
    test_extract_price_malformed_short_row();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
