#pragma once
#include <Windows.h>  
#include <fstream> 
#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>
#include "AppAllocator.h"

using namespace Live2D::Cubism::Framework;

class Live2DManager {
public:
    static Live2DManager* GetInstance() {
        if (s_instance == nullptr) {
            s_instance = new Live2DManager();
        }
        return s_instance;
    }

    void Initialize() {
        if (_isInitialized) return;

        _allocator = new LAppAllocator();
        
        Csm::CubismFramework::Option option;
        option.LogFunction = PrintLog;
        option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

        Csm::CubismFramework::StartUp(_allocator, &option);
        Csm::CubismFramework::Initialize();

        _isInitialized = true;
    }

    static void PrintLog(const Csm::csmChar* message) {
        OutputDebugStringA(message);
    }

private:
    Live2DManager() : _allocator(nullptr), _isInitialized(false) {}
    
    static Live2DManager* s_instance;
    LAppAllocator* _allocator;
    bool _isInitialized;
};

// Static member definition
__declspec(selectany) Live2DManager* Live2DManager::s_instance = nullptr;