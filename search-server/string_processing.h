#pragma once
#include <set>
#include <string>
#include <vector>


//Парсинг контейнера на уникальные и не пустые слова
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

// Разбивает строку на слова, разделенные пробелами, игнорируя лишние пробелы
std::vector<std::string_view> SplitIntoWords(std::string_view text);