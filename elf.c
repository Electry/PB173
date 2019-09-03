#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>

#include "elf.h"

int get_elf_sections(byte_t *bin, section_t sections[], int *n_sections) {
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)bin;

  // Get section header of "section name" string table
  Elf64_Shdr *shdr_section_name_table = NULL;
  if (ehdr->e_shstrndx != SHN_XINDEX) {
    shdr_section_name_table = (Elf64_Shdr *)(bin + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize));
  }
  else {
    int ndx = ((Elf64_Shdr *)(bin + ehdr->e_shoff))->sh_link;
    shdr_section_name_table = (Elf64_Shdr *)(bin + ehdr->e_shoff + (ndx * ehdr->e_shentsize));
  }

  // Find .text section among all sections
  *n_sections = ehdr->e_shnum;
  for (int i = 0; i < ehdr->e_shnum; i++) {
    Elf64_Shdr *shdr = (Elf64_Shdr *)(bin + ehdr->e_shoff + (i * ehdr->e_shentsize));
    sections[i].vaddr = shdr->sh_addr;
    sections[i].elf_offset = shdr->sh_offset;
    sections[i].size = shdr->sh_size;
    sections[i].name = (char *)(bin + shdr_section_name_table->sh_offset + shdr->sh_name);
  }

  return 0;
}

int get_elf_info(byte_t *bin, section_t sections[], int n_sections, uintptr_t *entry, uintptr_t *vaddr, uintptr_t *text_offset, int *text_size) {
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)bin;
  *entry = ehdr->e_entry;

  // Get vaddr
  /*
  *vaddr = 0;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)(bin + ehdr->e_phoff + (i * ehdr->e_phentsize));
    if (phdr->p_type == PT_LOAD && phdr->p_offset == 0) {
      *vaddr = phdr->p_vaddr;
      break;
    }
  }
  */

  for (int i = 0; i < n_sections; i++) {
    if (!strncmp(sections[i].name, ".text", 5)) {
      *vaddr = sections[i].vaddr;
      *text_offset = sections[i].elf_offset;
      *text_size = sections[i].size;
      return 0;
    }
  }

  printf("Could not find .text section!\n");
  return 1;
}

int get_elf_symtab(byte_t *bin, section_t sections[], int n_sections, symbol_t symbols[], int *n_symbols) {
  section_t *s_symtab = NULL, *s_strtab = NULL;
  for (int i = 0; i < n_sections; i++) {
    if (!strncmp(sections[i].name, ".symtab", 5)) {
      s_symtab = &sections[i];
    }
    else if (!strncmp(sections[i].name, ".strtab", 5)) {
      s_strtab = &sections[i];
    }
  }

  if (s_symtab == NULL) {
    printf("Could not find .symtab section!\n");
    *n_symbols = 0;
    return 0;
  }
  if (s_strtab == NULL) {
    printf("Could not find .strtab section!\n");
    *n_symbols = 0;
    return 0;
  }

  *n_symbols = 0;
  for (int i = 0; i * sizeof(Elf64_Sym) < s_symtab->size; i++) {
    Elf64_Sym *sym = (Elf64_Sym *)(bin + s_symtab->elf_offset + (i * sizeof(Elf64_Sym)));
    char *name = (char *)(bin + s_strtab->elf_offset + sym->st_name);
    if (sym->st_name != 0 && name[0] != '\0') {
      symbols[*n_symbols].value = sym->st_value;
      symbols[*n_symbols].size = sym->st_size;
      symbols[*n_symbols].type = ELF64_ST_TYPE(sym->st_info);
      symbols[*n_symbols].binding = ELF64_ST_BIND(sym->st_info);
      symbols[*n_symbols].name = name;
      symbols[*n_symbols].shndx = sym->st_shndx;

      (*n_symbols)++;
    }
  }

  return 0;
}

int load_file(const char *path, int *fd, byte_t **bin, int *fsize) {
  // Open file for reading
  *fd = open(path, O_RDONLY);
  if (*fd < 0) {
    printf("Could not open file: %s\n", path);
    return 1;
  }

  // Get file size
  struct stat stat_buf;
  fstat(*fd, &stat_buf);
  *fsize = stat_buf.st_size;

  // Map file to memory
  *bin = (byte_t *)mmap(NULL, *fsize, PROT_READ, MAP_PRIVATE, *fd, 0);
  if (*bin < 0) {
    printf("Could not mmap file: %s\n", path);
    close(*fd);
    return 1;
  }

  return 0;
}

int close_file(byte_t *bin, int fd, int fsize) {
  munmap(bin, fsize);
  close(fd);
  return 0;
}

void proc_section_labels(instr_t instr[], int count, section_t sections[], int n_sections) {
  for (int i = 0; i < count; i++) {
    // Should print note?
    if (!instr[i].has_ext_opcode) {
      switch (instr[i].opcode) {
        case OP_MOV_89:
        case OP_MOV_8B:
          if (instr[i].modrm.mod != 0b11
              && strstr(instr[i].mnemo_operand, "(\%rip)") != NULL) { // strstr LULZ
            long long dest = instr[i].addr + instr[i].len + instr[i].value;
            bool done = false;

            for (int j = 0; j < n_sections; j++) {
              if (dest >= sections[j].vaddr && dest < sections[j].vaddr + sections[j].size) {
                snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN,
                    "%s + 0x%llx", sections[j].name, dest - sections[j].vaddr);
                done = true;
                break;
              }
            }

            if (!done) {
              snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN, "0x%llx", dest);
            }
          }
          break;
      }
    }
  }
}

void proc_symtab_labels(instr_t instr[], int count, symbol_t symbols[], int n_symbols) {
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < n_symbols; j++) {
      // Entrypoint match?
      if (instr[i].addr == symbols[j].value) {
        snprintf(instr[i].label, LABEL_LEN, "%s", symbols[j].name);
        break;
      }

      // Rename all unnamed bblocks
      if (instr[i].label[0] != '\0'
          && instr[i].sub_addr == symbols[j].value
          && !strncmp(instr[i].label, "sub_", 4)) {
        snprintf(instr[i].label, LABEL_LEN, "%s_%x", symbols[j].name, instr[i].addr);
        break;
      }
    }
  }
}