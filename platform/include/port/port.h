// Internal interface of the port platform layer (not seen by decomp code).
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
void port_log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void port_fatal(const char* fmt, ...) __attribute__((format(printf, 1, 2), noreturn));
// Also write the log to this file, keeping the previous one as <path>.prev.
void port_log_file(const char* path);
#define PORT_LOG(...) port_log(__VA_ARGS__)

// ---------------------------------------------------------------------------
// Memory map.  The game sees a Wii-like 32-bit address space:
//   MEM1 cached 0x80000000 (uncached mirror 0xC0000000)
//   MEM2 cached 0x90000000 (uncached mirror 0xD0000000)
//   game library image loaded at 0x98000000 by the launcher
//   low pool (thread stacks etc) at 0xA0000000
// ---------------------------------------------------------------------------
#define PORT_MEM1_BASE 0x80000000u
#define PORT_MEM1_SIZE 0x08000000u /* 128 MiB (Wii: 24 MiB) */
#define PORT_MEM2_BASE 0x90000000u
#define PORT_MEM2_SIZE 0x08000000u /* 128 MiB (Wii: 64 MiB) */
#define PORT_MEM1_UNCACHED 0xC0000000u
#define PORT_MEM2_UNCACHED 0xD0000000u
#define PORT_LIB_BASE 0x98000000u
#define PORT_LIB_SIZE 0x08000000u
#define PORT_LOWPOOL_BASE 0xA0000000u
#define PORT_LOWPOOL_SIZE 0x10000000u
#define PORT_LOCKED_CACHE 0xE0000000u

int port_mem_init(void);
// Page-granular allocations below 4 GiB (never freed individually).
void* port_low_alloc(size_t size);
int port_is_game_memory(const void* p);

// Translation between game "physical" addresses (as programmed into GPU/DSP
// registers) and host pointers.
void* port_phys_to_host(uint32_t phys);
uint32_t port_host_to_phys(const void* p);

// ---------------------------------------------------------------------------
// Virtual CPU: interrupts
// Hardware-like host threads raise interrupt bits; they are delivered on the
// game "CPU" at safe points (OS calls, GX flushes) or when the CPU is idle.
// ---------------------------------------------------------------------------
enum PortIrq {
    PORT_IRQ_ALARM = 0,
    PORT_IRQ_VI,
    PORT_IRQ_DVD,
    PORT_IRQ_AI,
    PORT_IRQ_DSP,
    PORT_IRQ_PE_TOKEN,
    PORT_IRQ_PE_FINISH,
    PORT_IRQ_NAND,
    PORT_IRQ_WPAD,
    PORT_IRQ_COUNT
};
typedef void (*PortIrqHandler)(void);
void port_irq_set_handler(int irq, PortIrqHandler handler);
void port_irq_raise(int irq);  // thread-safe, from any host thread
void port_irq_poll(void);      // safe point; only from the thread owning the CPU

// ---------------------------------------------------------------------------
// Threads
// ---------------------------------------------------------------------------
// Boots the OS layer and runs `entry` as the Wii default thread (priority 16).
void port_os_boot(void (*entry)(void));
int port_is_game_thread(void);
int64_t port_host_time_ns(void);  // monotonic
void port_host_sleep_ns(int64_t ns);

// Freezes the game: the video retrace and audio DMA clocks stop, so the game
// thread waits in VIWaitForRetrace and no new audio is mixed.  Used while the
// headset session does not have input focus (system menu, headset removed).
void port_set_paused(int paused);
int port_is_paused(void);

// A video retrace timed by the headset's display (vi.cpp).  While these keep
// coming, the free-running 59.94 Hz clock stays quiet, so every game frame
// stays on screen for the same whole number of display refreshes.
void port_vi_retrace(void);

// Cutscene skipping (platform/src/port/cutscene_skip.cpp; the game's side is
// in port/compat.h).
void port_skip_set_hold_seconds(float seconds);  // 0 turns skipping off
int port_skip_inject_a(void);                    // A as the game should read it, -1: as pressed
void port_skip_indicator(float* progress, int* forwarding, unsigned* skips);
int port_input_real_a(void);  // controller 0's A button as the player presses it (wpad.cpp)

// Frame-time statistics (platform/src/port/perf.cpp), logged every 10 s.
void port_perf_frame_begin(void);      // the game thread starts a frame
void port_perf_frame_work_done(void);  // ... and waits for the retrace
void port_perf_record(int64_t ns);     // GX command recording, game thread
void port_perf_prepare(int64_t ns);    // a frame prepared for the renderer
void port_perf_render(int64_t cpuNs, int64_t uploadNs);  // a displayed frame
void port_perf_gpu_wait(int64_t ns);   // GPU time after submission (simulation)

#ifdef __cplusplus
}
#endif
