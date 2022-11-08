#pragma once
#include <utility>
#include <iostream>
#include <vector>
#include <iterator>

template <typename Iterator>
class Page {
private:
    std::pair<Iterator, Iterator> page;
    size_t size_;
public:
    Page(Iterator range_begin, Iterator range_end) : size_(distance(range_begin, range_end)) {
        page = std::make_pair(range_begin, range_end);
    }

    Iterator begin()const {
        return page.first;
    }

    Iterator end()const {
        return page.second;
    }

    size_t size()const {
        return size_;
    }
};


template <typename Iterator>
class Paginator {
private:
    std::vector<Page<Iterator>> pages;
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {

        for (size_t left = distance(range_begin, range_end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = std::next(range_begin, current_page_size);
            pages.push_back(Page(range_begin, current_page_end));

            left -= current_page_size;
            range_begin = current_page_end;
        }

    }

    auto begin()const {
        return pages.begin();
    }

    auto end()const {
        return pages.end();
    }

    size_t size()const {
        return pages.size();
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, Page<Iterator> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        output << *it;
    }
    return output;
}