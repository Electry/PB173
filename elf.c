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

int get_elf_info(byte_t *bin, uintptr_t *entry, uintptr_t *text_offset, uintptr_t *vaddr, int *size, section_t sections[], int *n_sections) {
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
  Elf64_Shdr *shdr_text = NULL;
  *n_sections = ehdr->e_shnum;
  for (int i = 0; i < ehdr->e_shnum; i++) {
    Elf64_Shdr *shdr = (Elf64_Shdr *)(bin + ehdr->e_shoff + (i * ehdr->e_shentsize));
    sections[i].addr = shdr->sh_addr;
    sections[i].size = shdr->sh_size;
    sections[i].name = (char *)(bin + shdr_section_name_table->sh_offset + shdr->sh_name);

    if (!strncmp(sections[i].name, ".text", 5)) {
      // Found .text
      shdr_text = shdr;
    }
  }

  if (shdr_text == NULL) {
    printf("Could not find .text section!\n");
    return 1;
  }

  *vaddr = shdr_text->sh_addr;
  *size = shdr_text->sh_size;
  *text_offset = shdr_text->sh_offset;
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
              if (dest >= sections[j].addr && dest < sections[j].addr + sections[j].size) {
                snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN,
                    "%s + 0x%llx", sections[j].name, dest - sections[j].addr);
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
