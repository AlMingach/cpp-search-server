#pragma once
#include <map>
#include <stdexcept>
#include <numeric>
#include <queue>
#include <algorithm>
#include <cmath>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

using namespace std::literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ERROR_RATE = 1e-6;

class SearchServer {
public:

    template <typename StopWordsContainer>
    explicit SearchServer(const StopWordsContainer& stop_words);

    explicit SearchServer(const std::string& stop_words);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const;


    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    //int GetDocumentId(int index) const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    //std::set<int>::iterator begin();

    //std::set<int>::iterator end();

    void RemoveDocument(int document_id);

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_; // Контейнер стоп-слов
    std::map<std::string, std::map<int, double>> word_to_document_freqs_; // Контейнер слово - ID и IDF-TF
    std::map<int, DocumentData> documents_; // ID Документа и его рейтинг и статус
    std::vector<int> added_doc_id_;
    //std::set<int> added_doc_id_;
    std::map<int, std::map<std::string, double>> doc_id_words_freq_;
    const std::map<std::string, double> empty_map_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    static bool IsValidWord(const std::string& word);

    static bool IsValidStopWord(const std::string& word);

    bool CheckID(const int& id) const;
};

template <typename StopWordsContainer>
SearchServer::SearchServer(const StopWordsContainer& stop_words) : stop_words_(UniqueContainerWithoutEmpty(stop_words))
{
    for (const auto& word : stop_words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("stop words contain invalid characters");
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    //LOG_DURATION_STREAM("Operation time"s, std::cout);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < ERROR_RATE) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_id_data = documents_.at(document_id);
            if (document_predicate(document_id, document_id_data.status, document_id_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);