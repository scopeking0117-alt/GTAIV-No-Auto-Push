#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#ifndef HELPER_TEST
#include "NativeThread.h"
#include "ScriptHookManager.h"
#endif

namespace {
SRWLOCK lock = SRWLOCK_INIT;
uintptr_t pendingPed, pendingTask;
DWORD requestedAt;
bool pending;
char status[256] = "waiting for game-thread callback";
using Setter = void(__thiscall*)(void*, void*, int);
Setter secondarySetter;
bool initialized;

bool Readable(uintptr_t address, size_t size, bool executable = false) {
    MEMORY_BASIC_INFORMATION memory = {};
    if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) ||
        memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    uintptr_t end = reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize;
    if (address > end || size > end - address) return false;
    DWORD protection = memory.Protect & 0xFF;
    return executable
        ? protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE ||
            protection == PAGE_EXECUTE_WRITECOPY
        : protection == PAGE_READONLY || protection == PAGE_READWRITE ||
            protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

bool Read(uintptr_t address, uintptr_t& value) {
    SIZE_T count = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        &value, sizeof(value), &count) && count == sizeof(value);
}

Setter FindSetter() {
    // Complete analyzed routine, except the relative displacement of its internal call.
    const unsigned char signature[] = {
        0x8B,0x44,0x24,0x08,0x53,0x8B,0xD9,0x8B,0x4C,0x83,0x14,0x56,0x8D,0x34,0x83,0x57,
        0x8B,0x7C,0x24,0x10,0x3B,0xCF,0x74,0x44,0x85,0xC9,0x74,0x06,0x8B,0x01,0x6A,0x01,
        0xFF,0x10,0x57,0x8B,0xCB,0x89,0x7E,0x14,0xE8,0,0,0,0,0x8B,0x46,0x14,0x85,0xC0,
        0x74,0x28,0x8B,0xC8,0x8B,0x40,0x08,0x85,0xC0,0x75,0xF7,0x8B,0x01,0x8B,0x40,0x08,
        0xFF,0xD0,0x84,0xC0,0x75,0x14,0x8B,0x4E,0x14,0x85,0xC9,0x74,0x06,0x8B,0x01,0x6A,
        0x01,0xFF,0x10,0xC7,0x46,0x14,0,0,0,0,0x5F,0x5E,0x5B,0xC2,0x08,0
    };
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    auto sections = IMAGE_FIRST_SECTION(nt);
    uintptr_t found = 0;
    for (unsigned section = 0; section < nt->FileHeader.NumberOfSections; ++section) {
        if (!(sections[section].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        uintptr_t start = base + sections[section].VirtualAddress;
        size_t length = sections[section].Misc.VirtualSize;
        std::vector<unsigned char> sectionBytes(length);
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(start),
                sectionBytes.data(), length, &bytesRead) || bytesRead != length) continue;
        for (size_t offset = 0; offset + sizeof(signature) <= length; ++offset) {
            uintptr_t address = start + offset;
            auto bytes = sectionBytes.data() + offset;
            if (bytes[0] != signature[0]) continue;
            bool matches = true;
            for (size_t i = 0; i < sizeof(signature); ++i) {
                if (i >= 41 && i <= 44) continue;
                if (bytes[i] != signature[i]) { matches = false; break; }
            }
            if (!matches) continue;
            if (!Readable(address, sizeof(signature), true)) continue;
            if (found) return nullptr;
            found = address;
        }
    }
    return reinterpret_cast<Setter>(found);
}

bool IsShove(uintptr_t task, uintptr_t& abortMethod) {
    uintptr_t table, getter;
    if (!Read(task, table) || !Read(table + 12, getter) || !Readable(getter, 6, true)) return false;
    const unsigned char expected[] = {0xB8,0x3E,0x01,0x00,0x00,0xC3};
    if (memcmp(reinterpret_cast<void*>(getter), expected, sizeof(expected))) return false;
    const unsigned char abortPrefix[] = {
        0x56,0x8B,0xF1,0x80,0x7E,0x20,0x00,0x74,0x2A,0x8B,0x4E,0x14,0x85,0xC9,0x74,0x1D
    };
    return Read(table + 20, abortMethod) && Readable(abortMethod, sizeof(abortPrefix), true) &&
        !memcmp(reinterpret_cast<void*>(abortMethod), abortPrefix, sizeof(abortPrefix));
}

