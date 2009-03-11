#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char * getString(const char*nam, int *argc, char***argv) {
    size_t namlen=strlen(nam);
    char * retval=NULL;
    int i=1;
    while (i<*argc) {
        if (strlen((*argv)[i])>=namlen&&memcmp((*argv)[i],nam,namlen)==0) {
            int j;
            /*match beginning;*/
            retval=(*argv)[i]+namlen;
            for (j=i;j+1<*argc;++j) {
                (*argv)[j]=(*argv)[j+1];
            }
            --*argc;
        }else {
            ++i;
        }
    }
    return retval;
}

int main(int argc, char **argv) {
        int c;
        int count = 0;
        FILE * inputfp=stdin;
        FILE * outputfp=stdout;
        unsigned long long size=0;
        int dostatic=0;
        const char * namespacename=getString("-namespace=",&argc,&argv);
        const char * sizevar = getString("-size=",&argc,&argv);
        const char * exportvar=getString("-export=",&argc,&argv);
        const char * pchvar=getString("-pch=",&argc,&argv);
        if (getString("-static",&argc,&argv)!=NULL)
            dostatic=1;
        if (argc < 2) {
                fprintf(stderr, "Usage: %s arrayName\n", argv[0]);
                return 1;
        }
        if (argc >= 3) {
            inputfp=fopen(argv[2],"rb");
        }
        if (argc >= 4) {
            outputfp=fopen(argv[3],"w");
        }
        if (pchvar) {
            fprintf(outputfp,"#include \"%s\"\n",pchvar);
        }
        if (namespacename) {
            fprintf(outputfp,"namespace %s {\n",namespacename);
        }
        if (dostatic)
            fprintf(outputfp,"static ");
        if (exportvar) {
            fprintf(outputfp,"%s ",exportvar);
        }
        fprintf(outputfp,"unsigned char %s[] = {\n", argv[1]);
        
        if((c=fgetc(inputfp))!=-1) {
                fprintf(outputfp,"%d",c);
                ++count;++size;
        }
        while ((c=fgetc(inputfp))!=-1) {
                fprintf(outputfp,",%d",c);
                ++size;
                if (++count > 24) {
                        fprintf(outputfp,"\n");
                        count = 0;
                }
        }
        fprintf(outputfp,"\n};\n");
        if (sizevar) {
            unsigned int siz=(unsigned int)size;
            if (dostatic) {
                fprintf(outputfp,"static ");
            }
            if (exportvar) {
                fprintf(outputfp,"%s ",exportvar);
            }
            if (!(size<(1<<30))) {
                fprintf (stderr,"Do not know how to deal with files of this magnitude!\n");
            }
            fprintf(outputfp,"unsigned int %s = %d;\n",sizevar,siz);
        }
        if (namespacename)
            fprintf(outputfp,"}\n");        
        fclose(outputfp);
        fclose(inputfp);
        return 0;
}
