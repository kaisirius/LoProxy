#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

// mirrored version of that in src
class ConnectionState { // Proxy Session
    private:
        int32_t connectedSocketFD;
        int32_t upstreamFD;
    public:
        ConnectionState(const int32_t fd) {
            connectedSocketFD = fd;
        }
        
        int32_t connectUpstream(std::string& host, int32_t port) {
            std::cout << "[LOG]: Creating upstream socket to connect with backend" << "\n";

            upstreamFD = socket(AF_INET, SOCK_STREAM, 0);
            if(upstreamFD == -1) {
                std::cout << "[ERROR]: Internal server error while creating upstream socket." << "\n";
            } else {
                fcntl(upstreamFD, F_SETFL, O_NONBLOCK); // non blocking ops
                //method - 1 (handles both type of hosts 127.0.0.1 & localhost/xyz) 
                addrinfo* result;
                addrinfo hints{};
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;

                int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);

                // method - 2 (handles only hosts like 127.0.0.1)
                // sockaddr_in addr;
                // socklen_t addrLen;
                // addr.sin_family = AF_INET;
                // addr.sin_port = htons(8080);
                // inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

                if(status != 0) { // Address resolution error
                    close(upstreamFD); // will close the FD since we can't resolve our connection and still return -1 as system failure
                    std::cout << "[ERROR]: DNS Resolution failed." << "\n";
                    return -1;
                }
                int upstreamConnectionStatus = connect(upstreamFD, result->ai_addr, result->ai_addrlen);
                
                if(upstreamConnectionStatus == -1 && errno != EINPROGRESS) {
                    close(upstreamFD); // will close the FD since we can't resolve our connection and still return -1 as system failure
                    std::cout << "[ERROR]: Connection to upstream failed." << "\n";
                    freeaddrinfo(result);
                    return -1;
                }
                freeaddrinfo(result);
            }

            return upstreamFD;
        }
        
};

int main() {
    // throw away code
    ConnectionState connState(123);
    std::string domain = "httpbin.org";
    int upstreamFD = connState.connectUpstream(domain, 80);
    std::cout << "upstream FD : " << upstreamFD << "\n";
    std::string req = "GET / HTTP/1.0\r\nHost: httpbin.org\r\n\r\n";
    ssize_t sent = send(upstreamFD, req.c_str(), req.length(), 0);
    while(sent == -1) {
        sent = send(upstreamFD, req.c_str(), req.length(), 0);
    }
    std::cout << "Request forwarded" << "\n";

    char response[1025];
    std::string accumulatedResponse = ""; 

    ssize_t msgSizeRec = recv(upstreamFD, response, 1024, 0);
  
    while(true) {
        if(msgSizeRec == 0) {
            close(upstreamFD);
            break;
        } 
        if(msgSizeRec != -1)
        response[msgSizeRec] = '\0';

        accumulatedResponse = accumulatedResponse + response;

        msgSizeRec = recv(upstreamFD, response, 1024, 0);
    }

    std::cout << accumulatedResponse << "\n";
}