#include <models/lb/HealthChecker.hpp>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

HealthChecker::HealthChecker(std::vector<UpstreamServer*> upstreamServers,int interval) {
    this->upstreamServers = upstreamServers;
    this->interval = interval;
}

void HealthChecker::start() {
    healthThread = std::thread(&HealthChecker::checkLoop, this);
}

void HealthChecker::checkLoop() {
    while(!stop_) {
        for(int i = 0; i < (int)upstreamServers.size(); i++) {
            checkOne(upstreamServers[i]);
        }
        // now sleep using conditional variable + mutex
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(interval), [this] { return this->stop_.load(); });
    }   
}

void HealthChecker::checkOne(UpstreamServer* server) {
    
    int32_t upstreamFD = socket(AF_INET, SOCK_STREAM, 0);
    if(upstreamFD != -1) {
        fcntl(upstreamFD, F_SETFL, O_NONBLOCK);
        addrinfo* result;
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int status = getaddrinfo(server->host.c_str(), std::to_string(server->port).c_str(), &hints, &result);
        if(status != 0) { 
            close(upstreamFD); 
            freeaddrinfo(result);
            return;
        }

        int upstreamConnectionStatus = connect(upstreamFD, result->ai_addr, result->ai_addrlen);
        if(upstreamConnectionStatus == 0) {
            server->isHealthy = true;
        } else if(upstreamConnectionStatus == -1 && errno != EINPROGRESS) {
            server->isHealthy = false;
        } else if(upstreamConnectionStatus == -1 && errno == EINPROGRESS) {
            struct pollfd fd[1];
            fd[0].fd = upstreamFD;
            fd[0].events = POLLOUT;
            fd[0].revents = 0;

            int ret = poll(fd, 1, 3000);
            if(ret == -1) {
                // poll error, health check failed so skip health check now    
            } else if(ret == 0) {
                // unhealthy as backend taking too long
                server->isHealthy = false;
            } else {
                int error = -1;
                socklen_t errorlen = sizeof(error);
                int optstatus = getsockopt(upstreamFD, SOL_SOCKET, SO_ERROR, &error, &errorlen);
                if(optstatus == -1) {
                    // socket opt error, health checck failed so skip health check now
                } else if(error != 0) {
                    server->isHealthy = false;
                } else if(error == 0) {
                    server->isHealthy = true;
                }
            }
        }
        close(upstreamFD);
        freeaddrinfo(result);
    }
}

void HealthChecker::stop() {
    stop_ = true;
    cv.notify_one();
    if(healthThread.joinable()) {
        healthThread.join();
    }
}