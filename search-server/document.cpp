#include "document.h"

Document::Document()
    : id(0)
    , relevance(0.0)
    , rating(0)
{
}

Document::Document(int id_doc, double relevance_doc, int rating_doc)
    : id(id_doc)
    , relevance(relevance_doc)
    , rating(rating_doc)
{
}

std::ostream& operator<<(std::ostream& output, Document document) {
    output << "{ document_id = " << document.id << ", relevance = " << document.relevance << ", rating = " << document.rating << " }";
    return output;
}