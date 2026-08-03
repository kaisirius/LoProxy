#include <http/ParseResult.hpp>

ParseResult::ParseResult(Status status, size_t bytes_consumed) {
    this->status = status;
    this->bytes_consumed = bytes_consumed;
}