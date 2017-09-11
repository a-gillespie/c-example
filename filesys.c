/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"

diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
fatentry_t   currentDirIndex         = 0 ;


/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
    printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
    FILE * dest = fopen( filename, "w" ) ;
    if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
        fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
    //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
    fclose(dest) ;
}

void readdisk ( const char * filename )
{
    FILE * dest = fopen( filename, "r" ) ;
    if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
        fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
    //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
        fclose(dest) ;
    // set current dir to root
    currentDirIndex = rootDirIndex;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */
 
void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}

void readblock ( diskblock_t * block, int block_address)
{
    //printf ( "readblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
    memmove (block->data, virtualDisk[block_address].data, BLOCKSIZE);
    //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
}


/* read and write FAT
 */

void copyFAT()
{
    diskblock_t tmp;
    for (int i = 0; i < MAXBLOCKS/FATENTRYCOUNT; i++) {
        memmove(tmp.data, &FAT[i*FATENTRYCOUNT], BLOCKSIZE);
        writeblock(&tmp, i+1);
    }
}


/* implement format()
 */

void format ( )
{
    printf("Formatting virtual disk...\n");
    
    diskblock_t block ;
    // direntry_t  rootDir ;
    // int         pos             = 0 ;
    // int         fatentry        = 0 ;
    int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;

    /* prepare block 0 : fill it with '\0',
     * use strcpy() to copy some text to it for test purposes
	 * write block 0 to virtual disk
	 */
    for (int i = 0; i < BLOCKSIZE; i++) block.data[i] = '\0';
    // copy text into block using string union
    strcpy(block.str, "CS3026 Operating Systems Assessment");
    writeblock(&block, 0);
    
	/* prepare FAT table
	 * write FAT blocks to virtual disk
	 */
    for (int i = 0; i < MAXBLOCKS; i++) FAT[i] = UNUSED;
    
    // dynamically initialize FAT with it's own block chain
    FAT[0] = ENDOFCHAIN;
    for (int i = 1; i < fatblocksneeded; i++) FAT[i] = i + 1;
    FAT[fatblocksneeded] = ENDOFCHAIN;
    FAT[fatblocksneeded + 1] = ENDOFCHAIN; //root dir FAT entry
    copyFAT();
    
	/* prepare root directory
	 * write root directory block to virtual disk
	 */
    for (int i = 0; i < BLOCKSIZE; i++) block.data[i] = '\0';
    block.dir.parent = 0;
    for (int j = 0; j < DIRENTRYCOUNT; j++) block.dir.entrylist[j].unused = 1;
    writeblock(&block, fatblocksneeded+1);
    rootDirIndex = fatblocksneeded+1;
    
    // set current dir to root
    currentDirIndex = rootDirIndex;
    
    printf("done.\n");
}


/* find and return the blockno/entryindex of a named entry in a given dir
 * returns {.blocki = start of dir, .entryi = -1} if the entry is not found
 */

direntryloc getdirentryloc(const char *entryname, int dirindex)
{
    direntryloc i;
    i.blocki = dirindex;
    while (!NULL) {
        for (i.entryi = 0; i.entryi < DIRENTRYCOUNT; i.entryi++){
            if (strcmp(virtualDisk[i.blocki].dir.entrylist[i.entryi].name, entryname) == 0 && virtualDisk[i.blocki].dir.entrylist[i.entryi].unused == 0) {
                return i;
            }
        }
        if (FAT[i.blocki] == ENDOFCHAIN) {
            i.blocki = dirindex;
            i.entryi = -1; //-1 if entry not found in dir given by dirindex
            return i;
        } else {
            i.blocki = FAT[i.blocki];
        }
    }
}

/* get the index of the first unused block from the FAT, by simple iteration
 * returns 0 if we've run out of available blocks
 */

fatentry_t getfirstunusedblock(void)
{
    for (int i = 0; i < MAXBLOCKS; i++) {
        if (FAT[i] == UNUSED) return i;
    }
    return 0;
}


/* initialise new files and directories
 */

