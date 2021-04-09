#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int create(const char *dir,const char *inputDir);
int extract(const char *directoryName);
int print_tar_tree(const char *tarDir);

int main(int argc, char *argv[]){
    char mode=0;
    int c;
    char *tar;
    while((c = getopt (argc, argv, "cxtf:")) != -1) {
        switch (c) {
            case 'c':
                if (mode != 0) {
                    printf("Error: Multiple modes specified.\n");
                    return -1;
                }
                mode = 'c';
                break;
            case 'x':
                if (mode != 0) {
                    printf("Error: Multiple modes specified.\n");
                    return -1;
                }
                mode = 'x';
                break;
            case 't':
                if (mode != 0) {
                    printf("Error: Multiple modes specified.\n");
                    return -1;
                }
                mode = 't';
                break;
            case 'f':
                if (optarg == NULL) {
                    printf("Error: No directory target specified.\n");
                    return -1;
                }
                tar = optarg;
                if (tar==NULL){
                    printf("Error: No tarfile specified.");
                    return -1;
                }
                break;
            default:
                printf("Please use: myTar [cxtf:] [file]filename\n");
                return -1;
        }
    }

    char *fileName;
    while(optind<argc){
        fileName=argv[optind];
        optind++;
    }
    if (mode==0){
        printf("Error: No mode specified.\n");
        return -1;
    }
    if(strlen(tar)==0){
        printf("Error: No tarfile specified.\n");
        return -1;
    }
    switch (mode) {
        case'c':return create(tar,fileName);
        case'x':return extract(tar);
        case't':return print_tar_tree(tar);
    }

    return 0;
}