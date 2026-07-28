#include <engine/EpollEngine.hpp>
#include <iostream> 
#include <string.h>
#include <errno.h>
#include <unistd.h>


std::unique_ptr<EpollEngine> EpollEngine::epollEngInstance = nullptr;
std::mutex EpollEngine::mtx;
uint32_t EpollEngine::defaultEvents = EPOLLIN | EPOLLERR | EPOLLHUP;

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

epoll_event EpollEngine::createTemplateEventStruct(int fileDescriptor) {

    epoll_event templateEvent = epoll_event();
    templateEvent.events = defaultEvents;
    templateEvent.data.fd = fileDescriptor;

    return templateEvent;
}

void EpollEngine::addObserver(const int fileDescriptor) {

    if(registeredFDs.find(fileDescriptor) != registeredFDs.end()) {
        std::cout << "[WARN]: Socket " << fileDescriptor << " " << "already exists in epoll engine." << "\n";
        return;
    }

    if(fileDescriptor == epollEngFD) {
        std::cout << "[WARN]: Socket " << fileDescriptor << " " << "is same as epoll engine's FD." << "\n";
        return;
    }
    
    epoll_event event = createTemplateEventStruct(fileDescriptor);
    int flag = epoll_ctl(epollEngFD, EPOLL_CTL_ADD, fileDescriptor, &event);
    if(flag == -1) {
        std::cerr << "[ERROR]: " << "Cannot register socket to epoll engine." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    } else {
        registeredFDs.insert(fileDescriptor);
        std::cout << "[LOG]: Socket " << fileDescriptor << " " << "successfully added to epoll engine." << "\n";
    }
}

void EpollEngine::removeObserver(const int fileDescriptor) {

    if(registeredFDs.find(fileDescriptor) == registeredFDs.end()) {
        std::cout << "[WARN]: Socket " << fileDescriptor << " " << "does not exist in epoll engine. Nothing to delete." << "\n";
        return;
    }

    int flag = epoll_ctl(epollEngFD, EPOLL_CTL_DEL, fileDescriptor, nullptr);
    if(flag == -1) {
        std::cerr << "[ERROR]: " << "Cannot de-register socket from epoll engine." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    } else {
        registeredFDs.erase(fileDescriptor);
        std::cout << "[LOG]: Socket " << fileDescriptor << " " << "successfully removed from epoll engine." << "\n";
    }
}

void EpollEngine::modifyObserver(const int fileDescriptor, const uint32_t events) {

    if(registeredFDs.find(fileDescriptor) == registeredFDs.end()) {
        std::cout << "[WARN]: Socket " << fileDescriptor << " " << "does not exist in epoll engine. Nothing to modify." << "\n";
        return;
    }

    epoll_event updatedEvents = createTemplateEventStruct(fileDescriptor);
    updatedEvents.events = events;

    int flag = epoll_ctl(epollEngFD, EPOLL_CTL_MOD, fileDescriptor, &updatedEvents);
    if(flag == -1) {
        std::cerr << "[ERROR]: " << "Cannot modify socket from epoll engine." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    } else {
        std::cout << "[LOG]: Socket " << fileDescriptor << " " << "successfully modified in epoll engine." << "\n";
    }
}

std::pair<std::vector<epoll_event>, int> EpollEngine::fetchReadySockets(int timeoutMs) {
    if(timeoutMs < 0) {
        // -1 will lead to infinite loop
        std::cout << "Invalid time in millisecond as an argument." << "\n";
        return {};
    }
    // -1 will be used as flag to check how many ready sockets we received 
    // other work around is to return pair<vector, int>  = (readySockets, numberOfReadySockets)
    std::vector<epoll_event> readySockets(1000, createTemplateEventStruct(-1));

    int numberOfReadySockets = epoll_wait(epollEngFD, &readySockets[0], 1000, timeoutMs);
    return {readySockets, numberOfReadySockets};
}

uint32_t EpollEngine::getDefaultEvents() {
    return defaultEvents;
}

EpollEngine::~EpollEngine() {
    std::cout << "----------Stopping epoll engine----------" << "\n";
    close(epollEngFD);
    // unnecessary to do delete epollEngInstance.get();
}