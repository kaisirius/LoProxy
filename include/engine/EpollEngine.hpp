#pragma once
#include <sys/epoll.h>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

class EpollEngine {
    private:
        static std::unique_ptr<EpollEngine> epollEngInstance;
        static std::mutex mtx;
        int epollEngFD;
        uint32_t defaultEvents;
        std::set<int> registeredFDs;
        epoll_event createTemplateEventStruct(int fileDescriptor);
        EpollEngine();

    public:
        EpollEngine(const EpollEngine& EpollEngine) = delete;
        EpollEngine& operator = (const EpollEngine& EpollEngine) = delete;
        static EpollEngine* getInstance();
        void addObserver(const int fileDescriptor);
        void removeObserver(const int fileDescriptor);
        void modifyObserver(const int fileDescriptor,const uint32_t events);
        std::vector<epoll_event> fetchReadySockets(int timeoutMs);
        ~EpollEngine();
};