#include <non-blocking-server/Server.hpp>
#include <engine/EpollEngine.hpp>
#include <models/proxy/ConnectionState.hpp>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <http/HttpResponse.hpp>
#include <netdb.h>
#include <lb/RoundRobin.hpp>
#include <lb/LeastConn.hpp>

Server::Server(Config config) {
    std::cout << config.listeningHost << "\n";
    std::cout << config.listeningPort << "\n";
    std::cout << config.lbStrategy << "\n";
    
    upstreamServers.resize((size_t)config.upstreams.size());
    for(ssize_t i = 0; i < (ssize_t)config.upstreams.size(); i++) {
        upstreamServers[i] = new UpstreamServer();
        upstreamServers[i]->host = config.upstreams[i].host;
        upstreamServers[i]->port = config.upstreams[i].port;
    }

    // better to have a factory to avoid OCP violation but deliberately leaving that part as of now 
    if(config.lbStrategy == "round_robin") {
        lb = new RoundRobin(upstreamServers);
    } else if(config.lbStrategy == "least_connections") {
        lb = new LeastConn(upstreamServers);
    } else {
        throw std::runtime_error("Invalid load balancing strategy");
    }

    healthChecker = new HealthChecker(upstreamServers, 2);
    healthChecker->start();

    // storing config - IPv4 address container
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addrLen = sizeof(addr);
}

void Server::init() {
    std::cout << "-----Starting server at 127.0.0.1-----" << "\n";

    fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

     if(fileDescriptor == -1) {
        std::cerr << "[ERROR]: " << "Cannot open socket at this port." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    }

    int opt = 1;
    int flag = setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // flag should be 0 on success 
    if(flag == -1) {
        std::cerr << "[ERROR]: " << "Socket configuration failed to set." << "\n";
        throw std::runtime_error(strerror(errno));
    }

    EpollEngine* epollEngine = EpollEngine::getInstance();
    if(epollEngine) {
        epollEngine->addObserver(fileDescriptor);
        epollEngine->modifyObserver(fileDescriptor, EPOLLIN); // because server is just a listening socket
    } else {
        throw std::runtime_error("Server initialisation failed due to null epoll engine.");
    }

    // making listening socket non blocking so that accept() is non blocking op
    fcntl(fileDescriptor, F_SETFL, O_NONBLOCK);

    addrLen = sizeof(addr);

    if(bind(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), addrLen) == -1) {
        std::cerr << "[ERROR]: " << "Binding error." << "\n";
        throw std::runtime_error(strerror(errno));
        // EADDRINUSE : address already in use error comes here, solution -> SO_RESUSEADDR SO_REUSEPORT
    }
    if(listen(fileDescriptor, 5) == -1) {
        // if more than 5 connections come in OS networking queue for this socket, will show ECONNREFUSED
        std::cerr << "[ERROR]: " << "Cannot start listening on this port." << "\n";
        throw std::runtime_error(strerror(errno));
    }
    
    std::cout << "-----Server started listening on port 8080, Ready to accept connections-----" << "\n";
    std::cout << "=========================================" << "\n";
} 

void Server::runEventLoop() {

    std::vector<epoll_event> readyEvents;
    int numberOfReadySockets;
    while(true) {
        std::pair<std::vector<epoll_event>, int> res = EpollEngine::getInstance()->fetchReadySockets(1000);  
        readyEvents = res.first;
        numberOfReadySockets = res.second;
        if(numberOfReadySockets > 0) {
            for(size_t i = 0; i < numberOfReadySockets; i++) {
                handleEvent(readyEvents[i]);
            }
        }
    }
}

// echo "hello" | nc 127.0.0.1 8080

