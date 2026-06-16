#include "direct.hpp"
#include "core/loader.hpp"
#include <cstdint>
#include <stdexcept>

#ifndef _WIN32

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

struct MappedFile::Impl {
    int fd;
};

MappedFile::MappedFile(const std::string& path)
    : impl_(new Impl()) {
    impl_->fd = open(path.c_str(), O_RDONLY);
    if (impl_->fd == -1) throw std::runtime_error("Invalid path: \"" + path + "\"");
    size_ = lseek(impl_->fd, 0, SEEK_END);
    data_ = (uint8_t*)mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, impl_->fd, 0);
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        throw std::runtime_error("mmap failed for path: \"" + path + "\"");
    }
}

MappedFile::~MappedFile() {
    if (impl_) {
        if (data_) munmap(data_, size_);
        if (impl_->fd != -1) close(impl_->fd);
        delete impl_;
    } 

}

#else

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

BCCodeProviderRegistry resolve_modules(BCDatabase& db, BCStatusViewModel& sv) {
    BCCodeProviderRegistry registry;

    sv.setup_job("Resolving modules", db.store.modules().size());
    int mr_progress = 0;

    for (const auto& mod : db.store.modules()) {
        BCFileType type = detect_type(mod->path);
        if (type == BCFileType::UNKNOWN) {
            printf("Failed to register module %s (not a module)\n", mod->path.c_str());
            continue;
        }

        try {
            std::shared_ptr<MappedFile> file = std::make_shared<MappedFile>(mod->path);
            std::unique_ptr<DirectCodeProvider> provider = std::make_unique<DirectCodeProvider>(file, mod->start, mod->end);

            if (type == BCFileType::PE)         provider->parse_pe();
            else if (type == BCFileType::ELF)   provider->parse_elf();

            provider->init_cs_handler();

            registry.register_provider(*mod, std::move(provider));
            printf("Successfully registered module %s\n", mod->path.c_str());

        } catch (const std::runtime_error& e) {
            printf("Failed to register module %s\n", e.what());
        }

        sv.update_job_progress(mr_progress++);
    }
    return registry;
}