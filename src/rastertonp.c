/*
 * Star Micronics
 * 
 * CUPS Filter
 *
 * compile cmd: gcc -Wl,-rpath,/usr/lib -Wall -fPIC -O2 -o rastertonp rastertonp.c -lcupsimage -lcups
 * compile requires cups-devel-1.1.19-13.i386.rpm (version not neccessarily important?)
 * find cups-devel location here: http://rpmfind.net/linux/rpm2html/search.php?query=cups-devel&submit=Search+...&system=&arch=
 */

/*
 * Copyright (C) 2004 Star Micronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * Include necessary headers...
 */

#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef RPMBUILD

#include <dlfcn.h>

typedef cups_raster_t * (*cupsRasterOpen_fndef)(int fd, cups_mode_t mode);
typedef unsigned (*cupsRasterReadHeader_fndef)(cups_raster_t *r, cups_page_header_t *h);
typedef unsigned (*cupsRasterReadPixels_fndef)(cups_raster_t *r, unsigned char *p, unsigned len);
typedef void (*cupsRasterClose_fndef)(cups_raster_t *r);

static cupsRasterOpen_fndef cupsRasterOpen_fn;
static cupsRasterReadHeader_fndef cupsRasterReadHeader_fn;
static cupsRasterReadPixels_fndef cupsRasterReadPixels_fn;
static cupsRasterClose_fndef cupsRasterClose_fn;

#define CUPSRASTEROPEN (*cupsRasterOpen_fn)
#define CUPSRASTERREADHEADER (*cupsRasterReadHeader_fn)
#define CUPSRASTERREADPIXELS (*cupsRasterReadPixels_fn)
#define CUPSRASTERCLOSE (*cupsRasterClose_fn)

typedef void (*ppdClose_fndef)(ppd_file_t *ppd);
typedef ppd_choice_t * (*ppdFindChoice_fndef)(ppd_option_t *o, const char *option);
typedef ppd_choice_t * (*ppdFindMarkedChoice_fndef)(ppd_file_t *ppd, const char *keyword);
typedef ppd_option_t * (*ppdFindOption_fndef)(ppd_file_t *ppd, const char *keyword);
typedef void (*ppdMarkDefaults_fndef)(ppd_file_t *ppd);
typedef ppd_file_t * (*ppdOpenFile_fndef)(const char *filename);

typedef void (*cupsFreeOptions_fndef)(int num_options, cups_option_t *options);
typedef int (*cupsParseOptions_fndef)(const char *arg, int num_options, cups_option_t **options);
typedef int (*cupsMarkOptions_fndef)(ppd_file_t *ppd, int num_options, cups_option_t *options);

static ppdClose_fndef ppdClose_fn;
static ppdFindChoice_fndef ppdFindChoice_fn;
static ppdFindMarkedChoice_fndef ppdFindMarkedChoice_fn;
static ppdFindOption_fndef ppdFindOption_fn;
static ppdMarkDefaults_fndef ppdMarkDefaults_fn;
static ppdOpenFile_fndef ppdOpenFile_fn;

static cupsFreeOptions_fndef cupsFreeOptions_fn;
static cupsParseOptions_fndef cupsParseOptions_fn;
static cupsMarkOptions_fndef cupsMarkOptions_fn;

#define PPDCLOSE            (*ppdClose_fn)
#define PPDFINDCHOICE       (*ppdFindChoice_fn)
#define PPDFINDMARKEDCHOICE (*ppdFindMarkedChoice_fn)
#define PPDFINDOPTION       (*ppdFindOption_fn)
#define PPDMARKDEFAULTS     (*ppdMarkDefaults_fn)
#define PPDOPENFILE         (*ppdOpenFile_fn)

#define CUPSFREEOPTIONS     (*cupsFreeOptions_fn)
#define CUPSPARSEOPTIONS    (*cupsParseOptions_fn)
#define CUPSMARKOPTIONS     (*cupsMarkOptions_fn)

#else

#define CUPSRASTEROPEN cupsRasterOpen
#define CUPSRASTERREADHEADER cupsRasterReadHeader
#define CUPSRASTERREADPIXELS cupsRasterReadPixels
#define CUPSRASTERCLOSE cupsRasterClose