void Server::handleEvent(const epoll_event event) {

    if(event.data.fd == fileDescriptor) {
        if(event.events & EPOLLIN) {
            handleAcceptEvent();
        } else {
            throw std::runtime_error("Listening socket internal error");
        }
    } else if(clientFDs.find(event.data.fd) != clientFDs.end()) {
        // handling client FD

        if(event.events & (EPOLLERR | EPOLLHUP)) {
            sendAllBytes(event.data.fd, (ssize_t)HttpResponse::bad_gateway_502().length(), HttpResponse::bad_gateway_502());
            shutdownConnection(event.data.fd);
        } else if(event.events & EPOLLIN) {
            handleReadEvent(event.data.fd);

            if(connectedSockets.find(event.data.fd) != connectedSockets.end()) {
                ParseResult parseRes = connectedSockets.at(event.data.fd)->parse();
                
                if(parseRes.status == COMPLETE) {
                    // parsing done so now make upstream FD to be written (sockets are always ready to be written but gotta check whether connection to upstream done or not)
                    int32_t upstreamFD = connectedSockets.at(event.data.fd)->getUpstreamFD();
                    EpollEngine::getInstance()->modifyObserver(upstreamFD, (EpollEngine::getDefaultEvents() ^ EPOLLIN) | EPOLLOUT);
                } else if(parseRes.status == ERROR) {
                    sendAllBytes(event.data.fd, (ssize_t)HttpResponse::bad_rquest_400().length(), HttpResponse::bad_rquest_400());
                    shutdownConnection(event.data.fd);
                } 
                
            }
        } else if(event.events & EPOLLOUT) {
            std::string dataToBeSent = connectedSockets.at(event.data.fd)->getUpstreamBuffer();
            if((int)dataToBeSent.length() > 0) {
                sendAllBytes(event.data.fd, dataToBeSent.length(), dataToBeSent);

                EpollEngine::getInstance()->modifyObserver(event.data.fd, EpollEngine::getDefaultEvents());

                connectedSockets.at(event.data.fd)->setUpstreamBuffer("");
                connectedSockets.at(event.data.fd)->setClientBuffer("");
            } 
        }
    } else {
        // Handling upstream FD
        if(event.events & (EPOLLERR | EPOLLHUP)) {
            sendAllBytes(connectedSockets.at(event.data.fd)->getConnectedSocketFD(), (ssize_t)HttpResponse::bad_gateway_502().length(), HttpResponse::bad_gateway_502());
            shutdownConnection(connectedSockets.at(event.data.fd)->getConnectedSocketFD());
        } else if(event.events & EPOLLIN) {
            handleUpstreamReadEvent(event.data.fd);

            if(connectedSockets.find(event.data.fd) != connectedSockets.end() && connectedSockets.at(event.data.fd)->getUpstreamFD() == -1) {
                int32_t clientFD = connectedSockets.at(event.data.fd)->getConnectedSocketFD();
                EpollEngine::getInstance()->modifyObserver(clientFD, (EpollEngine::getDefaultEvents() ^ EPOLLIN) | EPOLLOUT );
                connectedSockets.erase(event.data.fd);
            }
        } else if(event.events & EPOLLOUT) {
            if(!connectedSockets.at(event.data.fd)->getUpstreamConnectedFlag()) {
                int error = -1;
                socklen_t errorlen = sizeof(error);
                getsockopt(connectedSockets.at(event.data.fd)->getUpstreamFD(), SOL_SOCKET, SO_ERROR, &error, &errorlen);
                if(error != 0) {
                    // connection not succeeded
                    sendAllBytes(connectedSockets.at(event.data.fd)->getConnectedSocketFD(), (ssize_t)HttpResponse::bad_gateway_502().length(), HttpResponse::bad_gateway_502());
                    shutdownConnection(connectedSockets.at(event.data.fd)->getConnectedSocketFD());
                    return;
                }
                connectedSockets.at(event.data.fd)->setUpstreamConnectedFlag(true);
            }
            
            std::string dataToBeSent = connectedSockets.at(event.data.fd)->getClientBuffer();
            if((int)dataToBeSent.length() > 0) {
                sendAllBytes(event.data.fd, dataToBeSent.length(), dataToBeSent);
                EpollEngine::getInstance()->modifyObserver(event.data.fd, EpollEngine::getDefaultEvents());
            }
        }
    }
}

void Server::handleAcceptEvent() {
    
    int32_t connectedSocketFD = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    while(connectedSocketFD != -1) {
        fcntl(connectedSocketFD, F_SETFL, O_NONBLOCK);
        std::shared_ptr<ConnectionState> socketState= std::make_shared<ConnectionState>(connectedSocketFD);
        connectedSockets.insert({connectedSocketFD, socketState});
        clientFDs.insert(connectedSocketFD);
        EpollEngine::getInstance()->addObserver(connectedSocketFD);

        std::cout << "[LOG]: CLient connected: " << connectedSocketFD << "\n";

        UpstreamServer* upstreamServer = lb->selectBackend();
        if(upstreamServer == nullptr) {
            sendAllBytes(connectedSocketFD, (ssize_t)HttpResponse::service_unavailable_503().length(), HttpResponse::service_unavailable_503());
            shutdownConnection(connectedSocketFD);
        } else {
            int upstreamFD = connectUpstream(upstreamServer->host, upstreamServer->port);

            if(upstreamFD == -1) {
                lb->onConnectionClosed(upstreamServer);
                sendAllBytes(connectedSocketFD, (ssize_t)HttpResponse::service_unavailable_503().length(), HttpResponse::service_unavailable_503());
                shutdownConnection(connectedSocketFD);
            } else {
                socketState->setUpstreamServer(upstreamServer);
                socketState->setUpstreamFD(upstreamFD);
                connectedSockets.insert({upstreamFD, socketState});
                EpollEngine::getInstance()->addObserver(upstreamFD);
                std::cout << "[LOG]: Upstream connection instantiated: " << upstreamFD << "\n";
            }  
        }
        

        connectedSocketFD = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    }
}

