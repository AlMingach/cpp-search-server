#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::set<std::string_view>, int> words_to_doc_id;
    std::set<int> erase_doc_id;
    for (const auto& document_id : search_server) {
        if (erase_doc_id.count(document_id) != 0) {
            continue;
        }
        std::set<std::string_view> words;
        for (const auto& [word, _] : search_server.GetWordFrequencies(document_id)) {
            words.insert(word);
        }
        if (words_to_doc_id.count(words) != 0) {
            erase_doc_id.insert(document_id);
        }
        else {
            words_to_doc_id[words] = document_id;
        }
        
    }

    for (auto& id : erase_doc_id) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}