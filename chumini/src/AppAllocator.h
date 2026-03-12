#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>
#include <malloc.h>

// 名前空間の指定
using namespace Live2D::Cubism::Framework;

// Live2D用のメモリアロケータ
class LAppAllocator : public ICubismAllocator {
public:
    // ★修正箇所: csmSize を csmSizeType に変更
    void* Allocate(const csmSizeType size) override {
        return malloc(size);
    }

    // メモリ解放
    void Deallocate(void* memory) override {
        free(memory);
    }

    // ★修正箇所: csmSize を csmSizeType に変更
    void* AllocateAligned(const csmSizeType size, const csmUint32 alignment) override {
        return _aligned_malloc(size, alignment);
    }

    // アライメント付きメモリ解放
    void DeallocateAligned(void* alignedMemory) override {
        _aligned_free(alignedMemory);
    }
};