#define PPDCLOSE ppdClose
#define PPDFINDCHOICE ppdFindChoice
#define PPDFINDMARKEDCHOICE ppdFindMarkedChoice
#define PPDFINDOPTION ppdFindOption
#define PPDMARKDEFAULTS ppdMarkDefaults
#define PPDOPENFILE ppdOpenFile

#define CUPSFREEOPTIONS cupsFreeOptions
#define CUPSPARSEOPTIONS cupsParseOptions
#define CUPSMARKOPTIONS cupsMarkOptions

#endif

#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

#define FALSE 0
#define TRUE  (!FALSE)

// definitions for model number
#define NP326          326
#define NP211          211

// definitions for printable width
#define NP326_MAX_WIDTH 72
#define NP326_STD_WIDTH 72
#define NP211_MAX_WIDTH 48
#define NP211_STD_WIDTH 48

#define FOCUS_LEFT      0
#define FOCUS_CENTER    1
#define FOCUS_RIGHT     2

struct settings_
{
    int modelNumber;

    float pageWidth;
    float pageHeight;
    
    int printDensity;
    int pageType;
    int cutActionTiming;

    int bytesPerScanLine;
    int bytesPerScanLineStd;
};

struct command
{
    int    length;
    char* command;
};

// define printer initialize command
// timing: jobSetup.0
// models: all
static const struct command printerInitializeCommand =
{2,(char[2]){0x1b,'@'}};

// define print density commands
// timing: jobSetup.1
// models: all
static const struct command printDensityCommand [14] =
{{3,(char[3]){0x1d,0x7e,0x41}},                                   //  0 65 percent
 {3,(char[3]){0x1d,0x7e,0x46}},                                   //  0 70 percent
 {3,(char[3]){0x1d,0x7e,0x4b}},                                   //  0 75 percent
 {3,(char[3]){0x1d,0x7e,0x50}},                                   //  0 80 percent
 {3,(char[3]){0x1d,0x7e,0x55}},                                   //  0 85 percent
 {3,(char[3]){0x1d,0x7e,0x5a}},                                   //  0 90 percent
 {3,(char[3]){0x1d,0x7e,0x5f}},                                   //  0 95 percent
 {3,(char[3]){0x1d,0x7e,0x64}},                                   //  0 100 percent
 {3,(char[3]){0x1d,0x7e,0x69}},                                   //  0 105 percent
 {3,(char[3]){0x1d,0x7e,0x6e}},                                   //  0 110 percent
 {3,(char[3]){0x1d,0x7e,0x73}},                                   //  0 115 percent
 {3,(char[3]){0x1d,0x7e,0x78}},                                   //  0 120 percent
 {3,(char[3]){0x1d,0x7e,0x7d}},                                   //  0 125 percent
 {3,(char[3]){0x1d,0x7e,0x82}}};                                  //  0 130 percent

// define cut command
// timing: end page
// models: all
static const struct command cutCommand =
{5,(char[5]){0x1b,'J',0x78,0x1b,'i'}};                           // cut

// define end raster block command
// models: all
static const struct command endRasterBlockCommand =
{3,(char[3]){0x1b,'J',0x00}};

// define y relative command
// models: all
static const struct command yRelCommand =
{2,(char[2]){0x1b,'J'}};

inline void outputCommand(const struct command * output)
{
    int i = 0;

    for (; i < output->length; i++)
    {
        putchar(output->command[i]);
    }
}

inline void outputAsciiEncodedLength(int length)
{
    printf("%d",length);
}

inline void outputNullTerminator()
{
    putchar(0x00);
}

inline int getOptionChoiceIndex(const char * choiceName, ppd_file_t * ppd)
{
    ppd_choice_t * choice;
    ppd_option_t * option;

    choice = PPDFINDMARKEDCHOICE(ppd, choiceName);
    if (choice == NULL)
    {
        if ((option = PPDFINDOPTION(ppd, choiceName))          == NULL) return -1;
        if ((choice = PPDFINDCHOICE(option,option->defchoice)) == NULL) return -1;
    }

    return atoi(choice->choice);
}

