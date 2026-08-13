#include <http/HttpResponse.hpp>

std::string HttpResponse::CRLF = "\r\n";

std::string HttpResponse::ok_200(const std::string body, const std::string content_type) {
    return "HTTP/1.1 200 OK" + CRLF +  
        "Content-Type: " + content_type + CRLF + 
        "Content-Length: " + std::to_string((int)body.length()) + CRLF + 
        "Connection: close" + CRLF + CRLF + 
        body;
}

std::string HttpResponse::bad_rquest_400(const std::string body, const std::string content_type) {
    return "HTTP/1.1 400 Bad Request" + CRLF +  
        "Content-Type: " + content_type + CRLF + 
        "Content-Length: " + std::to_string((int)body.length()) + CRLF + 
        "Connection: close" + CRLF + CRLF + 
        body;
}

std::string HttpResponse::not_found_404(const std::string body, const std::string content_type) {
    return "HTTP/1.1 404 Not Found" + CRLF +  
        "Content-Type: " + content_type + CRLF + 
        "Content-Length: " + std::to_string((int)body.length()) + CRLF + 
        "Connection: close" + CRLF + CRLF + 
        body;
}

std::string HttpResponse::bad_gateway_502(const std::string body, const std::string content_type) {
    return "HTTP/1.1 502 Bad Gateway" + CRLF +  
        "Content-Type: " + content_type + CRLF + 
        "Content-Length: " + std::to_string((int)body.length()) + CRLF + 
        "Connection: close" + CRLF + CRLF + 
        body;
}

std::string HttpResponse::service_unavailable_503(const std::string body, const std::string content_type) {
    return "HTTP/1.1 503 Service Unavailable" + CRLF +  
        "Content-Type: " + content_type + CRLF + 
        "Content-Length: " + std::to_string((int)body.length()) + CRLF + 
        "Connection: close" + CRLF + CRLF + 
        body;
}