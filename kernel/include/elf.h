#ifndef __ELF_H__
#define __ELF_H__

#include <common.h>

#define ELF_MAGIC 0x464C457FU  

#define EI_MAG0		0		/* File identification byte 0 index */
#define ELFMAG0		0x7f		/* Magic number byte 0 */
   
#define EI_MAG1		1		/* File identification byte 1 index */
#define ELFMAG1		'E'		/* Magic number byte 1 */
   
#define EI_MAG2		2		/* File identification byte 2 index */
#define ELFMAG2		'L'		/* Magic number byte 2 */
   
#define EI_MAG3		3		/* File identification byte 3 index */
#define ELFMAG3		'F'		/* Magic number byte 3 */
   
/* Conglomeration of the identification bytes, for easy testing as a word.  */
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define EI_NIDENT (16)

typedef uint16 Elf32_Half;
typedef uint16 Elf64_Half;
   
typedef uint32 Elf32_Word;
typedef	int32  Elf32_Sword;
typedef uint32 Elf64_Word;
typedef	int32  Elf64_Sword;

typedef uint64 Elf32_Xword;
typedef	int64  Elf32_Sxword;
typedef uint64 Elf64_Xword;
typedef	int64  Elf64_Sxword;
   
typedef uint32 Elf32_Addr;
typedef uint64 Elf64_Addr;
   
typedef uint32 Elf32_Off;
typedef uint64 Elf64_Off;
   
typedef uint16 Elf32_Section;
typedef uint16 Elf64_Section;
   
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;


// File header  
#pragma pack(push, 1) 

typedef struct {
    unsigned char	e_ident[EI_NIDENT];
    Elf64_Half	e_type;			  /* Object file type */
    Elf64_Half	e_machine;		/* Architecture */
    Elf64_Word	e_version;	  /* Object file version */
    Elf64_Addr	e_entry;		  /* Entry point virtual address */
    Elf64_Off	  e_phoff;		  /* Program header table file offset */
    Elf64_Off	  e_shoff;		  /* Section header table file offset */
    Elf64_Word	e_flags;		  /* Processor-specific flags */
    Elf64_Half	e_ehsize;		  /* ELF header size in bytes */
    Elf64_Half	e_phentsize;	/* Program header table entry size */
    Elf64_Half	e_phnum;		  /* Program header table entry count */
    Elf64_Half	e_shentsize;	/* Section header table entry size */
    Elf64_Half	e_shnum;		  /* Section header table entry count */
    Elf64_Half	e_shstrndx;		/* Section header string table index */
} Elf64_Ehdr;

// Program section header

typedef struct {
    Elf64_Word	p_type;			/* Segment type */
    Elf64_Word	p_flags;		/* Segment flags */
    Elf64_Off	p_offset;		/* Segment file offset */
    Elf64_Addr	p_vaddr;		/* Segment virtual address */
    Elf64_Addr	p_paddr;		/* Segment physical address */
    Elf64_Xword	p_filesz;		/* Segment size in file */
    Elf64_Xword	p_memsz;		/* Segment size in memory */
    Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;

#pragma pack(pop) 

typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;

// Values for Phdr type
#define	PT_NULL		  0		/* Program header table entry unused */
#define PT_LOAD		  1		/* Loadable program segment */
#define PT_DYNAMIC	  2	    /* Dynamic linking information */
#define PT_INTERP	  3		/* Program interpreter */
#define PT_NOTE		  4		/* Auxiliary information */
#define PT_SHLIB	  5		/* Reserved */
#define PT_PHDR		  6		/* Entry for header table itself */
#define PT_TLS		  7		/* Thread-local storage segment */
#define	PT_NUM		  8		/* Number of defined types */

// Flag bits for Phdr flags
#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */

#endif
