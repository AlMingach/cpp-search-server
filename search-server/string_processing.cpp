#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    const int64_t pos_end = str.npos;

    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        str.remove_prefix(space == pos_end ? str.size() : space);
        str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    }
    return result;
}