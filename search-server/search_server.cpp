#include "search_server.h"

using namespace std::literals;

SearchServer::SearchServer(const std::string& stop_words) : SearchServer(std::string_view(stop_words))
{
}

SearchServer::SearchServer(const std::string_view stop_words)
{
    if (!IsValidWord(stop_words)) {
        throw std::invalid_argument("stop words contain invalid characters");
    }
    for (auto word : SplitIntoWords(stop_words)) {
        stop_words_.insert(std::string(word));
    }
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {

    if (!CheckID(document_id)) {
        throw std::invalid_argument("the document id already exists or is less than zero");
    }

    if (!IsValidWord(document)) {
        throw std::invalid_argument("the document contain invalid characters");
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        auto it = word_of_documents_.emplace(std::string(word));
        word_to_document_freqs_[*(it.first)][document_id] += inv_word_count;
        doc_id_words_freq_[document_id][*(it.first)] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status});
    added_doc_id_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const {
    return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

using DocQueryAndStatus = std::tuple<std::vector<std::string_view>, DocumentStatus>;

DocQueryAndStatus SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {

    if (added_doc_id_.count(document_id) == 0) {
        throw std::out_of_range("the document id does not exist");
    }

    const auto query = ParseQuery(raw_query, true);
    std::vector<std::string_view> matched_words;

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const auto word) {
        return doc_id_words_freq_.at(document_id).count(word) != 0;
        })) {
        return { matched_words, documents_.at(document_id).status };
    }

    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}


DocQueryAndStatus SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

DocQueryAndStatus SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const {
    if (added_doc_id_.count(document_id) == 0) {
        throw std::out_of_range("the document id does not exist");
    }
    const auto query = ParseQuery(raw_query, false);

    DocQueryAndStatus result{ {}, documents_.at(document_id).status };

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const auto word) {
        return doc_id_words_freq_.at(document_id).count(word) != 0;
        })) {
        return result;
    }

    std::get<0>(result).resize(query.plus_words.size());

    auto it_end = copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), std::get<0>(result).begin(), [this, &document_id](const auto word) {
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        });

    std::sort(std::get<0>(result).begin(), it_end);
    it_end = std::unique(std::get<0>(result).begin(), it_end);
    std::get<0>(result).erase(it_end, std::get<0>(result).end());
    return result;
}


std::set<int>::iterator SearchServer::begin() {
    return added_doc_id_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return added_doc_id_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> empty_map_;

    return (doc_id_words_freq_.count(document_id) == 0) ? empty_map_ : doc_id_words_freq_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    auto it = added_doc_id_.find(document_id);
    if (it == end()) {
        throw std::out_of_range("invalid document ID");
    }
    added_doc_id_.erase(it);
    for (auto& [word, id_relev] : word_to_document_freqs_) {
        id_relev.erase(document_id);
    }

    for (auto& [word, _] : doc_id_words_freq_.at(document_id)) {
        auto iter = word_of_documents_.find(word);
        if ((*iter).empty()) {
            word_to_document_freqs_.erase(word);
            word_of_documents_.erase(iter);
        }
    }

    documents_.erase(document_id);
    doc_id_words_freq_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    const auto it = added_doc_id_.find(document_id);
    if (it != end()) {

        std::map<std::string_view, double> words_relev = doc_id_words_freq_.at(document_id);
        std::vector<std::string_view> words(words_relev.size());

        std::transform(std::execution::par, words_relev.begin(), words_relev.end(), words.begin(), [](const auto& word_relev) {
            return word_relev.first;
        });

        std::for_each(std::execution::par, words.begin(), words.end(), [this, &document_id](const auto& word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });


        std::for_each(words.begin(), words.end(), [this, &document_id](const auto& word) {
            auto iter = word_of_documents_.find(word);
            if ((*iter).empty()) {
                word_to_document_freqs_.erase(word);
                word_of_documents_.erase(iter);
            }
            });

        added_doc_id_.erase(it);
        documents_.erase(document_id);
        doc_id_words_freq_.erase(document_id);
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {

    return ratings.empty() ? 0 : (std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
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


SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool sort) const {
    Query query;
    if (!IsValidWord(text)) {
        throw std::invalid_argument("query words contain invalid characters");
    }

    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (sort) {
        for (auto* word : { &query.plus_words, &query.minus_words }) {
            std::sort(word->begin(), word->end());
            word->erase(unique(word->begin(), word->end()), word->end());
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {

    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}
bool SearchServer::IsValidStopWord(const std::string_view word) {

    return (word[1] == '-' || word.size() == 1 || word.back() == '-') ? false : true;
}

bool SearchServer::CheckID(const int& id) const {

    return (documents_.count(id) != 0 || id < 0) ? false : true;

}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}