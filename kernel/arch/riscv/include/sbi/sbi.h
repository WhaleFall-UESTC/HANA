#ifndef __SBI_H__
#define __SBI_H__

struct sbiret {
    long error;
    long value;
};
typedef struct sbiret sbiret_t;

sbiret_t sbi_ecall(int ext_id, int func_id, uint64 arg0, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5);


#define SBI_EXT_BASE 0x10
#define SBI_EXT_LEGACY_CONSOLE 0x1
#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_CONSOLE 0x434F4E53

#define SBI_FUNC_CONSOLE_PUTCHAR 0
#define SBI_FUNC_CONSOLE_GETCHAR 1
#define SBI_FUNC_SET_TIMER 0

#define SBI_SUCCESS 0
#define SBI_ERR_FAILED -1
#define SBI_ERR_NOT_SUPPORTED -2
#define SBI_ERR_INVALID_PARAM -3
#define SBI_ERR_DENIED -4
#define SBI_ERR_INVALID_ADDRESS -5
#define SBI_ERR_ALREADY_AVAILABLE -6
#define SBI_ERR_ALREADY_STARTED -7
#define SBI_ERR_ALREADY_STOPPED -8
#define SBI_ERR_NO_SHMEM -9



#define sbi_console_putchar(c) \
    sbi_ecall(SBI_EXT_LEGACY_CONSOLE, SBI_FUNC_CONSOLE_PUTCHAR, c, 0, 0, 0, 0, 0)

#define sbi_console_getchar() \
    sbi_ecall(SBI_EXT_LEGACY_CONSOLE, SBI_FUNC_CONSOLE_GETCHAR, 0, 0, 0, 0, 0, 0)

#define sbi_set_timer(t) \
    sbi_ecall(SBI_EXT_TIME, SBI_FUNC_SET_TIMER, t, 0, 0, 0, 0, 0)

#define sbi_get_time() \
    sbi_ecall(SBI_EXT_TIME, SBI_FUNC_GET_TIME, 0, 0, 0, 0, 0, 0)

#endif // __SBI_H__