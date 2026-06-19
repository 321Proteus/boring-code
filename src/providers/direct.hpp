#pragma once

#include <capstone/capstone.h>
#include "core/address.hpp"
#include "core/view.hpp"
#include "data/provider.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class MappedFile {
public:
    MappedFile(const std::string& path);
    ~MappedFile();

    const uint8_t* data() const { return data_; };
    size_t size() const { return size_; };

private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;

    struct Impl;
    Impl* impl_;
};

struct BCFileSection {
    uint64_t va;
    uint32_t offset;
    uint32_t raw_size;
    uint32_t virtual_size;
};

class DirectCodeProvider : public BCCodeProvider {
private:

    std::vector<BCFileSection> sections;
    std::shared_ptr<MappedFile> file;
    csh handle;
    BCAddr image_base;
    bool is_x64;

    bool is_bb_end(cs_insn* instr) const {
        if (!instr || !instr->detail) return false;
        for (int i=0;i<instr->detail->groups_count;i++) {
            uint8_t group = instr->detail->groups[i];
            if (
                group == CS_GRP_BRANCH_RELATIVE ||
                group == CS_GRP_CALL || 
                group == CS_GRP_IRET ||
                group == CS_GRP_JUMP ||
                group == CS_GRP_RET ||
                group == CS_GRP_INT
            ) return true;
        }
        return false;
    }

    bool has_imm_target(cs_insn* insn) const {

        if (!insn || !insn->detail) return false;

        for (int i=0;i<insn->detail->groups_count;i++) {
            uint8_t g = insn->detail->groups[i];
            if (g == CS_GRP_JUMP || g == CS_GRP_CALL || g == CS_GRP_BRANCH_RELATIVE) {
                const auto& ops = insn->detail->x86;
                if (ops.op_count > 0 && ops.operands[0].type == X86_OP_IMM) return true;
            }
        }
        return false;

    }

    template<typename T>
    static T read(const uint8_t* p, bool big_endian) {
        T a = 0;
        if (big_endian) for (int i=0;i<sizeof(T);i++) a = (a << 8) | p[i];
        else memcpy(&a, p, sizeof(T));
        return a;
    }

    void sort_finish_parse() {

        if (sections.empty()) {
            throw std::runtime_error("Warning: Parsing succeeded, but no code sections were found. This usually indicates a corrupted binary");
            return;
        }

        std::sort(sections.begin(), sections.end(),
            [](const BCFileSection& a, const BCFileSection& b) {
                return a.va < b.va;
            });

        printf("Parsed %zu sections ranging from %llX to %llX\n", sections.size(), sections[0].va, sections.back().va + sections.back().virtual_size);
    }

public:

    DirectCodeProvider(std::shared_ptr<MappedFile> file, BCAddr start, BCAddr end)
        : BCCodeProvider(start, end), file(std::move(file)) {}

    void parse_pe() {

        if (file->size() < 0x100) throw std::runtime_error("File too small");

        uint32_t pe_offset;
        memcpy(&pe_offset, file->data() + 60, sizeof(pe_offset));

        if (pe_offset + 4 > file->size()) throw std::runtime_error("Bad PE offset");
        if (memcmp(file->data() + pe_offset, "PE\0\0", 4) != 0) throw std::runtime_error("Not a PE file");

        const uint8_t* coff = file->data() + pe_offset + 4;

        uint16_t section_count, size_of_opt_header;

        memcpy(&size_of_opt_header, coff + 16, 2);
        memcpy(&section_count, coff + 2, 2);

        const uint8_t* opt = coff + 20;
        uint16_t pe_magic; memcpy(&pe_magic, opt, 2);
        
        if (pe_magic == 0x10B) {
            is_x64 = false;
            uint32_t base32;
            memcpy(&base32, opt + 0x1C, 4);
            image_base = base32; 
        } else if (pe_magic == 0x20B) {
            is_x64 = true;
            memcpy(&image_base, opt + 0x18, 8); 
        } else {
            throw std::runtime_error("Incorrect PE optional header magic");
        }

        const uint8_t* section_table = opt + size_of_opt_header;

        for (uint16_t i=0;i<section_count;i++) {

            const uint8_t* s = section_table + i * 40;
            BCFileSection section {};

            memcpy(&section.virtual_size,   s + 8,  4);  // Misc.VirtualSize
            memcpy(&section.va,             s + 12, 4);  // VirtualAddress
            memcpy(&section.raw_size,       s + 16, 4);  // SizeOfRawData
            memcpy(&section.offset,         s + 20, 4);  // PointerToRawData

            if (section.raw_size != 0) sections.push_back(section);
        
        }

        sort_finish_parse();

    }

