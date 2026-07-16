#include <engine/EpollEngine.hpp>
#include <iostream> 

std::unique_ptr<EpollEngine> EpollEngine::epollEngInstance = nullptr;
std::mutex EpollEngine::mtx;

EpollEngine::EpollEngine() {
    epollEngFD = epoll_create1(0);
}


EpollEngine* EpollEngine::getInstance() {
    if(epollEngInstance == nullptr) {
        std::lock_guard<std::mutex> lock(mtx);
        if(epollEngInstance == nullptr) {
            epollEngInstance = std::unique_ptr<EpollEngine>(new EpollEngine());
        }
    }
    return epollEngInstance.get();
}

EpollEngine::~EpollEngine() {
    std::cout << "----------Stopping epoll engine----------" << "\n";
    // delete epollEngInstance.get();
}