
enum {
    SYS_BLANK,
    
    // File Operations
    SYS_open,   
    SYS_read,   
    SYS_write,  
    SYS_close,  
    SYS_lseek,  
    SYS_stat,   
    SYS_fstat,  
    SYS_unlink, 
    SYS_mkdir,  
    SYS_rmdir,  
    SYS_link,   
    SYS_symlink,
    SYS_access, 

    // Process Management
    SYS_fork,    
    SYS_execve,  
    SYS_exit,    
    SYS_wait4,   
    SYS_getpid,  
    SYS_getppid, 
    SYS_kill,    

    // IPC
    SYS_pipe,    
    SYS_shmget,  
    SYS_shmat,   
    SYS_msgget,  
    SYS_msgsnd,  

    // Memory Management
    SYS_brk,     
    SYS_mmap,    
    SYS_munmap,  
    SYS_mprotect,

    // Time
    SYS_time,    
    SYS_gettimeofday,
    SYS_nanosleep,

    // Signals
    SYS_sigaction,
    SYS_sigprocmask,

    NR_SYSCALL
};

void syscall();