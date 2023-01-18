#pragma once
#include <set>
#include <string>
#include <vector>

template <typename StringContainer>
std::set<std::string, std::less<>> UniqueContainerWithoutEmpty(const StringContainer& text) {
    std::set<std::string, std::less<>> no_empty_text;
    for (const auto word : text) {
        if (!word.empty()) {
            no_empty_text.insert(word);
        }
    }
    return no_empty_text;
}

std::vector<std::string_view> SplitIntoWords(std::string_view text);