
#include "Epoll.h"
#include <sys/epoll.h>
#include <unistd.h>

EpollInstance::EpollInstance(){
    epfd = epoll_create1(0);
}

EpollInstance::~EpollInstance(){
    close(epfd);
}

void EpollInstance::addFd(int fd, Callback cb){
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    handlers[fd] = cb;
}

void EpollInstance::run(){
    epoll_event events[1024];

    while(true){
        int n = epoll_wait(epfd, events, 1024, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            handlers[fd](fd);
        }
    }
}