void ProcessRequest() {
    if (GetTickCount() - requestedAt > 250) {
        strcpy_s(status, "not run - request expired");
        return;
    }
    uintptr_t intelligence, owner, task, abortMethod;
    unsigned char playerFlag = 0;
    SIZE_T count = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(pendingPed + 0x219),
            &playerFlag, 1, &count) || count != 1 || !playerFlag ||
        !Read(pendingPed + 0x224, intelligence) || !Read(intelligence + 0x40, owner) ||
        owner != pendingPed || !Read(intelligence + 0x68, task) ||
        task != pendingTask || !IsShove(task, abortMethod)) {
        strcpy_s(status, "not run - player/shove task changed");
        return;
    }
    using Abort = bool(__thiscall*)(void*, void*, int, void*);
    bool accepted = reinterpret_cast<Abort>(abortMethod)(
        reinterpret_cast<void*>(task), reinterpret_cast<void*>(pendingPed), 2, nullptr);
    if (!accepted) {
        strcpy_s(status, "rejected - shove abort returned false");
        return;
    }
    uintptr_t current;
    if (!Read(intelligence + 0x68, current) || current != task) {
        strcpy_s(status, "accepted - slot changed; removal skipped");
        return;
    }
    // Match engine accepted-abort bookkeeping before engine-owned destruction.
    auto flags = reinterpret_cast<uint32_t*>(task + 0x0C);
    MEMORY_BASIC_INFORMATION memory = {};
    if (!Readable(reinterpret_cast<uintptr_t>(flags), sizeof(*flags)) ||
        !VirtualQuery(flags, &memory, sizeof(memory)) ||
        ((memory.Protect & 0xFF) != PAGE_READWRITE &&
         (memory.Protect & 0xFF) != PAGE_EXECUTE_READWRITE)) {
        strcpy_s(status, "accepted - flags not writable; removal skipped");
        return;
    }
    *flags |= 2;
    secondarySetter(reinterpret_cast<void*>(intelligence + 0x44), nullptr, 4);
    if (!Read(intelligence + 0x68, current)) {
        strcpy_s(status, "accepted - post-removal read failed");
        return;
    }
    sprintf_s(status, "accepted - task %08X removed; slot %08X",
        static_cast<unsigned>(task), static_cast<unsigned>(current));
}

#ifndef HELPER_TEST
class HelperThread : public NativeThread {
    void RunTick() override {
        AcquireSRWLockExclusive(&lock);
        if (!initialized) {
            secondarySetter = FindSetter();
            initialized = true;
            strcpy_s(status, secondarySetter ? "ready - automatic cancellation" :
                "disabled - secondary setter signature absent/ambiguous");
        }
        if (pending) {
            pending = false;
            if (secondarySetter) ProcessRequest();
        }
        ReleaseSRWLockExclusive(&lock);
    }
public:
    HelperThread() { char name[] = "NoAutoPushHelper"; SetName(name); }
};
#endif
}

extern "C" int __cdecl RequestShoveAbort(uint32_t ped, uint32_t task) {
    AcquireSRWLockExclusive(&lock);
    int queued = initialized && secondarySetter && !pending && ped && task;
    if (queued) {
        pendingPed = ped;
        pendingTask = task;
        requestedAt = GetTickCount();
        pending = true;
        strcpy_s(status, "queued for game-thread cancellation");
    }
    ReleaseSRWLockExclusive(&lock);
    return queued;
}

extern "C" int __cdecl GetShoveAbortStatus(char* buffer, int capacity) {
    if (!buffer || capacity < 256) return 0;
    AcquireSRWLockShared(&lock);
    strcpy_s(buffer, static_cast<size_t>(capacity), status);
    ReleaseSRWLockShared(&lock);
    return 1;
}

#ifndef HELPER_TEST
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        ScriptHookManager::RegisterThread(new HelperThread(), module);
    }
    return TRUE;
}
#endif