    void parse_elf() {

        if (file->size() < 16) throw std::runtime_error("File too small");
        
        is_x64      = file->data()[4] == 0x2;
        bool is_be  = file->data()[5] == 0x2;

        const size_t phoff_off      = is_x64 ? 32 : 28;
        const size_t phentsize_off  = is_x64 ? 54 : 42;
        const size_t phnum_off      = is_x64 ? 56 : 44;

        if (file->size() < phnum_off + 2) throw std::runtime_error("ELF header truncated");

        BCAddr ph_offset = is_x64 ? read<uint64_t>(file->data() + phoff_off, is_be)
                                    : read<uint32_t>(file->data() + phoff_off, is_be);

        uint16_t ph_count  = read<uint16_t>(file->data() + phnum_off, is_be);
        uint16_t ph_entry  = read<uint16_t>(file->data() + phentsize_off, is_be);
        
        for (int i=0;i<ph_count;i++) {
            const uint8_t* ph = file->data() + ph_offset + i * ph_entry;
            if (ph + ph_entry > file->data() + file->size()) {
                throw std::runtime_error("Entry header is malformed");
            }
            uint32_t p_type = read<uint32_t>(ph, is_be);
            if (p_type != 1) continue; // PT_LOAD == 1

            BCFileSection entry {};
            if (is_x64) {
                entry.offset        = read<uint64_t>(ph + 8,  is_be); // p_offset
                entry.va            = read<uint64_t>(ph + 16, is_be); // p_vaddr
                entry.raw_size      = read<uint64_t>(ph + 32, is_be); // p_filesz
                entry.virtual_size  = read<uint64_t>(ph + 40, is_be); // p_memsz
            } else {
                entry.offset        = read<uint32_t>(ph + 4,  is_be); // p_offset
                entry.va            = read<uint32_t>(ph + 8,  is_be); // p_vaddr
                entry.raw_size      = read<uint32_t>(ph + 16, is_be); // p_filesz
                entry.virtual_size  = read<uint32_t>(ph + 20, is_be); // p_memsz
            }
            sections.push_back(entry);
        }

        sort_finish_parse();

    }

    ptrdiff_t rva_to_file_offset(uint64_t rva) const {

        int target_section = -1;

        int l = 0; int r = sections.size() - 1;

        while (l <= r) {
            int m = (l+r) / 2;
            const BCFileSection& sec = sections[m];

            if (rva < sec.va) {
                r = m-1;
            } else if (rva >= sec.va + sec.raw_size) {
                l = m+1;
            } else {
                uint64_t delta = rva - sec.va;
                if (delta >= sec.raw_size) return -1;
                target_section = m;
                break;
            }
        }
        
        if (target_section != -1) {
            const BCFileSection& sec = sections[target_section];
            uint64_t delta = rva - sec.va;
            if (delta >= sec.raw_size) return -1;
            return sec.offset + delta;
        }

        return -1;

    }

    void init_cs_handler() {
        cs_open(CS_ARCH_X86, is_x64 ? CS_MODE_64 : CS_MODE_32, &handle);
        cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    }

    std::vector<uint8_t> get_instructions(BCAddr runtime_base, BCAddr va, size_t count) override {

        if (va < runtime_base) return {};
        size_t rva = va - runtime_base;
        ptrdiff_t offset = rva_to_file_offset(rva);

        if (offset < 0 || static_cast<size_t>(offset) + count > file->size()) return {};
        
        const uint8_t* start = file->data() + offset;
        return std::vector<uint8_t>(start, start + count);

    }

    std::vector<BCInstruction> get_bb(BCAddr va) override {

        std::vector<BCInstruction> v;

        if (va < start) return v;

        ptrdiff_t rva = va - start;
        ptrdiff_t offset = rva_to_file_offset(rva);

        printf("VA 0x%lX, RVA 0x%lX, offset 0x%lX\n", va, rva, offset);
        if (offset < 0 || static_cast<size_t>(offset) > file->size()) return v;

        const uint8_t* ptr = file->data() + offset;
        size_t size = file->size() - offset;

        cs_insn* insn;  

        while (true) {

            size_t count = cs_disasm(handle, ptr, size, va, 1, &insn);
            if (count == 0) break;

            BCInstruction inst = {
                va, insn->size, insn->mnemonic, insn->op_str
            };


            ptr += insn->size;
            size -= insn->size;
            va += insn->size;

            bool end  = is_bb_end(insn);

            if (end && has_imm_target(insn)) {
                uint64_t target = insn->detail->x86.operands[0].imm;
                if (!is_x64) target = (uint32_t)target;
                inst.op_str = to_hex(target);
            }

            v.push_back(std::move(inst));
            cs_free(insn, count);
            if (end) break;

        }

        return v;

    }

    ~DirectCodeProvider() {
        cs_close(&handle);
    }

};

BCCodeProviderRegistry resolve_modules(const std::string& path, BCDatabase& db, BCStatusViewModel& sv);