inline void getPageWidthPageHeight(ppd_file_t * ppd, struct settings_ * settings)
{
    ppd_choice_t * choice;
    ppd_option_t * option;
    
    char width[20];
    int widthIdx;
    
    char height[20];
    int heightIdx;

    char * pageSize;
    int idx;

    int state;

    choice = PPDFINDMARKEDCHOICE(ppd, "PageSize");
    if (choice == NULL)
    {
        option = PPDFINDOPTION(ppd, "PageSize");
        choice = PPDFINDCHOICE(option,option->defchoice);
    }

    widthIdx = 0;
    memset(width, 0x00, sizeof(width));
    
    heightIdx = 0;
    memset(height, 0x00, sizeof(height));

    pageSize = choice->choice;
    idx = 0;

    state = 0; // 0 = init, 1 = width, 2 = height, 3 = complete, 4 = fail
    
    while (pageSize[idx] != 0x00)
    {
        if (state == 0)
        {
            if (pageSize[idx] == 'X')
            {
                state = 1;

                idx++;
                continue;
            }
        }
        else if (state == 1)
        {
            if ((pageSize[idx] >= '0') && (pageSize[idx] <= '9'))
            {
                width[widthIdx++] = pageSize[idx];

                idx++;
                continue;
            }
            else if (pageSize[idx] == 'D')
            {
                width[widthIdx++] = '.';

                idx++;
                continue;
            }
            else if (pageSize[idx] == 'M')
            {
                idx++;
                continue;
            }
            else if (pageSize[idx] == 'Y')
            {
                state = 2;

                idx++;
                continue;
            }
        }
        else if (state == 2)
        {
            if ((pageSize[idx] >= '0') && (pageSize[idx] <= '9'))
            {
                height[heightIdx++] = pageSize[idx];

                idx++;
                continue;
            }
            else if (pageSize[idx] == 'D')
            {
                height[heightIdx++] = '.';

                idx++;
                continue;
            }
            else if (pageSize[idx] == 'M')
            {
                state = 3;
                break;
            }
        }

        state = 4;
        break;
    }

    if (state == 3)
    {
        settings->pageWidth = atof(width);
        settings->pageHeight = atof(height);
    }
    else
    {
        settings->pageWidth = 0;
        settings->pageHeight = 0;
    }
}

inline void initializeSettings(char * commandLineOptionSettings, struct settings_ * settings)
{
    ppd_file_t *    ppd         = NULL;
    cups_option_t * options     = NULL;
    int             numOptions  = 0;
    int             modelNumber = 0;

    ppd = PPDOPENFILE(getenv("PPD"));

    PPDMARKDEFAULTS(ppd);

    numOptions = CUPSPARSEOPTIONS(commandLineOptionSettings, 0, &options);
    if ((numOptions != 0) && (options != NULL))
    {
        CUPSMARKOPTIONS(ppd, numOptions, options);

        CUPSFREEOPTIONS(numOptions, options);
    }

    memset(settings, 0x00, sizeof(struct settings_));

    modelNumber = settings->modelNumber = ppd->model_number;
    
    settings->printDensity          = getOptionChoiceIndex("PrintDensity"         , ppd);
    settings->pageType              = getOptionChoiceIndex("PageType"             , ppd);
    settings->cutActionTiming       = getOptionChoiceIndex("CutActionTiming"      , ppd);

    switch (modelNumber)
    {
        case NP326:     settings->bytesPerScanLine    = NP326_MAX_WIDTH;
                        settings->bytesPerScanLineStd = NP326_STD_WIDTH; break;
        case NP211:     settings->bytesPerScanLine    = NP211_MAX_WIDTH;
                        settings->bytesPerScanLineStd = NP211_STD_WIDTH; break;
    }

    getPageWidthPageHeight(ppd, settings);

    PPDCLOSE(ppd);
}

void jobSetup(struct settings_ settings)
{
    outputCommand(&printerInitializeCommand);
    
    outputCommand(&printDensityCommand[settings.printDensity]);
}

void pageSetup(struct settings_ settings, cups_page_header_t header)
{
}

void endPage(struct settings_ settings)
{
  if (settings.cutActionTiming == 1)
  {
    outputCommand(&cutCommand);
  }
}

