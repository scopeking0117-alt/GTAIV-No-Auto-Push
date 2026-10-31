#define HELPER_TEST
#include "NoAutoPushHelper.cpp"
#include <cassert>

namespace {
unsigned char ped[0x230] = {};
unsigned char intelligence[0x70] = {};
unsigned char task[0x28] = {};
uintptr_t table[6] = {};
unsigned char* code;
int removals;

void __fastcall FakeSetter(void* manager, void*, void* replacement, int slot) {
    assert(manager == intelligence + 0x44);
    assert(replacement == nullptr && slot == 4);
    assert(*reinterpret_cast<uint32_t*>(task + 0x0C) & 2);
    *reinterpret_cast<uintptr_t*>(intelligence + 0x68) = 0;
    ++removals;
}

void Reset() {
    memset(ped, 0, sizeof(ped));
    memset(intelligence, 0, sizeof(intelligence));
    memset(task, 0, sizeof(task));
    ped[0x219] = 1;
    *reinterpret_cast<uintptr_t*>(ped + 0x224) = reinterpret_cast<uintptr_t>(intelligence);
    *reinterpret_cast<uintptr_t*>(intelligence + 0x40) = reinterpret_cast<uintptr_t>(ped);
    *reinterpret_cast<uintptr_t*>(intelligence + 0x68) = reinterpret_cast<uintptr_t>(task);
    *reinterpret_cast<uintptr_t*>(task) = reinterpret_cast<uintptr_t>(table);
    *reinterpret_cast<uint32_t*>(task + 0x0C) = 0x80;
    task[0x20] = 1;
    code[1] = 0x3E;
    code[32 + 46] = 1;
    pendingPed = reinterpret_cast<uintptr_t>(ped);
    pendingTask = reinterpret_cast<uintptr_t>(task);
    requestedAt = GetTickCount();
    removals = 0;
    FlushInstructionCache(GetCurrentProcess(), code, 128);
}
}

int main() {
    static_assert(sizeof(void*) == 4, "Tests require x86");
    assert(FindSetter() == nullptr);
    code = static_cast<unsigned char*>(VirtualAlloc(nullptr, 128, MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE));
    assert(code);
    const unsigned char getter[] = {0xB8,0x3E,0x01,0,0,0xC3};
    memcpy(code, getter, sizeof(getter));
    const unsigned char prefix[] = {
        0x56,0x8B,0xF1,0x80,0x7E,0x20,0,0x74,0x2A,0x8B,0x4E,0x14,0x85,0xC9,0x74,0x1D
    };
    memcpy(code + 32, prefix, sizeof(prefix));
    const unsigned char result[] = {0xB0,1,0x5E,0xC2,0x0C,0};
    memcpy(code + 32 + 45, result, sizeof(result));
    table[3] = reinterpret_cast<uintptr_t>(code);
    table[5] = reinterpret_cast<uintptr_t>(code + 32);
    secondarySetter = reinterpret_cast<Setter>(&FakeSetter);
    initialized = true;

    Reset();
    ProcessRequest();
    assert(removals == 1 && strstr(status, "accepted - task"));
    assert(*reinterpret_cast<uint32_t*>(task + 0x0C) == 0x82);
    Reset();
    code[32] = 0x90;
    ProcessRequest();
    assert(removals == 0 && strstr(status, "not run"));
    code[32] = 0x56;
    Reset();
    code[1] = 0x3D;
    ProcessRequest();
    assert(removals == 0 && strstr(status, "not run"));
    Reset();
    requestedAt -= 251;
    ProcessRequest();
    assert(removals == 0 && strstr(status, "expired"));
    Reset();
    *reinterpret_cast<uintptr_t*>(intelligence + 0x68) = 0;
    ProcessRequest();
    assert(removals == 0 && strstr(status, "changed"));
    Reset();
    ped[0x219] = 0;
    ProcessRequest();
    assert(removals == 0);
    Reset();
    code[32 + 46] = 0;
    FlushInstructionCache(GetCurrentProcess(), code, 128);
    ProcessRequest();
    assert(removals == 0 && strstr(status, "rejected"));
    Reset();
    pending = false;
    assert(RequestShoveAbort(static_cast<uint32_t>(pendingPed), static_cast<uint32_t>(pendingTask)) == 1);
    assert(RequestShoveAbort(static_cast<uint32_t>(pendingPed), static_cast<uint32_t>(pendingTask)) == 0);
    char buffer[256];
    assert(GetShoveAbortStatus(buffer, sizeof(buffer)) == 1);
    assert(GetShoveAbortStatus(buffer, 10) == 0);
    assert(VirtualFree(code, 0, MEM_RELEASE));
    puts("PASS: accepted removal, flags preserved, ABI guard, door protection, expiry, changed task, non-player, rejection, queue, status");
}
