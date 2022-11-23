#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::sort(search_server.begin(), search_server.end(), [](const auto& lhs, const auto& rhs) {
        return lhs < rhs;
        });
    std::set<int> erase_doc_id;
    auto it_end = search_server.end();
    for (auto it = search_server.begin(); it < it_end - 1; ++it) {
        if (erase_doc_id.count(*it) != 0) {
            continue;
        }
        std::map<std::string, double> lhs = search_server.GetWordFrequencies(*it);
        for (auto it_ = it + 1; it_ < it_end; ++it_) {
            if (erase_doc_id.count(*it_) == 1) {
                continue;
            }
            std::map<std::string, double> rhs = search_server.GetWordFrequencies(*it_);
            if (Сomparison(lhs, rhs)) {
                erase_doc_id.insert(*it_);
            }
        }

    }

    for (auto& id : erase_doc_id) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}

//void RemoveDuplicates(SearchServer& search_server) {
//    std::reverse(search_server.begin(), search_server.end());
//    std::set<int> erase_doc_id;
//    for (const auto& document_id_lhs : search_server) {
//        if (erase_doc_id.count(document_id_lhs) != 0) {
//            continue;
//        }
//        const auto& lhs = search_server.GetWordFrequencies(document_id_lhs);
//        for (const auto& document_id_rhs : search_server) {
//            const auto& rhs = search_server.GetWordFrequencies(document_id_rhs);
//            if (document_id_lhs != document_id_rhs && Сomparison(lhs, rhs)) {
//                erase_doc_id.insert(document_id_rhs);
//            }
//        }
//    }
//        for (auto& id : erase_doc_id) {
//        std::cout << "Found duplicate document id "s << id << std::endl;
//        search_server.RemoveDocument(id);
//        }
//}

bool Сomparison(const std::map<std::string, double>& lhs, const std::map<std::string, double>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& [word, _] : lhs) {
        if (rhs.count(word) == 0) {
            return false;
        }
    }
    return true;
}