void endJob(struct settings_ settings)
{
  if (settings.cutActionTiming == 2)
  {
    outputCommand(&cutCommand);
  }
}

#define GET_LIB_FN_OR_EXIT_FAILURE(fn_ptr,lib,fn_name)                                      \
{                                                                                           \
    fn_ptr = dlsym(lib, fn_name);                                                           \
    if ((dlerror()) != NULL)                                                                \
    {                                                                                       \
        fputs("ERROR: required fn not exported from dynamically loaded libary\n", stderr);  \
        if (libCupsImage != 0) dlclose(libCupsImage);                                       \
        if (libCups      != 0) dlclose(libCups);                                            \
        return EXIT_FAILURE;                                                                \
    }                                                                                       \
}

#ifdef RPMBUILD
#define CLEANUP                                                         \
{                                                                       \
    if (originalRasterDataPtr   != NULL) free(originalRasterDataPtr);   \
    CUPSRASTERCLOSE(ras);                                               \
    if (fd != 0)                                                        \
    {                                                                   \
        close(fd);                                                      \
    }                                                                   \
    dlclose(libCupsImage);                                              \
    dlclose(libCups);                                                   \
}
#else
#define CLEANUP                                                         \
{                                                                       \
    if (originalRasterDataPtr   != NULL) free(originalRasterDataPtr);   \
    CUPSRASTERCLOSE(ras);                                               \
    if (fd != 0)                                                        \
    {                                                                   \
        close(fd);                                                      \
    }                                                                   \
}
#endif

int main(int argc, char *argv[])
{
    int                 fd                      = 0;        /* File descriptor providing CUPS raster data                                           */
    cups_raster_t *     ras                     = NULL;     /* Raster stream for printing                                                           */
    cups_page_header_t  header;                             /* CUPS Page header                                                                     */
    int                 page                    = 0;        /* Current page                                                                         */
    
    long                 blockY                  = 0;
    int                 y                       = 0;        /* Vertical position in page 0 <= y <= header.cupsHeight                                */
    int                 i                       = 0;
    
    unsigned char *     rasterData              = NULL;     /* Pointer to raster data buffer                                                        */
    unsigned char *     originalRasterDataPtr   = NULL;     /* Copy of original pointer for freeing buffer                                          */
    int                 leftByteDiff            = 0;        /* Bytes on left to discard                                                             */
    int                 scanLineBlank           = 0;        /* Set to TRUE is the entire scan line is blank (no black pixels)                       */
    int                 lastBlackPixel          = 0;        /* Position of the last byte containing one or more black pixels in the scan line       */
    int                 numBlankScanLines       = 0;        /* Number of scanlines that were entirely black                                         */
    struct settings_    settings;                           /* Configuration settings                                                               */
    
#ifdef RPMBUILD
    void * libCupsImage = NULL;                             /* Pointer to libCupsImage library                                                      */
    void * libCups      = NULL;                             /* Pointer to libCups library                                                           */
    
    libCups = dlopen ("libcups.so", RTLD_NOW | RTLD_GLOBAL);
    if (! libCups)
    {
        fputs("ERROR: libcups.so load failure\n", stderr);
        return EXIT_FAILURE;
    }
    
    libCupsImage = dlopen ("libcupsimage.so", RTLD_NOW | RTLD_GLOBAL);
    if (! libCupsImage)
    {
        fputs("ERROR: libcupsimage.so load failure\n", stderr);
        dlclose(libCups);
        return EXIT_FAILURE;
    }
    
    GET_LIB_FN_OR_EXIT_FAILURE(ppdClose_fn,             libCups,      "ppdClose"             );
    GET_LIB_FN_OR_EXIT_FAILURE(ppdFindChoice_fn,        libCups,      "ppdFindChoice"        );
    GET_LIB_FN_OR_EXIT_FAILURE(ppdFindMarkedChoice_fn,  libCups,      "ppdFindMarkedChoice"  );
    GET_LIB_FN_OR_EXIT_FAILURE(ppdFindOption_fn,        libCups,      "ppdFindOption"        );
    GET_LIB_FN_OR_EXIT_FAILURE(ppdMarkDefaults_fn,      libCups,      "ppdMarkDefaults"      );
    GET_LIB_FN_OR_EXIT_FAILURE(ppdOpenFile_fn,          libCups,      "ppdOpenFile"          );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsFreeOptions_fn,      libCups,      "cupsFreeOptions"      );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsParseOptions_fn,     libCups,      "cupsParseOptions"     );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsMarkOptions_fn,      libCups,      "cupsMarkOptions"      );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsRasterOpen_fn,       libCupsImage, "cupsRasterOpen"       );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsRasterReadHeader_fn, libCupsImage, "cupsRasterReadHeader" );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsRasterReadPixels_fn, libCupsImage, "cupsRasterReadPixels" );
    GET_LIB_FN_OR_EXIT_FAILURE(cupsRasterClose_fn,      libCupsImage, "cupsRasterClose"      );
