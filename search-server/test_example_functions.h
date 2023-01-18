#pragma once
#include <string>
#include <iostream>
#include <cassert>
#include <vector>

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s) 

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint)) 

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s) 

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint)) 

template <typename T>
void RunTestImpl(const T& func, const std::string& t_str);

#define RUN_TEST(func) RunTestImpl(func, #func) 

void TestExcludeStopWordsFromAddedDocumentContent();

void TestAddedDocumentContent();

void TestExcludeStopWords();

void TestExcludeMinusWords();

void TestMatchedDocuments();

void TestRelevanceSort();

void TestCalculatingRating();

void TestFiltrationPredicate();

void TestFiltrationStatus();

void TestCalculatingRelevance();

void TestRemoveDocument();

void TestRemoveDuplicate();

void TestProcessQueries();

void TestProcessQueriesJoined();

void TestSearchServer();