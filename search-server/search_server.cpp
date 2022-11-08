#include "search_server.h"

     SearchServer::SearchServer(const std::string& stop_words) : stop_words_(UniqueContainerWithoutEmpty(SplitIntoWords(stop_words)))
    {
       if (!IsValidWord(stop_words)) {
           throw std::invalid_argument("stop words contain invalid characters");
       }
    }

    void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings) {

        if (!CheckID(document_id)) {
            throw std::invalid_argument("the document id already exists or is less than zero");
        }

        if (!IsValidWord(document)) {
            throw std::invalid_argument("the document contain invalid characters");
        }

        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        added_doc_id_.push_back(document_id);
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });
    }

    int SearchServer::GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
        const auto query = ParseQuery(raw_query);
        std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return std::tuple<std::vector<std::string>, DocumentStatus>{ matched_words, documents_.at(document_id).status };
    }

    int SearchServer::GetDocumentId(int index) const {
        if (index < 0 || index >  added_doc_id_.size()) {
            throw std::out_of_range("invalid document ID");
        }
        return  added_doc_id_[index];
    }


    bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            if (!IsValidStopWord(text)) {
                throw std::invalid_argument("minus words are set incorrectly");
            }
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {

        Query query;
        if (!IsValidWord(text)) {
            throw std::invalid_argument("query words contain invalid characters");
        }

        for (const std::string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }

        return query;
    }

    double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }


    bool SearchServer::IsValidWord(const std::string& word) {
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    bool SearchServer::IsValidStopWord(const std::string& word) {
        if (word[1] == '-' || word.size() == 1 || word.back() == '-') {
            return false;
        }
        return true;
    }

    bool SearchServer::CheckID(const int& id) const {
        if (documents_.count(id) != 0 || id < 0) {
            return false;
        }
        return true;
    }