#endif

    if (argc < 6 || argc > 7)
    {
        fputs("ERROR: rastertostar job-id user title copies options [file]\n", stderr);
        
        #ifdef RPMBUILD
            dlclose(libCupsImage);
            dlclose(libCups);
        #endif
        
        return EXIT_FAILURE;
    }

    if (argc == 7)
    {
        if ((fd = open(argv[6], O_RDONLY)) == -1)
        {
            perror("ERROR: Unable to open raster file - ");
            sleep(1);

            #ifdef RPMBUILD
                dlclose(libCupsImage);
                dlclose(libCups);
            #endif
            
            return EXIT_FAILURE;
        }
    }
    else
    {
        fd = 0;
    }

    initializeSettings(argv[5], &settings);

    jobSetup(settings);

    ras = CUPSRASTEROPEN(fd, CUPS_RASTER_READ);

    page = 0;
    
    while (CUPSRASTERREADHEADER(ras, &header))
    {
        if ((header.cupsHeight == 0) || (header.cupsBytesPerLine == 0))
        {
            break;
        }
        
        if (rasterData == NULL)
        {
            rasterData = malloc(header.cupsBytesPerLine);
            if (rasterData == NULL)
            {
                CLEANUP;
                return EXIT_FAILURE;

            }
            originalRasterDataPtr = rasterData;  // used to later free the memory
        }

        pageSetup(settings, header);

        page++;
        fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);

        numBlankScanLines = 0;
      
        if (header.cupsBytesPerLine <= settings.bytesPerScanLine)
        {
            settings.bytesPerScanLine = header.cupsBytesPerLine;
            leftByteDiff = 0;
        }
        else
        {
            settings.bytesPerScanLine = settings.bytesPerScanLineStd;
            
            leftByteDiff = 0;
            
        }

        blockY = 0;
        for (y = 0; y < header.cupsHeight; y ++)
        {
            if ((y & 127) == 0)
            {
                fprintf(stderr, "INFO: Printing page %d, %d%% complete...\n", page, (100 * y / header.cupsHeight));
            }

            if (CUPSRASTERREADPIXELS(ras, rasterData, header.cupsBytesPerLine) < 1)
            {
                break;
            }

            rasterData += leftByteDiff;
        
            if (blockY == 0)
            {
                outputCommand(&endRasterBlockCommand);
                blockY = 0xffff;
                
                putchar((char) 0x1b);
                putchar((char) 'b');
                putchar((char) settings.bytesPerScanLine);
                
                int remainY = header.cupsHeight - y;
                if (remainY > 0xffff)
                {
                  putchar((char) 0xff);
                  putchar((char) 0xff);
                }
                else
                {
                  putchar((char)(remainY & 0x00ff));
                  putchar((char)((remainY & 0xff00) >> 8));
                }
            }
            
            for (i = 0; i < settings.bytesPerScanLine; i++)
            {
                putchar((char) *(rasterData + i));
            }
            
            blockY--;

            rasterData = originalRasterDataPtr;
        }
        
        outputCommand(&endRasterBlockCommand);
    
        endPage(settings);
    }

    endJob(settings);

    CLEANUP;

    if (page == 0)
    {
        fputs("ERROR: No pages found!\n", stderr);
    }
    else
    {
        fputs("INFO: Ready to print.\n", stderr);
    }

    return (page == 0)?EXIT_FAILURE:EXIT_SUCCESS;
}

// end of rastertostar.c
