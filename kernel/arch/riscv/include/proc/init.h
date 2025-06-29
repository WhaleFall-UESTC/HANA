#ifndef __RV_INIT_H__
#define __RV_INIT_H__

/**
 * This file contains init code for init process.
 * Code below will be copied to 0x00 and pc will be set there.
 * It will excute the userspace init program by exceve syscall.
 * 
 * Refer to user/init/rv.s,  user/init/initcode.c
 */

extern const char _binary_build_riscv_user_initcode_start[];
extern const char _binary_build_riscv_user_initcode_end[];
#define INIT_CODE _binary_build_riscv_user_initcode_start
#define INIT_CODE_SIZE ((size_t)(_binary_build_riscv_user_initcode_end - _binary_build_riscv_user_initcode_start))

char deadloop[] = {0x67, 0x00, 0x00, 0x00};

#endif // __RV_INIT_H__