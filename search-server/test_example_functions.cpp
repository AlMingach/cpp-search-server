#include "test_example_functions.h"
#include "search_server.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using namespace std::literals;

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(const T& func, const std::string& t_str) {
    func();
    std::cerr << t_str << " OK" << std::endl;
}


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }

    {
        SearchServer server(" "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in the"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

void TestExcludeStopWords() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::string stop_words = "cat the"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(stop_words);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat the"s);
        ASSERT(found_docs.empty());
    }

    {
        SearchServer server(stop_words);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

}
void TestExcludeMinusWords() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("-in the"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);
    }

    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("-the in"s);
        ASSERT(found_docs.empty());
    }
}

void TestMatchedDocuments() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs = server.MatchDocument("-in the"s, doc_id_1);
        ASSERT(std::get<0>(found_docs).empty());
    }

    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs = server.MatchDocument("the in"s, doc_id_1);
        ASSERT_EQUAL(std::get<0>(found_docs).size(), 2);
    }
}
void TestRelevanceSort() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in the"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT(found_docs[0].relevance > found_docs[1].relevance);
    }
}
void TestCalculatingRating() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 3, 3, 3 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in and"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL(doc0.rating, 3);
        ASSERT_EQUAL(doc1.rating, 2);
        ASSERT_EQUAL(doc0.id, 42);
        ASSERT_EQUAL(doc1.id, 13);
    }
}
void TestFiltrationPredicate() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 3, 3, 3 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in and"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_1);
    }
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto found_docs = server.FindTopDocuments("in and"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);
    }
}

void TestFiltrationStatus() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 3, 3, 3 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    const int doc_id_3 = 18;
    const std::string content_3 = "black and white"s;
    const std::vector<int> ratings_3 = { 3, 3, 3 };
    const int doc_id_4 = 58;
    const std::string content_4 = "hedgehog in the fog"s;
    const std::vector<int> ratings_4 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::IRRELEVANT, ratings_3);
        server.AddDocument(doc_id_4, content_4, DocumentStatus::REMOVED, ratings_4);
        const auto found_docs_1 = server.FindTopDocuments("in and"s, DocumentStatus::ACTUAL);
        const auto found_docs_2 = server.FindTopDocuments("in and"s, DocumentStatus::BANNED);
        const auto found_docs_3 = server.FindTopDocuments("in and"s, DocumentStatus::IRRELEVANT);
        const auto found_docs_4 = server.FindTopDocuments("in and"s, DocumentStatus::REMOVED);

        ASSERT_EQUAL(found_docs_1.size(), 1);
        const Document& doc1 = found_docs_1[0];
        ASSERT_EQUAL(doc1.id, doc_id_1);

        ASSERT_EQUAL(found_docs_2.size(), 1);
        const Document& doc2 = found_docs_2[0];
        ASSERT_EQUAL(doc2.id, doc_id_2);

        ASSERT_EQUAL(found_docs_3.size(), 1);
        const Document& doc3 = found_docs_3[0];
        ASSERT_EQUAL(doc3.id, doc_id_3);

        ASSERT_EQUAL(found_docs_4.size(), 1);
        const Document& doc4 = found_docs_4[0];
        ASSERT_EQUAL(doc4.id, doc_id_4);
    }
}

void TestCalculatingRelevance() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 1, 1 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in the"s);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        const double relevance_1 = 1.0 / 4.0 * log(2.0 / 2.0) + 1.0 / 4.0 * log(2.0 / 1.0);
        const double relevance_2 = 1.0 / 4.0 * log(2.0 / 2.0);
        ASSERT_EQUAL(doc0.relevance, relevance_1);
        ASSERT_EQUAL(doc1.relevance, relevance_2);
    }
}

void TestRemoveDocument() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 1, 1 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.RemoveDocument(42);
        int n = server.GetDocumentCount();
        ASSERT_EQUAL(n, 1);
        const auto found_docs = server.FindTopDocuments("in the"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, 13);
    }
}

void TestRemoveDuplicate() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 1, 1 };
    const int doc_id_2 = 13;
    const std::string content_2 = "the dog and clock"s;
    const std::vector<int> ratings_2 = { 3, 3, 3 };
    const int doc_id_3 = 14;
    const std::string content_3 = "cat in the city"s;
    const std::vector<int> ratings_3 = { 1, 2, 3 };
    {
        SearchServer server("sa"s);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        RemoveDuplicates(server);
        const auto found_docs = server.FindTopDocuments("in the"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, 14);
        const Document& doc2 = found_docs[1];
        ASSERT_EQUAL(doc2.id, 13);
        int n = server.GetDocumentCount();
        ASSERT_EQUAL(n, 2);
    }


}

void TestProcessQueries() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    std::vector<size_t> assert_result{ 3, 5, 2 };
    for ( const auto& documents : ProcessQueries(search_server, queries)) {
        assert(documents.size() == assert_result[id++]);
    }
}

void TestProcessQueriesJoined() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    std::vector<size_t> assert_result_id{ 1, 5, 4, 3, 1, 2, 5, 4, 2, 5 };
    for (const auto& documents : ProcessQueriesJoined(search_server, queries)) {
        assert(documents.id == assert_result_id[id++]);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeStopWords);
    RUN_TEST(TestExcludeMinusWords);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestCalculatingRating);
    RUN_TEST(TestFiltrationPredicate);
    RUN_TEST(TestFiltrationStatus);
    RUN_TEST(TestCalculatingRelevance);
    RUN_TEST(TestRemoveDocument);
    RUN_TEST(TestRemoveDuplicate);
    RUN_TEST(TestProcessQueries);
    RUN_TEST(TestProcessQueriesJoined);
    std::cout << "All tests complite!\n" << std::endl;
}