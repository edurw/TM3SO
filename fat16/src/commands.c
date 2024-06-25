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

void mv(FILE *fp, char *source_filename, char *dest_filename, struct fat_bpb *bpb) {
    // Listar todos os diretórios
    struct fat_dir *dirs = ls(fp, bpb);
    struct fat_dir file_to_move = find(dirs, source_filename, bpb);

    // Se o arquivo não for encontrado, retornar um erro
    if (file_to_move.name[0] == 0) {
        fprintf(stderr, "File %s not found.\n", source_filename);
        free(dirs);
        return;
    }

    // Copiar o arquivo para o novo nome
    struct fat_dir new_file = file_to_move;
    strncpy(new_file.name, dest_filename, 11);  // Atualizar o nome do arquivo

    // Calcular o offset do diretório raiz
    int dir_offset = bpb_froot_addr(bpb);
    fseek(fp, dir_offset, SEEK_SET);

    // Escrever a nova entrada do arquivo
    fwrite(&new_file, sizeof(struct fat_dir), 1, fp);

    // Remover o arquivo original
    rm(fp, source_filename, bpb);

    // Liberar a memória alocada
    free(dirs);
}

void rm(FILE *fp, char *filename, struct fat_bpb *bpb) {
    // List all directories
    struct fat_dir *dirs = ls(fp, bpb);
    struct fat_dir file_to_remove = find(dirs, filename, bpb);

    // If the file was not found, return an error
    if (file_to_remove.name[0] == 0) {
        fprintf(stderr, "File %s not found.\n", filename);
        free(dirs);
        return;
    }

    // Mark the directory entry as free
    file_to_remove.name[0] = DIR_FREE_ENTRY;

    // Calculate the offset of the directory entry
    int dir_offset = bpb_froot_addr(bpb) + (file_to_remove.starting_cluster - 2) * 32;

    // Seek to the directory entry and write the updated entry
    fseek(fp, dir_offset, SEEK_SET);
    fwrite(&file_to_remove, sizeof(struct fat_dir), 1, fp);

    // Free the clusters in the FAT
    uint16_t cluster = file_to_remove.starting_cluster;
    while (cluster < 0xFFF8) {
        // Calculate the offset in the FAT
        uint32_t fat_offset = bpb_faddress(bpb) + cluster * 2;

        // Read the next cluster in the chain
        uint16_t next_cluster;
        fseek(fp, fat_offset, SEEK_SET);
        fread(&next_cluster, sizeof(uint16_t), 1, fp);

        // Mark the current cluster as free
        fseek(fp, fat_offset, SEEK_SET);
        uint16_t free_cluster = 0x0000;
        fwrite(&free_cluster, sizeof(uint16_t), 1, fp);

        // Move to the next cluster
        cluster = next_cluster;
    }

    // Free allocated memory
    free(dirs);
}


void cp(FILE *fp, char *src, char *dest, struct fat_bpb *bpb){
    // Encontrar o arquivo no diretório raiz
    struct fat_dir *dirs = ls(fp, bpb);
    struct fat_dir file = find(dirs, src, bpb);

    if (file.name[0] == 0) {
        fprintf(stderr, "Arquivo não encontrado: %s\n", src);
        return;
    }

    // Abrir o arquivo de destino para escrita
    FILE *localf = fopen(dest, "wb");
    if (!localf) {
        fprintf(stderr, "Erro ao abrir arquivo de destino: %s\n", dest);
        return;
    }

    // Calcular o endereço inicial do cluster de dados
    uint32_t data_addr = bpb_fdata_addr(bpb) + (file.starting_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;
    fseek(fp, data_addr, SEEK_SET);

    // Ler e copiar o conteúdo do arquivo
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