void Server::handleReadEvent(const int32_t connectedSocketFD) {
    std::shared_ptr<ConnectionState> socketState = connectedSockets.at(connectedSocketFD);
    
    ssize_t msgSizeRec = recv(connectedSocketFD, socketState->getReadBuffer(), 1024, 0);
    if(msgSizeRec == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        shutdownConnection(connectedSocketFD);
        return;
    }
    while(msgSizeRec != -1) {

        if(msgSizeRec == 0) {
            std::cout << "[LOG]: FIN received from client. Client: " <<  connectedSocketFD << " disconnected." << "\n";
            
            shutdownConnection(connectedSocketFD);
            break;
        } 
        socketState->getReadBuffer()[msgSizeRec] = '\0';

        std::string data = socketState->getClientBuffer();
        data = data + socketState->getReadBuffer();
        socketState->setClientBuffer(data);

        msgSizeRec = recv(connectedSocketFD, socketState->getReadBuffer(), 1024, 0);

        if(msgSizeRec == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // expected, done reading
            sendAllBytes(connectedSocketFD, (ssize_t)HttpResponse::bad_gateway_502().length(), HttpResponse::bad_gateway_502());
            shutdownConnection(connectedSocketFD); // real error
            return;
        }
    }

    if(connectedSockets.find(connectedSocketFD) != connectedSockets.end()) {
        std::cout << "[LOG]: Data received from client - " << socketState->getClientBuffer() << "\n";
    }
}

void Server::handleUpstreamReadEvent(const int32_t upstreamFD) {
    std::shared_ptr<ConnectionState> socketState = connectedSockets.at(upstreamFD);
    
    ssize_t msgSizeRec = recv(upstreamFD, socketState->getReadBuffer(), 1024, 0);
    if(msgSizeRec == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        shutdownConnection(socketState->getConnectedSocketFD());
        return;
    }
    while(msgSizeRec != -1) {

        if(msgSizeRec == 0) {
            std::cout << "[LOG]: FIN received from upstream. Upstream: " <<  upstreamFD << " disconnected." << "\n";
            EpollEngine::getInstance()->removeObserver(upstreamFD);
            socketState->setUpstreamFD(-1);
            close(upstreamFD);
            break;
        } 
        socketState->getReadBuffer()[msgSizeRec] = '\0';

        std::string data = socketState->getUpstreamBuffer();
        data = data + socketState->getReadBuffer();
        socketState->setUpstreamBuffer(data);

        msgSizeRec = recv(upstreamFD, socketState->getReadBuffer(), 1024, 0);

        if(msgSizeRec == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // expected, done reading
            sendAllBytes(socketState->getConnectedSocketFD(), (ssize_t)HttpResponse::bad_gateway_502().length(), HttpResponse::bad_gateway_502());
            shutdownConnection(socketState->getConnectedSocketFD());              // real error
            return;
        }
    }

    if(msgSizeRec != 0) {
        std::cout << "---INCOMPLETE DATA FROM UPSTREAM (NO FIN)---" << "\n";
    } else if(msgSizeRec == 0) {
        std::cout << "---COMPLETE DATA FROM UPSTREAM---" << "\n";
    }
    if(connectedSockets.find(upstreamFD) != connectedSockets.end()) {
        std::cout << "[LOG]: Data received from upstream - " << socketState->getUpstreamBuffer() << "\n";
    }
}

void Server::sendAllBytes(int connectedSocket, ssize_t bytesToSend, const std::string &data) {
    ssize_t totalBytesSent = 0;
    while(totalBytesSent != bytesToSend) {
        int bytesSent = send(connectedSocket, &data[0] + totalBytesSent, bytesToSend - totalBytesSent, 0);
        if(bytesSent == -1) {
            std::cerr << "[ERROR]: " << "Could not send message." << "\n";
            throw std::runtime_error(strerror(errno));
        } else {
            std::cout << "[LOG]: Size of message sent: " << bytesSent << "\n";
        }

        totalBytesSent += bytesSent;
    }
}

int32_t Server::connectUpstream(std::string &host, int32_t port) {
    std::cout << "[LOG]: Creating upstream socket to connect with backend" << "\n";

    int32_t upstreamFD = socket(AF_INET, SOCK_STREAM, 0);
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

    return upstreamFD; // if returned -1 will catch it in server's event loop
}

void Server::shutdownAllConnections() {
    for(auto &connection: connectedSockets) {
        if(connection.second->getUpstreamFD() != -1) {
            EpollEngine::getInstance()->removeObserver(connection.second->getUpstreamFD());
            close(connection.second->getUpstreamFD());
        }
        EpollEngine::getInstance()->removeObserver(connection.second->getConnectedSocketFD());
        close(connection.second->getConnectedSocketFD());
    }
}

void Server::shutdownConnection(const int fd) {
    std::shared_ptr<ConnectionState> socketState = connectedSockets.at(fd);

    int32_t upstreamFD = socketState->getUpstreamFD();

    if(upstreamFD != -1) {
        EpollEngine::getInstance()->removeObserver(socketState->getUpstreamFD());
        lb->onConnectionClosed(socketState->getUpstreamServer());
        connectedSockets.erase(upstreamFD);
        close(socketState->getUpstreamFD());
    }

    EpollEngine::getInstance()->removeObserver(fd);
    connectedSockets.erase(fd);
    clientFDs.erase(fd);
    close(fd);
}

Server::~Server() {
    std::cout << "Closing server..." << "\n";
    healthChecker->stop();
    shutdownAllConnections();
    close(fileDescriptor);
    for(int i = 0 ; i < (int)upstreamServers.size(); i++) {
        delete upstreamServers[i];
    }
    delete lb;
    delete healthChecker;
}