fatentry_t makenewfile(const char*filename, direntryloc loc)
{
    //get first available block
    fatentry_t first = getfirstunusedblock();
    if (first == 0) return 0;
    
    //initialize directory entry for new file
    //virtualDisk[loc.blocki].dir.entrylist[loc.entryi].entrylength = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].isdir = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].unused = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].modtime = time(NULL);
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].filelength = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock = first;
    strncpy(virtualDisk[loc.blocki].dir.entrylist[loc.entryi].name, filename, MAXNAME);
    
    //set FAT entry to end of chain
    FAT[first] = ENDOFCHAIN;
    copyFAT();
    
    return first;
}

fatentry_t makenewdir(const char*dirname, direntryloc loc, fatentry_t parent)
{
    //get first available block
    fatentry_t first = getfirstunusedblock();
    if (first == 0) return 0;
    
    //set the new directory's information in it's parent dir
    //virtualDisk[loc.blocki].dir.entrylist[loc.entryi].entrylength = 0; removed for now
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].isdir = 1;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].unused = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].modtime = time(NULL);
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].filelength = 0;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock = first;
    strncpy(virtualDisk[loc.blocki].dir.entrylist[loc.entryi].name, dirname, MAXNAME);
    
    //initialise all entries in new block to unused
    for (int j = 0; j < DIRENTRYCOUNT; j++) virtualDisk[first].dir.entrylist[j].unused = 1;
    virtualDisk[first].dir.parent = parent;
    
    //set FAT entry to end of chain
    FAT[first] = ENDOFCHAIN;
    copyFAT();
    
    return first;
}


/* return the location of the first free entry in a given dir
 * link new dirblock if need be
 * if no space for new dirblock, return.blocki will be 0
 */

direntryloc getfreeentry(fatentry_t dirindex)
{
    direntryloc i;
    i.blocki = dirindex;
    while (!NULL) {
        for (i.entryi = 0; i.entryi < DIRENTRYCOUNT; i.entryi++){
            if (virtualDisk[i.blocki].dir.entrylist[i.entryi].unused == 1) {
                return i;
            }
        }
        if (FAT[i.blocki] == ENDOFCHAIN) { //check if we need to make another dirblock
            fatentry_t curblock = i.blocki;
            i.blocki = getfirstunusedblock();
            i.entryi = 0;
            if (i.blocki > 0) {
                virtualDisk[i.blocki].dir.parent = virtualDisk[dirindex].dir.parent; //copy parent from directory start
                for (int j = 0; j < DIRENTRYCOUNT; j++) {
                    virtualDisk[i.blocki].dir.entrylist[j].unused = 1; //set all unused
                }
                FAT[curblock] = i.blocki;
                FAT[i.blocki] = ENDOFCHAIN;
                copyFAT();
            }
            return i;          
        } else {
            i.blocki = FAT[i.blocki];
        }
    }
}


/* MyFILE open/close
 */

MyFILE *myfopen(const char *filepath, const char *mode)
{
    printf("myfopen> path = '%s', mode = '%s'\n", filepath, mode);
    
    //check mode
    if (strcmp(mode, "w") != 0 && strcmp(mode, "r") != 0) {
        printf("ERROR: expected 'w'|'r' (given '%s')", mode);
        return NULL;
    }
    
    //allocate and initialise MyFILE pointer
    MyFILE *myfp = malloc(sizeof(MyFILE));
    myfp->pos = 0;
    myfp->tot = 0;
    if (!strncpy(myfp->mode, mode, 3)){
        printf("ERROR: mode assignment failed ('%s')", mode);
        free(myfp);
        return NULL;
    }
    
    //navigate to directory
    direntryloc loc;
    if (filepath[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * path = calloc(strlen(filepath)+1, sizeof(char)); //need to allocate memory here
    strcpy(path, filepath);
    char * remainingPath = NULL;
    char * token = strtok_r(path, "/", &remainingPath);
    while (strlen(remainingPath) > 0) {
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki); //find the token in current dir
        }
        
        if (loc.entryi < 0) {
            printf("ERROR: directory '%s' not found on path '%s'\n", token, filepath);
            goto cleanup;
        }
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update 
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }

    loc = getdirentryloc(token, loc.blocki);
    fatentry_t myblockno;
    if (loc.entryi < 0) { //if file does not exist
        if (strcmp(mode, "r") == 0) { //read mode, file not found error
            printf("ERROR: file '%s' not found for read mode\n", token);
            goto cleanup;
        } else { //write mode, need to make a new file
            //i = virtualDisk[currentDirIndex].dir.nextEntry;
            loc = getfreeentry(loc.blocki);
            //check if directory is full
            if (loc.blocki == 0) {
                printf("ERROR: virtual disk is full (cannot extend directory)");
                goto cleanup;
            }
            myblockno = makenewfile(token, loc);
            if ( myblockno == 0) {
                printf("ERROR: virtual disk is full (cannot allocate file space)\n");
                goto cleanup;
            }
        }
    } else { //file exists, get the first block
        myblockno = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock;
    }
    free(path);
    
    //file exists or has been created, finish initializing myfp
    myfp->blockno = myblockno;
    myfp->loc = loc;
    readblock(&myfp->buffer, myblockno);
    return myfp;
    
    //sentinel, error handling
    cleanup:
        free(path);
        free(myfp);
        return NULL;
}

