#pragma once
#include <stdexcept>
#include <queue>
#include <cmath>
#include <execution>
#include <list>
#include <string_view>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
//#include "log_duration.h"

using namespace std::literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ERROR_RATE = 1e-6;

class SearchServer {
public:

    template <typename StopWordsContainer>
    explicit SearchServer(const StopWordsContainer& stop_words);

    explicit SearchServer(const std::string& stop_words);
    explicit SearchServer(const std::string_view stop_words);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentStatus document_status) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentStatus document_status) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> stop_words_; // Контейнер стоп-слов
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // Контейнер слово - word и ID-TF
    std::map<int, DocumentData> documents_; // ID Документа и его рейтинг и статус
    std::set<int> added_doc_id_;
    std::map<int, std::map<std::string_view, double>> doc_id_words_freq_;
    std::set<std::string, std::less<>> word_of_documents_;

    bool IsStopWord(const std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;
    Query ParseQuery(const std::string_view text, bool sort) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;

    static bool IsValidWord(const std::string_view word);

    static bool IsValidStopWord(const std::string_view word);

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
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

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

    for (const auto& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_id_data = documents_.at(document_id);
            if (document_predicate(document_id, document_id_data.status, document_id_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    return FindAllDocuments(query, document_predicate);
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(std::max(static_cast<int>(query.plus_words.size()), 100));

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&document_to_relevance, this, &document_predicate](const auto& word) {
        if (word_to_document_freqs_.count(word) != 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_id_data = documents_.at(document_id);
                if (document_predicate(document_id, document_id_data.status, document_id_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
        });


    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&document_to_relevance, this](const auto& word) {
        if (word_to_document_freqs_.count(word) != 0) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Erase(document_id);
            }
        }
        });

    std::vector<Document> matched_documents;
    std::map<int, double> result = document_to_relevance.BuildOrdinaryMap();
    matched_documents.reserve(result.size());

    std::for_each(std::execution::par, result.begin(), result.end(), [&matched_documents, this](const auto& doc) {
        matched_documents.push_back(
            { doc.first, doc.second, documents_.at(doc.first).rating });
        });

    return matched_documents;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);