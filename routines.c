
// Created by Kevin Tuyishime on 3/20/21.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include "inodemap.h"
#include <string.h>
#include <unistd.h>

//helper Routines
    int file_to_Tar(struct dirent *de ,const char *tdir,FILE *archive){
//    printf("starting file to tar\n");
        struct stat file;
        char *dirName=malloc(sizeof (tdir)+sizeof (de->d_name));
//    printf("%s/%s\n",tdir,de->d_name);
        sprintf(dirName,"%s/%s",tdir,de->d_name);

        int exists= stat(dirName,&file);
        if (exists<0){
            printf("Specified target (\"%s\") does not exist.\n",dirName);
            free(dirName);
            return -1;
        }
        u_int32_t fn_len;
        fn_len = strlen(dirName);
        if (get_inode(de->d_ino)){
            fwrite(&de->d_ino,8,1,archive);
            fwrite(&fn_len,4,1,archive);
            fwrite(dirName,fn_len,1,archive);
            fclose(archive);
        }else{
            set_inode(de->d_ino,dirName);
            fwrite(&de->d_ino,8,1,archive);
            fwrite(&fn_len,4,1,archive);
            fwrite(dirName,fn_len,1,archive);
            fwrite(&file.st_mode,4,1,archive);
            fwrite(&file.st_mtim,8,1,archive);
            fwrite(&file.st_size,8,1,archive);

            FILE *s_file = fopen(dirName,"r");
            char *content;
            content	= malloc(file.st_size);
            int  succ =fread(content, file.st_size, 1, s_file);
            if (succ<0){
                perror("fread");
                free(content);
                free(dirName);
                return -1;
            }
            fwrite(content,file.st_size, 1,archive);
            fclose(s_file);
            free(content);
        }
        free(dirName);
        // free(fn_len);
        return 0;
    }

    int dir_to_tar(struct dirent *de,const char *tdir,FILE *archive){
//    printf("starting dir to tar\n");
        struct stat dir;
        char *dirName=malloc(sizeof (tdir)+sizeof (de->d_name));
//    printf("%s/%s\n",tdir,de->d_name);
        sprintf(dirName,"%s/%s",tdir,de->d_name);
        int exists = stat(dirName,&dir);
        if (exists<0){
            printf("Error: Specified target (\"%s\") does not exist.\n", tdir);
            free(dirName);
            return -1;
        }else if(!S_ISDIR(dir.st_mode)){
            printf("Error: Specified target (\"%s\") is not a directory.\n", tdir);
            free(dirName);
            return -1;
        }

        fwrite(&dir.st_ino,8,1,archive);
        u_int32_t fn_len;
        fn_len = strlen(dirName);
//    printf("directory name %s and the size: %d\n",dirName,*fn_len);
        fwrite(&fn_len,4,1,archive);
        fwrite(dirName,fn_len,1,archive);
        fwrite(&dir.st_mode,4,1,archive);
        fwrite(&dir.st_mtim,8,1,archive);
//    printf("directory name %s and the size: %d, with mode: %o\n",dirName,*fn_len,dir.st_mode);
        struct dirent *de_r;DIR *d_r;
        d_r= opendir(dirName);
//    printf("into"
//           " the loop\n");
        for(de_r=readdir(d_r); de_r != NULL; de_r=readdir(d_r)){
            if (de_r->d_type == DT_DIR){
                if (strcmp(de_r->d_name, ".") != 0 &&
                    strcmp(de_r->d_name, "..") != 0 ){
                    dir_to_tar(de_r, dirName, archive);
                }}
            else if (de_r->d_type == DT_REG) file_to_Tar(de_r, dirName, archive);

        }
        closedir(d_r);
//    free(fn_len);
        free(dirName);
        return 0;
    }


//main routines
int create(const char *tdir,const char *inputDir) {
        struct stat dir;
        int exists = stat(inputDir, &dir);
        // verify directory provided
        if (exists < 0) {
            printf("Error: Specified target (\"%s\") does not exist.\n", tdir);
            return -1;
        } else if (!S_ISDIR(dir.st_mode)) {
            printf("Error: Specified target (\"%s\") is not a directory.\n", tdir);
            return -1;
        }
        FILE *archive; //tar file creation
        archive = fopen(tdir, "w");
        u_int32_t m_nber; //magic number
        m_nber = 1918989421;
        fwrite(&m_nber, 4, 1, archive);
        fwrite(&dir.st_ino,8,1, archive);
        u_int32_t fn_len;
        fn_len = strlen(inputDir);
        fwrite(&fn_len, 4, 1, archive);
        fwrite(inputDir, fn_len, 1, archive);
        fwrite(&dir.st_mode, 4, 1, archive);
        fwrite(&dir.st_mtim, 8, 1, archive);
        DIR *d;
        d = opendir(inputDir);
        struct dirent *de;
        for (de = readdir(d); de != NULL; de = readdir(d)) {
            if (de->d_type == DT_DIR && strcmp(de->d_name, ".")
                                        != 0 && strcmp(de->d_name, "..") != 0) {
                dir_to_tar(de, inputDir, archive);
            } else if (de->d_type == DT_REG) file_to_Tar(de, inputDir, archive);
        }
        fclose(archive);
        closedir(d);
        //free(fn_len);
//    free(m_nber);
        return 0;
    }

