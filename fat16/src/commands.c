#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "commands.h"
#include "fat16.h"
#include "support.h"

off_t fsize(const char *filename){
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

struct fat_dir find(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb){
    struct fat_dir curdir;
    int dirs_len = sizeof(struct fat_dir) * bpb->possible_rentries;
    int i;

    for (i=0; i < dirs_len; i++){
        if (strcmp((char *) dirs[i].name, filename) == 0){
            curdir = dirs[i];
            break;
        }
    }
    return curdir;
}

struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb){
    int i;
    struct fat_dir *dirs = malloc(sizeof (struct fat_dir) * bpb->possible_rentries);

    for (i=0; i < bpb->possible_rentries; i++){
        uint32_t offset = bpb_froot_addr(bpb) + i * 32;
        read_bytes(fp, offset, &dirs[i], sizeof(dirs[i]));
    }
    return dirs;
}

int write_dir(FILE *fp, char *fname, struct fat_dir *dir){
    char* name = padding(fname);
    strcpy((char *) dir->name, (char *) name);
    if (fwrite(dir, 1, sizeof(struct fat_dir), fp) <= 0)
        return -1;
    return 0;
}

int write_data(FILE *fp, char *fname, struct fat_dir *dir, struct fat_bpb *bpb){

    FILE *localf = fopen(fname, "r");
    int c;

    while ((c = fgetc(localf)) != EOF){
        if (fputc(c, fp) != c)
            return -1;
    }
    return 0;
}

int wipe(FILE *fp, struct fat_dir *dir, struct fat_bpb *bpb){
    int start_offset = bpb_froot_addr(bpb) + (bpb->bytes_p_sect * \
            dir->starting_cluster);
    int limit_offset = start_offset + dir->file_size;

    while (start_offset <= limit_offset){
        fseek(fp, ++start_offset, SEEK_SET);
        if(fputc(0x0, fp) != 0x0)
            return 01;
    }
    return 0;
}

void mv(FILE *fp, char *src, char *dest, struct fat_bpb *bpb){
    ;;
}



void rm(FILE *fp, char *src, struct fat_bpb *bpb) {
    // Encontrar o arquivo no diretório raiz
    struct fat_dir *dirs = ls(fp, bpb);
    struct fat_dir file = find(dirs, src, bpb);

    // Marca a entrada do diretório como livre
    file.name[0] = DIR_FREE_ENTRY;

    // Calcula o offset da entrada do diretório
    int dir_offset = bpb_froot_addr(bpb) + (file.starting_cluster - 2) * 32;

    // Procura a entrada do diretório e escreve a entrada atualizada
    fseek(fp, dir_offset, SEEK_SET);
    fwrite(&file, sizeof(struct fat_dir), 1, fp);

    // Libera os clusters no FAT
    uint16_t cluster = file.starting_cluster;
    while (cluster < 0xFFF8) {
        // Calcula o offset no FAT
        uint32_t fat_offset = bpb_faddress(bpb) + cluster * 2;

        // Lê o próximo cluster na sequência
        uint16_t next_cluster;
        fseek(fp, fat_offset, SEEK_SET);
        fread(&next_cluster, sizeof(uint16_t), 1, fp);

        // Marca o cluster atual como livre
        fseek(fp, fat_offset, SEEK_SET);
        uint16_t free_cluster = 0x0000;
        fwrite(&free_cluster, sizeof(uint16_t), 1, fp);

        // Troca para o próximo cluster
        cluster = next_cluster;
    }

    free(dirs);
}


void cp(FILE *fp, char *src, const char *dest, struct fat_bpb *bpb){
    // Encontra o arquivo no diretório raiz
    struct fat_dir *dirs = ls(fp, bpb);
    struct fat_dir file = find(dirs, src, bpb);

    // Abre o arquivo de destino para escrita
    FILE *localf = fopen(dest, "wb");

    // Calcula o endereço inicial do cluster de dados
    uint32_t data_addr = bpb_fdata_addr(bpb) + (file.starting_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;
    fseek(fp, data_addr, SEEK_SET);

    // Lê e copiar o conteúdo do arquivo
    uint8_t buffer[512];
    uint32_t bytes_left = file.file_size;
    while (bytes_left > 0) {
        size_t bytes_to_read = (bytes_left > sizeof(buffer)) ? sizeof(buffer) : bytes_left;
        fread(buffer, 1, bytes_to_read, fp);
        fwrite(buffer, 1, bytes_to_read, localf);
        bytes_left -= bytes_to_read;
    }

    fclose(localf);
    free(dirs);
}

