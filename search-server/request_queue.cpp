#include "request_queue.h"


std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {

    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddRequests(static_cast<int>(result.size()));
    return result;

}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {

    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddRequests(static_cast<int>(result.size()));
    return result;

}

    RequestQueue::RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
        , no_results_requests_(0)
        , current_time_(0)
    {

    }

    int RequestQueue::GetNoResultRequests() const {
        return no_results_requests_;
    }


    RequestQueue::QueryResult::QueryResult(uint64_t time, int result)
        : timestamp(time)
        , results(result)
    {
    }

    void RequestQueue::AddRequests(int result) {

        ++current_time_;

        if (!requests_.empty() && current_time_ >= min_in_day_ + 1) {
            if (requests_.front().results == 0) {
                --no_results_requests_;
            }
            requests_.pop_front();
        }
        requests_.push_back(QueryResult(current_time_, result));
        if (result == 0) {
            ++no_results_requests_;
        }

    }