int extract(const char *dir){
        FILE *tar = fopen(dir, "r");
        u_int32_t m_nber;
        fseek(tar, 0, SEEK_SET);
        fread(&m_nber, 4, 1, tar);
//    printf("%d\n", *m_nber);
        if (m_nber != 1918989421) {
            printf("Error: Bad magic number (%d), should be: %d.\n", m_nber, 1918989421);
            return -1;
        }
        // recreate init directory
        ino_t *t_inode;
        t_inode = malloc(8);
        //   int *f_len = malloc(4);
        u_int32_t t_len;
        mode_t *t_mode;
        t_mode = malloc(4);
    
        fread(t_inode, 8, 1, tar);
        fread(&t_len, 4, 1, tar);
        char *t_name=malloc(t_len+1);
        fread(t_name, t_len, 1, tar);
        t_name[t_len] = '\0';
    
        fread(t_mode,4,1,tar);
       
        mkdir(t_name,*t_mode);
        printf("created dir: %s\n",t_name);

        struct timeval t_mtime;
        fread(&t_mtime,8,1,tar);
    
        while (!feof(tar)) {

            ino_t *f_inode;
            f_inode = malloc(8);
            //   int *f_len = malloc(4);
            u_int32_t f_len;
            fread(f_inode, 8, 1, tar);
            fread(&f_len, 4, 1, tar);
            char *f_name=malloc(f_len+1);
            fread(f_name, f_len, 1, tar);
            f_name[f_len] = '\0';

            if (get_inode(*f_inode)) {
                link(get_inode(*f_inode), f_name);
//            printf("created link: %s\n",f_name);
            }else{
                mode_t *f_mode;
                f_mode = malloc(4);
                fread(f_mode, 4, 1, tar);
                set_inode(*f_inode,f_name);
//           printf("%s -- inode: %llu\n", f_name, *f_inode);

                if (S_ISDIR(*f_mode)) {
                    int i = mkdir(f_name,*f_mode);
                    if (i<0)perror("mkdir()");

//                printf("created dir: %s\n",f_name);
                    int j =chmod(f_name,*f_mode);
                    if (j<0)perror("chmod()");
                    struct timeval f_mtime;
//	            f_mtime	= malloc(8);
                    fread(&f_mtime,8,1,tar);
//                free(f_mtime);
                }else
                if (S_ISREG(*f_mode)){
                    struct timeval f_mtime;
//                int *f_size = malloc(8);
                    u_int64_t f_size;
                    fread(&f_mtime, 8, 1, tar);
                    fread(&f_size,8,1,tar);
                    char *content = malloc(f_size+1);
                    fread(content,f_size,1,tar);
                    FILE *create_file = fopen(f_name,"w+");

//                printf("created file: %s\n",f_name);
                    struct timeval new_time;
                    gettimeofday(&new_time,NULL);

                    int j = chmod(f_name,*f_mode);
                    if (j<0) perror("chmod()");
                    int  k = fwrite(content,f_size,1,create_file);
                    if (k<0)perror("fwrite()");
                    //set time
                    utimes(f_name,&new_time);
                    fclose(create_file);
                    free(content);
//                free(f_size);
//                free(new_time);
//                free(f_mtime);

                }

            }
            free(f_inode);
            free(f_name);
//        free(f_len);
        }
//    free(m_nber);
        free(t_inode);
        free(t_name);
        free(t_mode);
        fclose(tar);
        return 0;
    }

int print_tar_tree(const char *dir) {
        FILE *tar = fopen(dir, "r");
        u_int32_t m_nber;
        fseek(tar, 0, SEEK_SET);
        fread(&m_nber, 4, 1, tar);
        if (m_nber != 1918989421) {
            printf("Error: Bad magic number (%d), should be: %d.\n", m_nber, 1918989421);
//        free(m_nber);
            return -1;
        }

        while (!feof(tar)) {
//        printf("in the loop\n");

            ino_t *f_inode;
            f_inode	= malloc(8);
//        int *f_len = malloc(4);
            u_int32_t f_len;
            fread(f_inode, 8, 1, tar);
            fread(&f_len, 4, 1, tar);
            char *f_name;
            f_name = malloc(f_len+1);
            fread(f_name, f_len, 1, tar);
            f_name[f_len] = '\0';
            if (!get_inode(*f_inode)) {
                mode_t *f_mode;
                f_mode = malloc(4);
                fread(f_mode, 4, 1, tar);
//            printf("%s -- inode: %llu\n", f_name, *f_inode);
                if (S_ISREG(*f_mode)) {
//                printf("it's a file\n");
                    struct timeval f_mtime;
//                f_mtime=malloc(8);
//                int *f_size=malloc(4);
                    u_int64_t f_size;
                    fread(&f_mtime, 8, 1, tar);
                    fread(&f_size, 8, 1, tar);
                    if (S_ISREG(*f_mode)) {
                        printf("%s -- inode: %lu, mode: %o, mtime: %ld, size: %lld\n",
                               f_name, *f_inode, *f_mode,f_mtime.tv_sec, f_size);
                    } else if ((*f_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) { //when file is exe.
                        printf("%s* -- inode: %lu, mode: %o, mtime:%ld, size: %lld\n",
                               f_name, *f_inode, *f_mode,f_mtime.tv_sec, f_size);
                    }
                    char *content;
                    content=malloc(f_size+1);
                    // to progress the reading
                    fread(content, f_size, 1, tar);
//                free(f_mtime);
//                free(f_size);
                    free(content);
                } else if (S_ISDIR(*f_mode)) {
//                printf("it's a directory\n");
                    struct timeval f_mtime;
//                f_mtime	=malloc(8);
                    fread(&f_mtime, 8, 1, tar);
                    printf("%s/ -- inode: %lu, mode: %o, mtime: %ld\n",
                           f_name, *f_inode, *f_mode, f_mtime.tv_sec);
//                    free(f_mtime);
                }
                free(f_mode);

            } else {
                printf("%s -- inode: %lu\n",f_name, *f_inode);
            }

//      printf("end if\n");
            free(f_inode);
//        free(f_len);
            free(f_name);
        }
//  printf("closing\n");
//    free(m_nber);
        fclose(tar);
        return 0;
    }
