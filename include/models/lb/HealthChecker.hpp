#pragma once 
#include <vector>
#include <models/lb/UpstreamServer.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

class HealthChecker {
    private:
        std::vector<UpstreamServer*> upstreamServers;
        int interval;
        std::thread healthThread;
        std::atomic<bool> stop_ = false;
        std::mutex mtx;
        std::condition_variable cv;

    public:
        HealthChecker(std::vector<UpstreamServer*> upstreamServers,int interval);
        void start();
        void checkLoop();
        void checkOne(UpstreamServer* server);
        void stop();    
};