void myfclose(MyFILE *stream)
{
    if (stream == NULL) {
        printf("ERROR: stream not open\n");
        return;
    }
    
    printf("myfclose> file = '%s'\n", 
    virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].name);
    
    //write contents of buffer to current block
    writeblock(&stream->buffer, stream->blockno);
    virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].modtime = time(NULL);
    
    //check if we wrote past the previous EOF, update file length if so
    if (stream->tot > virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].filelength) {
        virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].filelength = stream->tot;
    }
    
    //free memory allocation
    free(stream);
}


/* Read to and write from MyFILE streams
 */

void myfputc(int b, MyFILE *stream)
{
    //first check if we need to change blocks
    if (stream->pos >= BLOCKSIZE) {
        if (FAT[stream->blockno] == ENDOFCHAIN) {
            //current block is EOC
            //we need the next unused block to extend the chain
            fatentry_t first = getfirstunusedblock();
            if (first == 0){
                printf("ERROR: virtual disk is full, closing '%s'\n", 
                virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].name);
                myfclose(stream);
                return;
            } else {
                //clear the contents of the block, it may contain leftover data
                diskblock_t block;
                for (int i = 0; i < BLOCKSIZE; i++) block.data[i] = '\0';
                writeblock(&block, first);
                //extend the FAT chain
                FAT[stream->blockno] = first;
                FAT[first] = ENDOFCHAIN;
                copyFAT();
            }
        }
        //current block should not be EOC
        //write contents of buffer to current block
        writeblock(&stream->buffer, stream->blockno);
        //update stream
        stream->pos = 0;
        stream->blockno = FAT[stream->blockno];
        //read block to buffer
        readblock(&stream->buffer, stream->blockno);
    }
    //write byte to buffer and increment pos, tot
    memmove(&stream->buffer.data[stream->pos], &b, sizeof(Byte));
    stream->pos++;
    stream->tot++;
}

int myfgetc(MyFILE *stream)
{
    //check for EOF
    if (stream->tot >= virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].filelength) return EOF;
    
    //check to see if next block needs to be loaded
    if (stream->pos >= BLOCKSIZE) {
        if (FAT[stream->blockno] == ENDOFCHAIN) {
            return EOF; // ensure we never read past the end of the chain
        } else {
            //write current block to disk image
            writeblock(&stream->buffer, stream->blockno);
            //update stream
            stream->pos = 0;
            stream->blockno = FAT[stream->blockno];
            //read block to buffer
            readblock(&stream->buffer, stream->blockno);
        }
    }
    
    //get next byte
    int b = stream->buffer.data[stream->pos];
    stream->pos++;
    stream->tot++;
    return b;
}


/* make directory
 */

