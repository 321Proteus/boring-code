#ifndef _WIN32

#include <cstdint>
#include "direct.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

struct MappedFile::Impl {
    int fd;
};

MappedFile::MappedFile(const std::string& path)
    : impl_(new Impl()) {
    impl_->fd = open(path.c_str(), O_RDONLY);
    size_ = lseek(impl_->fd, 0, SEEK_END);
    data_ = (uint8_t*)mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, impl_->fd, 0);
}

MappedFile::~MappedFile() {
    if (impl_) {
        if (data_) munmap(data_, size_);
        if (impl_->fd != -1) close(impl_->fd);
        delete impl_;
    } 

}

#endif