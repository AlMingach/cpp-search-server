#include "process_queries.h"


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& query) {
        return search_server.FindTopDocuments(query);
        });

    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result_process_queries = ProcessQueries(search_server, queries);
    std::list<Document> result;
    for (auto& elem : result_process_queries) {
        std::move(elem.begin(), elem.end(), std::back_inserter(result));
    }
    return result;
}