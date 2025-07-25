#include <vector>
#include "header/spec.hpp"

#define Byte unsigned char

std::vector<Byte> ram;

// RAMおよびVRAMを管理する関数
/*

Byte vram_peek(std::vector<Byte>& vram, int addr) {
    if (addr < 0) {
        return Byte(0);
    }
    if (addr < S3HS_VRAM_SIZE) {
        return vram.at(addr);
    }
    return Byte(0);
}

void vram_poke(std::vector<Byte>& vram, int addr, Byte val) {
    if (addr < 0) {
        return;
    }   
    if (addr < S3HS_VRAM_SIZE) {
        vram.at(addr) = val;
    }
}

std::vector<Byte> vram_peek2array(std::vector<Byte>& vram, int addr, int block) {
    std::vector<Byte> out;
    for (int i = addr; i < addr + block; i++)
    {
        out.push_back(vram_peek(vram, i));
    }
    return out;
}

void vram_pokefill(std::vector<Byte>& vram, int addr, int block, Byte val) {
    std::fill(vram.begin() + addr, vram.begin() + addr + block, val);
}

void vram_poke2array(std::vector<Byte>& vram, int addr, std::vector<Byte>& vals) {
    std::copy(vals.begin(), vals.end(), vram.begin() + addr);
} */

void ram_boot(std::vector<Byte>& ram) {
    ram.resize(S3HS_RAM_SIZE, Byte(0));
}

Byte ram_peek(std::vector<Byte>& ram, int addr) {
    if (addr < 0) {
        return Byte(0);
    }
    if (addr < S3HS_RAM_SIZE) {
        return ram[addr];
    }
    return Byte(0);
}

void ram_poke(std::vector<Byte>& ram, int addr, Byte val) {
    if (addr < 0) {
        return;
    }   
    if (addr < S3HS_RAM_SIZE) {
        ram[addr] = val;
    }
}

std::vector<Byte> ram_peek2array(std::vector<Byte>& ram, int addr, int block) {
    std::vector<Byte> out;
    for (int i = addr; i < addr + block; i++)
    {
        out.push_back(ram_peek(ram, i));
    }
    return out;
}

void ram_pokefill(std::vector<Byte>& ram, int addr, int block, Byte val) {
    std::fill(ram.begin() + addr, ram.begin() + addr + block, val);
}

void ram_poke2array(std::vector<Byte>& ram, int addr, std::vector<Byte>& vals) {
    if (addr < 0) return;
    if (ram.size() < addr + vals.size()) {
        ram.resize(addr + vals.size(), Byte(0));
    }
    std::copy(vals.begin(), vals.end(), ram.begin() + addr);
}

