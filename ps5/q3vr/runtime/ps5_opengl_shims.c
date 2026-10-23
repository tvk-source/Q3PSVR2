/* Minimal process shims required by the statically linked ps5-opengl/Mesa runtime.
 * Quake3e already owns process exit/logging, so unlike ps5-homebrew-ui's full
 * runtime_shims.c this deliberately does not define catchReturnFromMain(). */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint64_t sceKernelGetProcessTime(void);

void ps5q3_glapi_tls_context_init(void) __asm__("_ZTH23_mesa_glapi_tls_Context");
void ps5q3_glapi_tls_context_init(void) {}

int mkstemps(char *template_name, int suffix_length)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    static unsigned counter;
    const size_t length = template_name ? strlen(template_name) : 0;
    if (suffix_length < 0 || length < (size_t)suffix_length + 6u) { errno = EINVAL; return -1; }
    char *name = template_name + length - (size_t)suffix_length - 6u;
    for (int i = 0; i < 6; ++i) if (name[i] != 'X') { errno = EINVAL; return -1; }
    for (int attempt = 0; attempt < 100; ++attempt) {
        uint64_t value = sceKernelGetProcessTime() +
            (uint64_t)__atomic_add_fetch(&counter, 1u, __ATOMIC_RELAXED) * UINT64_C(0x9E3779B97F4A7C15);
        for (int i = 0; i < 6; ++i) { name[i] = letters[value % 36u]; value /= 36u; }
        const int file = open(template_name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (file >= 0 || errno != EEXIST) return file;
    }
    errno = EEXIST;
    return -1;
}
int mkstemp(char *template_name) { return mkstemps(template_name, 0); }
int isatty(int descriptor) { (void)descriptor; errno = ENOTTY; return 0; }
void openlog(const char *identifier, int option, int facility) { (void)identifier; (void)option; (void)facility; }
FILE *popen(const char *command, const char *mode) { (void)command; (void)mode; errno = ENOSYS; return NULL; }
int pclose(FILE *stream) { (void)stream; errno = ENOSYS; return -1; }


/*
 * Required by libc++abi used by the statically linked ps5-opengl runtime.
 * PS5's clean-room libc does not provide this entry point.
 */
__attribute__((noreturn))
void __assert(const char *function, const char *file, int line,
              const char *expression)
{
    fprintf(stderr,
            "[Q3VR] assertion failed: %s (%s:%d, %s)\n",
            expression, file, line, function);
    abort();
}
