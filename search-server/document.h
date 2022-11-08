#pragma once
#include <iostream>

struct Document {
    int id;
    double relevance;
    int rating;

    Document();

    Document(int id_doc, double relevance_doc, int rating_doc);
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& output, Document document);