#pragma once
#include <stddef.h>
#include <enums/Status.hpp>

struct ParseResult {
    Status status;
    size_t bytes_consumed;  

    ParseResult(Status status, size_t bytes_consumed);
};