void mymkdir(const char *path)
{  
    printf("mymkdir> path = '%s'\n", path);
    
    direntryloc loc;
    if (path[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * mypath = calloc(strlen(path)+1, sizeof(char));
    strcpy(mypath, path);
    char * remainingPath = NULL;
    char * token = strtok_r(mypath, "/", &remainingPath);
    while (token) {
        fatentry_t cdir = loc.blocki; //value may be needed as parent value later
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki);
        }
        
        if (loc.entryi < 0) { //dir not found in directory starting @ loc.blocki
            loc = getfreeentry(loc.blocki); //get first free direntry in current dir
            if (loc.blocki == 0) { //getfreeentry couldn't initialise another dirblock
                printf("ERROR: virtual disk is full (cannot extend directory)\n");
                goto cleanup;
            }
            
            fatentry_t myblockno = makenewdir(token, loc, cdir);
            if ( myblockno == 0) {
                printf("ERROR: virtual disk is full (cannot allocate directory space for %s)\n", token);
                goto cleanup;
            }
        }
        
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }
    
    cleanup:
        free(mypath);
}


/* return a list of the names of all in-use entries in a directory
 * ATTENTION: list is memory allocated and 
 * should be freed with freelist() after use
 */

char **mylistdir(const char *path)
{
    printf("mylistdir> path = '%s'\n", path);
    
    //handle absolute and relative paths
    direntryloc loc;
    if (path[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * mypath = calloc(strlen(path)+1, sizeof(char));
    strcpy(mypath, path);
    char * remainingPath = NULL;
    char * token = strtok_r(mypath, "/", &remainingPath);
    while (token) {
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki);
        }
        if (loc.entryi < 0) {
            printf("ERROR: directory '%s' not found on path '%s'\n", token, path);
            goto cleanup;
        }
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update 
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }
    
    char **strings = NULL;
    int strcount = 0;
    while (!NULL) {
        for (int i = 0; i < DIRENTRYCOUNT; i++){
            if (virtualDisk[loc.blocki].dir.entrylist[i].unused == 0) {
                strings = realloc(strings, (strcount + 1) * sizeof(char *));
                strings[strcount++] = strdup(virtualDisk[loc.blocki].dir.entrylist[i].name);
            }
        }
        if (FAT[loc.blocki] == ENDOFCHAIN) { //check for end of chain
            //add null as last element and return
            strings = realloc(strings, (strcount + 1) * sizeof(char *));
            strings[strcount] = NULL;
            free(mypath); //don't forget this
            return strings;
        } else { //update and continue
            loc.blocki = FAT[loc.blocki];
        }
    }    
    cleanup:
        free(mypath);
        return NULL;
}


/* change directory
 */
 
void mychdir( const char * path )
{
    printf("mychdir> path = '%s'\n", path);
    
    direntryloc loc;
    if (path[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * mypath = calloc(strlen(path)+1, sizeof(char));
    strcpy(mypath, path);
    char * remainingPath = NULL;
    char * token = strtok_r(mypath, "/", &remainingPath);
    while (token) {
        fatentry_t cdir = loc.blocki;
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki);
        }
        
        if (loc.entryi < 0) { //dir not found in directory starting @ loc.blocki
            printf("ERROR: directory '%s' not found in path '%s'\n", token, path);
            goto cleanup;
        }
        
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }
    currentDirIndex = loc.blocki;
    cleanup:
        free(mypath);
}


/* helper functions for file and dir removal
 */

void tidyFAT(fatentry_t cur, fatentry_t prev)
{
    fatentry_t next;
    if (prev == 0) {
        prev = cur;
        next = FAT[cur];
        goto cont;
    }
    
    for (int i = 0; i < DIRENTRYCOUNT; i++){
        if (virtualDisk[cur].dir.entrylist[i].unused == 0) {
            prev = cur;
            goto cont;
        }
    }
    
    FAT[prev] = FAT[cur];
    FAT[cur] = UNUSED;
    
    cont:
        if (next != ENDOFCHAIN) tidyFAT(next, prev);
}

void clearFAT(fatentry_t block)
{
    if (FAT[block] != ENDOFCHAIN) clearFAT(FAT[block]);
    FAT[block] = UNUSED;
}


/* removal functions
 */

