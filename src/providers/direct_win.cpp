#ifdef _WIN32

#include "direct.hpp"
#include <windows.h>

struct MappedFile::Impl {
    HANDLE file;
    HANDLE mapping;
};

MappedFile::MappedFile(const std::string& path) {
    impl_->file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    size_ = GetFileSize((HANDLE)impl_->file, NULL);

    impl_->mapping = CreateFileMapping((HANDLE)impl_->file, NULL, PAGE_READONLY, 0, 0, NULL);
    data_ = (uint8_t*)MapViewOfFile((HANDLE)impl_->mapping, FILE_MAP_READ, 0, 0, 0);
}

MappedFile::~MappedFile() {
    if (data_) UnmapViewOfFile(data_);
    if (impl_->mapping) CloseHandle((HANDLE)impl_->mapping);
    if (impl_->file) CloseHandle((HANDLE)impl_->file);
}

#endif