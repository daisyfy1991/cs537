/*
 * =====================================================================================
 *
 *       Filename:  fastsort.c
 *
 *    Description:  project 1 part a 
 *
 *        Version:  1.0
 *        Created:  01/23/2016 14:53:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YING FANG (), fang42@wisc.edu
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>

int compareLinesByWord( const void * ptrObj1, const void * ptrObj2 );

char** readfile(char * input_file);

void printLines ( char**lines );

extern int gWordLoc;
extern int gNumLines;

int gWordLoc = 1;
int gNumLines = 0;

int main(int argc, char * argv[]) {

    char ** lines;
    char * input_file;

    if (argc == 2) {
        input_file = argv[1];
    } else if (argc == 3) {
        input_file = argv[2];
        //convert str to integer 
        //long strtol(const char *restrict str, char **restrict endptr, int base);
        gWordLoc = strtol( argv[1]+1, NULL, 10 );
        // invalid loc input
        if (gWordLoc == 0 ) {
            fprintf (stderr, "Error: Bad command line parameters\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Error: Bad command line parameters\n");
        exit(1);
    }
    lines = readfile(input_file);
    // printf( "before sorting: \n" );
    // printLines ( lines);
    qsort(lines, gNumLines, sizeof(char*), compareLinesByWord);
    // printf( "\nafter sorting: \n" );
    printLines ( lines);

    return 0;


}


char** readfile(char * input_file) {
    // open file
    FILE* fp = fopen(input_file, "r");
    //invalid files
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        exit(1);
    }
    // read line number
    struct stat sb;

    if (stat(input_file, &sb) == -1 ) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        exit(1);
    } else {
        gNumLines = sb.st_size;
    }
    // create an array of char pointers and allocate memory
    char ** lines = malloc( gNumLines * sizeof( char * ) );  
    if( lines == NULL ) {
        fprintf( stderr, "malloc failed\n" );
        exit(1);
    }
    // read file line by line
    int i = 0;
    while (1) {
        lines[i] = malloc( 128 * sizeof(char) );
        if (fgets(lines[i], 128, fp) == NULL) break;
        if (strlen(lines[i]) != 0) {
            if (lines[i][strlen(lines[i])-1] != '\n') {
                fprintf(stderr, "Line too long\n");
                exit(1);
            }
        }
        i++;
    }
    fclose(fp);
    return lines;
}

void printLines ( char** lines ) {
    int  i;
    for( i = 0; i < gNumLines; i++ ) {
        if( lines[i] != NULL )
            printf( "%s", lines[i] );
    }
}

int compareLinesByWord( const void * ptrObj1, const void * ptrObj2 ) {
    
    char * word1, * word2;
    char lcp1[128];
    char lcp2[128];
    if( (*(const char**)ptrObj1) == NULL ) {
        word1 = "";
    } else {
        strcpy( lcp1, *((const char**)ptrObj1) );
        int loc = 1;
        word1 = strtok( lcp1, " " );
        while( loc < gWordLoc && word1 != NULL ) {
            char * p = strtok( NULL, " " );
            if( p != NULL ) {
                word1 = p;
                loc++;
            } else {
                break;
            }
        }

        if( word1 == NULL ) {
            word1 = "";
        }
    }

    if( *(const char**)ptrObj2 == NULL ) {
        word2 = "";
    } else {
        strcpy( lcp2, *((const char**)ptrObj2) );
        word2 = strtok( lcp2, " " );
        int loc = 1;
        while( loc < gWordLoc && word2 != NULL ) {
            char * p = strtok( NULL, " " );
            if( p != NULL ) {
                word2 = p;
                loc++;
            } else {
                break;
            }
        }
        if( word2 == NULL ) {
            word2 = "";
        }
    }

    return strcmp( word1, word2 );
}