void myremove(const char *filepath)
{
    printf("myremove> path = '%s'\n", filepath);

    //navigate to directory
    direntryloc loc;
    if (filepath[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * path = calloc(strlen(filepath)+1, sizeof(char)); //need to allocate memory here
    strcpy(path, filepath);
    char * remainingPath = NULL;
    char * token = strtok_r(path, "/", &remainingPath);
    while (strlen(remainingPath) > 0) {
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki); //find the token in current dir
        }
        
        if (loc.entryi < 0) {
            printf("ERROR: directory '%s' not found on path '%s'\n", token, filepath);
            goto cleanup;
        }
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update 
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }
    
    fatentry_t dirindex = loc.blocki; //save the location of the first block of directory
    loc = getdirentryloc(token, dirindex);
    fatentry_t firstblock;
    if (loc.entryi < 0) { //if file does not exist
        printf("ERROR: file not found.\n");
        goto cleanup;
    } else { //file exists, get the first block
        firstblock = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock;
    }

    //mark the directory entry as unused
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].unused = 1;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].modtime = time(NULL);

    //clear the FAT chain for the file
    clearFAT(firstblock);
    tidyFAT(dirindex, 0); //removes empty directory blocks
    copyFAT();
    
    //sentinel, error handling
    cleanup:
        free(path);
}

void myrmdir(const char *filepath)
{
    printf("myrmdir> path = '%s'\n", filepath);

    //navigate to directory
    direntryloc loc;
    if (filepath[0] == '/') {
        loc.blocki = rootDirIndex;
    } else {
        loc.blocki = currentDirIndex;
    }
    
    char * path = calloc(strlen(filepath)+1, sizeof(char)); //need to allocate memory here
    strcpy(path, filepath);
    char * remainingPath = NULL;
    char * token = strtok_r(path, "/", &remainingPath);
    while (strlen(remainingPath) > 0) {
        if (strcmp(token, "..") == 0) { //handle going up one level
            if (virtualDisk[loc.blocki].dir.parent <= 0) {
                printf("ERROR: cannot go up from root\n");
                goto cleanup;
            } else {
                loc.blocki = virtualDisk[loc.blocki].dir.parent;
                goto next;
            }
        } else if (strcmp(token, ".") == 0) { //continue, "." references current dir
            goto next;
        } else {
            loc = getdirentryloc(token, loc.blocki); //find the token in current dir
        }
        
        if (loc.entryi < 0) {
            printf("ERROR: directory '%s' not found on path '%s'\n", token, filepath);
            goto cleanup;
        }
        loc.blocki = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock; //update 
        next:
            token = strtok_r(NULL, "/", &remainingPath);
    }
    
    loc = getdirentryloc(token, loc.blocki);
    fatentry_t firstblock;
    if (loc.entryi < 0) { //if dir does not exist
        printf("ERROR: directory not found.\n");
        goto cleanup;
    } else { //dir exists, get the first block
        firstblock = virtualDisk[loc.blocki].dir.entrylist[loc.entryi].firstblock;
    }
    
    //check there are no files left in the directory before trying to clear it
    for (int i = 0; i < DIRENTRYCOUNT; i++){
        if ((virtualDisk[firstblock].dir.entrylist[i].unused == 0) || (FAT[firstblock] != ENDOFCHAIN)) {
            printf("WARNING: directory not empty, aborting removal.\n");
            goto cleanup;
        }
    }
    
    //mark the directory entry as unused
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].unused = 1;
    virtualDisk[loc.blocki].dir.entrylist[loc.entryi].modtime = time(NULL);

    //clear the FAT for the empty directory
    FAT[firstblock] = UNUSED;
    copyFAT();
    
    //sentinel, error handling
    cleanup:
        free(path);
}

/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

void printlist( char ** list )
{
    int i = 0;
    while (list[i] != NULL) {
        printf("\t%s\n", list[i]);
        i++;
    }
}

void freelist( char ** list )
{
    int i = 0;
    while (list[i] != NULL) {
        free(list[i]);
        i++;
    }
    free(list[i]);
    free(list);
}

void writejunk(MyFILE *stream, int blocks)
{
    printf("Writing %d blocks (%d bytes) to '%s'...\n", 
    blocks, 
    blocks * BLOCKSIZE, 
    virtualDisk[stream->loc.blocki].dir.entrylist[stream->loc.entryi].name);
    
    for (int i = 0; i < (blocks * BLOCKSIZE) - 1; i++) myfputc((i % 26) + 'A', stream);
    myfputc('\0', stream);
    printf("done.\n");
}