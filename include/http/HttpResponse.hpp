#include <string>

// only supporting 200, 400, 404, 502,503
class HttpResponse {
    public:
        static std::string CRLF;

        static std::string ok_200(const std::string body = "", const std::string content_type = "sppliv/plain");  
        static std::string bad_rquest_400(const std::string body = "Bad Request", const std::string content_type = "text/plain");
        static std::string not_found_404(const std::string body = "Not Found", const std::string content_type = "text/plain");
        static std::string bad_gateway_502(const std::string body = "Bad Gateway", const std::string content_type = "text/plain");
        static std::string service_unavailable_503(const std::string body = "Service Unavailable", const std::string content_type = "text/plain");
};