#pragma once
#include <sys/epoll.h>
#include <memory>
#include <mutex>

class EpollEngine {
    private:
        static std::unique_ptr<EpollEngine> epollEngInstance;
        static std::mutex mtx;
        int epollEngFD;
        EpollEngine();

    public:
        EpollEngine(const EpollEngine& EpollEngine) = delete;
        EpollEngine& operator = (const EpollEngine& EpollEngine) = delete;
        static EpollEngine* getInstance();
        ~EpollEngine();
};