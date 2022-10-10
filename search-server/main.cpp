#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ERROR_RATE = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
template <typename StringContainer>
set<string> UniqueContainerWithoutEmpty(const StringContainer& text) {
    set<string> no_empty_text;
    for (const auto& word : text) {
        if (!word.empty()) {
            no_empty_text.insert(word);
        }
    }
    return no_empty_text;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;

    Document()
        :id(0),
        relevance(0.0),
        rating(0)
    {
    }

    Document(int id_doc, double relevance_doc, int rating_doc)
        :id(id_doc),
        relevance(relevance_doc),
        rating(rating_doc)
    {
    }
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    //inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StopWordsContainer>
    explicit SearchServer(const StopWordsContainer& stop_words) : stop_words_(UniqueContainerWithoutEmpty(stop_words))
    {
        for (const auto& word : stop_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("stop words contain invalid characters");
            }
        }
    }

    explicit SearchServer(const string& stop_words) : stop_words_(UniqueContainerWithoutEmpty(SplitIntoWords(stop_words)))
    {
        if (!IsValidWord(stop_words)) {
            throw invalid_argument("stop words contain invalid characters");
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {

        if (!CheckID(document_id) || !IsValidWord(document)) {
            throw invalid_argument("the document id already exists or is less than zero");
        }

        if (!IsValidWord(document)) {
            throw invalid_argument("the document contain invalid characters");
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        added_doc_id_.push_back(document_id);
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });
    }


    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < ERROR_RATE) {
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

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const auto query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return tuple<vector<string>, DocumentStatus>{ matched_words, documents_.at(document_id).status };    
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >  added_doc_id_.size()) {
           throw out_of_range("invalid document ID");
        }
        return  added_doc_id_[index];
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_; // Контейнер стоп-слов
    map<string, map<int, double>> word_to_document_freqs_; // Контейнер слово - ID и IDF-TF
    map<int, DocumentData> documents_; // ID Документа и его рейтинг и статус
    vector<int> added_doc_id_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {

        Query query;
        if (!IsValidWord(text)) {
            throw invalid_argument("query words contain invalid characters");
        }

        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidStopWord(word)) {
                throw invalid_argument("minus words are set incorrectly");
            }
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

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static bool IsValidStopWord(const string& word) {
        if (word.find("--") != string::npos || (word.size() == 1 && word == "-") || word.back() == '-') {
            return false;
        }
        return true;
    }

    bool CheckID(const int& id) const {
        if (documents_.count(id) != 0 || id < 0) {
            return false;
        }
        return true;
    }

};