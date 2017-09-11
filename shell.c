#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Expected 1 argument, %d given\nUsage: >./shell [cgs band]\n",
        argc-1);
        return 1;
    }
    
    switch(argv[1][0]) {
        
        //D3-D1
        case 'd':
        case 'D':
            format();
            writedisk("virtualdiskD3_D1");
            break;
        
        //C3-C1
        case 'c':
        case 'C':
            format();
            
            //create testfile.txt and fill it with 4 blocks of repeating data
            MyFILE *myfp = myfopen("testfile.txt", "w");
            writejunk(myfp, 4);
            myfclose(myfp);
            
            //save virtual disk image
            writedisk("virtualdiskC3_C1");
            
            //open testfile.txt again, read and print contents byte-by-byte
            //to stdout, and to testfileC3_C1_copy.txt
            myfp = myfopen("testfile.txt", "r");
            FILE *fp = fopen("testfileC3_C1_copy.txt", "w");
            int b;
            printf("Reading file and writing to stdout/copy...\n");
            while ((b = myfgetc(myfp))!= EOF) {
                printf("%c", b);
                fputc(b, fp);
            }
            printf("\n");
            printf("done.\n");
            myfclose(myfp);
            fclose(fp);
            break;
        
        //B3-B1
        case 'b':
        case 'B':
            format();
            mymkdir("/myfirstdir/myseconddir/mythirddir");
            char **list = mylistdir("/myfirstdir/myseconddir");
            printlist(list);
            freelist(list);
            writedisk("virtualdiskB3_B1_a");
            
            myfp = myfopen("/myfirstdir/myseconddir/testfile.txt", "w");
            myfclose(myfp);
            list = mylistdir("/myfirstdir/myseconddir");
            printlist(list);
            freelist(list);
            writedisk("virtualdiskB3_B1_b");
            break;
        
        //A3-A1
        case 'a':
        case 'A':
            format();
            
            mymkdir("/firstdir/seconddir");
            myfp = myfopen("/firstdir/seconddir/testfile1.txt", "w");
            writejunk(myfp, 1);
            myfclose(myfp);
            
            list = mylistdir("/firstdir/seconddir");
            printlist(list);
            freelist(list);
            
            mychdir("/firstdir/seconddir");
            list = mylistdir(".");
            printlist(list);
            freelist(list);
            
            myfp = myfopen("testfile2.txt", "w");
            writejunk(myfp, 1);
            myfclose(myfp);
            
            mymkdir("thirddir");
            myfp = myfopen("thirddir/testfile3.txt", "w");
            writejunk(myfp, 1);
            myfclose(myfp);
            
            writedisk("virtualdiskA5_A1_a");
            
            myremove("testfile1.txt");
            myremove("testfile2.txt");
            
            writedisk("virtualdiskA5_A1_b");
            
            mychdir("thirddir");
            myremove("testfile3.txt");
            writedisk("virtualdiskA5_A1_c");
            
            mychdir("..");
            myrmdir("thirddir");
            
            mychdir("/firstdir");
            myrmdir("seconddir");
            
            mychdir("..");
            myrmdir("firstdir");
            writedisk("virtualdiskA5_A1_d");
            break;
            
        case 'x':
            //demonstrates that directories are extended and contracted
            //when files are added and removed
            format();
            myfp = myfopen("testfile1.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile2.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile3.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile4.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile5.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile6.txt", "w");
            myfclose(myfp);
            myfp = myfopen("testfile7.txt", "w");
            myfclose(myfp);
            writedisk("virtualdiskTEST_a");
            
            myremove("testfile4.txt");
            myremove("testfile5.txt");
            myremove("testfile6.txt");
            writedisk("virtualdiskTEST_b");
            
            myremove("testfile7.txt");
            writedisk("virtualdiskTEST_c");
            break;
        
        //sentinel
        default:
            printf("Expected CGS band as arg, '%c' given\nUsage: >./shell [cgs band]\n",
            argv[1][0]);
            return 1;
    }
    return 0;
}