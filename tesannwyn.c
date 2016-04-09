/* TESAnnwyn: A TES3 height map importer/exporter (to & from RAW or BMP).
 *
 * Paul Halliday: 31-Dec-2006
 * Bret Curtis: 2015
 *
 * This is entirely my own work. No borrowed code.
 * All reverse engineering has been researched by myself.
 *
 * License: GNU (Copy, modify, distribute as you like.
 * Please just credit me if you borrow code. ;)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>

#include "defs.h"

#include "tes3_import.h"
#include "tes3_export.h"

int DecodeDimensions(char *, int *, int *);

int DecodeFilenames(char *);

int DecodeOptIgnoreLand(char *, int *, int *);

int DecodeLimits(char *, int *opt_lower_limit, int *opt_upper_limit);

int ShowUsageExit(char *);

int main(int argc, char *argv[]) {
    int argn = 0;
    int opt_mode = UNKNOWN,
            opt_bpp = 16,
            opt_image_type = UNKNOWN_IMAGE,
            opt_sx = 1024,
            opt_sy = 1024,
            opt_x_cell_offset = 0,
            opt_y_cell_offset = 0,
            opt_adjust_height = 0,
            opt_rescale = 0,
            opt_vclr = 0,
            opt_limit = 0,
            opt_lower_limit = -2147483647,
            opt_upper_limit = 2147483647,
            opt_grid = -1,
            opt_ignore_land_upper = -1073741824,
            opt_ignore_land_lower = 1073741824;

    float opt_scale = 1.0;

    char c,
            file_header[256];

    char opt_ignore_land_string[64],
            opt_dimensions_string[48],
            opt_limit_string[48],
            opt_texture[32];

    opt_texture[0] = '\0';

    FILE *fp_in;

    printf("************************************************************************\n"
                   "* TESAnnwyn: A RAW/BMP heightmap importer/exporter for TES3/Morrowind. *\n"
                   "************************************************************************\n\n");

    /***********************************
     * Parse the command line arguments.
     **********************************/
    argn = argc;
    while ((c = getopt(argc, argv, "d:x:y:s:h:p:b:o:t:l:cgrvV")) != EOF) {
        switch (c) {
            case 'c':
                opt_vclr = 1;
                break;
            case 'g':
                opt_grid = 0;
                break;
            case 'b':
                if (optarg) {
                    opt_bpp = atoi(optarg);
                    argn -= 2;
                } else
                    ShowUsageExit(argv[0]);
                break;
            case 'p':
                if (optarg) {
                    opt_image_type = atoi(optarg);
                    argn -= 2;
                } else
                    ShowUsageExit(argv[0]);
                break;
            case 'x':
                if (optarg) {
                    opt_x_cell_offset = atoi(optarg);
                    argn -= 2;
                } else
                    ShowUsageExit(argv[0]);
                break;
            case 'y':
                if (optarg) {
                    opt_y_cell_offset = atoi(optarg);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 't':
                if (optarg) {
                    strcpy(opt_texture, optarg);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 'd':
                if (optarg) {
                    strcpy(opt_dimensions_string, optarg);
                    DecodeDimensions(opt_dimensions_string, &opt_sx, &opt_sy);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 's':
                if (optarg) {
                    opt_scale = (float) atof(optarg);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 'h':
                if (optarg) {
                    opt_adjust_height = atoi(optarg);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 'l':
                if (optarg) {
                    strcpy(opt_limit_string, optarg);
                    DecodeLimits(opt_limit_string, &opt_lower_limit, &opt_upper_limit);
                    argn -= 2;
                    opt_limit = 1;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 'o':
                if (optarg) {
                    strcpy(opt_ignore_land_string, optarg);
                    DecodeOptIgnoreLand(opt_ignore_land_string, &opt_ignore_land_lower, &opt_ignore_land_upper);
                    argn -= 2;
                } else {
                    ShowUsageExit(argv[0]);
                }
                break;
            case 'r':
                opt_rescale = 1;
                break;
            case 'v':
            case 'V':
                printf("Program Name: %s\n"
                               "Version:      0.25\n"
                               "Date:         2006 -> 2016\n"
                               "Author:       Paul Halliday and Bret Curtis\n",
                       APP_NAME);
                exit(0);
            default:
                ShowUsageExit(argv[0]);
        }
    }

    if (argn < 2) {
        ShowUsageExit(argv[0]);
    }

    DecodeFilenames(argv[argc - 1]);

    // Double check the option file details: Is the input file a TES3, TES4, BMP, etc.

    /* ************************************
     * File type check (BMP or TES3) ...
     * ************************************
     */

    if ((fp_in = fopen(input_files.filename[0], "rb")) == 0) {
        fprintf(stderr, "Cannot open %s for reading: %s\n",
                input_files.filename[0], strerror(errno));
        exit(1);
    }

    if (!fread(file_header, 4, 1, fp_in)) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    if (strncmp("TES3", file_header, 4) == 0) {
        printf("Looks like a TES3 file. Assuming want to export this to an image.\n");
        opt_mode = EXPORT;
    } else if (strncmp("BM", file_header, 2) == 0) {
        printf("This is a BMP. You must want to import this image.\n");
        opt_mode = IMPORT;
        opt_image_type = BMP;
    } else {
        opt_mode = IMPORT;
        opt_image_type = RAW;
    }

    fclose(fp_in);

    printf("Using a height scale factor of: %f.\n", opt_scale);
    printf("Using a height offset value of: %d.\n", opt_adjust_height);

    if (opt_limit && opt_mode == IMPORT) {
        printf("Imported land below %d game units and above %d game units will be flattened.\n",
               opt_lower_limit, opt_upper_limit);
        opt_upper_limit /= 8;
        opt_lower_limit /= 8;
    }

    if (opt_texture[0] != '\0') {
        printf("Landscape will be textured with texture number '%s'\n\n", opt_texture);
    }

    if (opt_mode == IMPORT) {
        if (opt_ignore_land_lower < 1073741824 && opt_ignore_land_upper > -1073741824) {
            printf("Landscape only containing heights between %d and %d will not be imported.\n",
                   opt_ignore_land_lower, opt_ignore_land_upper);
        }
        ImportImage(input_files.filename[0], opt_bpp, opt_vclr, opt_sx, opt_sy,
                    opt_image_type, opt_rescale, opt_adjust_height,
                    opt_limit, opt_lower_limit, opt_upper_limit, opt_x_cell_offset,
                    opt_y_cell_offset, opt_ignore_land_upper, opt_ignore_land_lower,
                    opt_texture, opt_scale);
    } else {
        ExportImages(opt_image_type, opt_bpp, opt_vclr, opt_grid,
                     opt_adjust_height, opt_rescale, opt_scale);
    }

    return 0;
}


int DecodeOptIgnoreLand(char *s, int *opt_ignore_land_lower, int *opt_ignore_land_upper) {
    int i;
    int p = 0;
    char tmp[64];

    if (s[0] == '-') p++;

    for (i = 0; s[i + p] != '-' && s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    *opt_ignore_land_lower = atoi(tmp);
    if (s[0] == '-') *opt_ignore_land_lower *= -1;

    p += i + 1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    if (tmp[0] == '\0') {
        //printf("Fixing.\n");
        *opt_ignore_land_upper = *opt_ignore_land_lower;
    } else {
        *opt_ignore_land_upper = atoi(tmp);
    }

    return 0;
}

int DecodeLimits(char *s, int *opt_lower_limit, int *opt_upper_limit) {
    int i;
    int p = 0;
    char tmp[64];

    for (i = 0; s[i + p] != ',' && s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    *opt_lower_limit = atoi(tmp);

    p += i + 1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    if (tmp[0] == '\0') {
        //printf("Fixing.\n");
        *opt_upper_limit = 2147483647;
    } else {
        *opt_upper_limit = atoi(tmp);
    }

    return 0;
}

int DecodeDimensions(char *s, int *opt_sx, int *opt_sy) {
    int i;
    int p = 0;
    char tmp[64];

    for (i = 0; s[i + p] != 'x' && s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    *opt_sx = atoi(tmp);

    p += i + 1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i + p], i++);
    tmp[i] = '\0';

    *opt_sy = atoi(tmp);

    return 0;
}

int DecodeFilenames(char *s) {
    int i = 0,
            p = 0;

    input_files.count = 0;

    while (s[p] != '\0') {
        for (i = 0; s[i + p] != ',' && s[i + p] != '\0'; input_files.filename[input_files.count][i] = s[i + p], i++);
        input_files.filename[input_files.count][i] = '\0';
        input_files.count++;

        if (s[i + p] == '\0') break;
        p += i + 1;
    }

    return 0;
}

/************************************************************************
 ** ShowUsageExit: Show how to use the command line arguments, then exit.
 ***********************************************************************/
int ShowUsageExit(char *argv0) {
    fprintf(stderr, "Please see the associated Readme document for more detailed help.\n\n"
                    "Usage: %s [options] filename\n\n"
                    "\tfilename: The image filename you want to import or ESP/ESM to export.\n\n"
                    "\t-c: Import/Export the VCLR vertex colour image as a BMP called %s.\n"
                    "\t-d (numxnum): X and Y dimensions of import image. e.g. 2688x1344 (default is 1024x1024).\n"
                    "\t-p (num): Type of image file ( 1 = RAW, 2 = BMP, 3 = CSV).\n"
                    "\t-b (num): Bits per pixel of the input/output image RAW/BMP image file. For BMP, 8 bit implies Greyscale.\n"
                    "\t-x (num): The Bottom-Left cell X co-ordinate will start here (default 0).\n"
                    "\t-y (num): The Bottom-Left cell Y co-ordinate will start here (default 0).\n"
                    "\t-h (num): Offset height of land by this number of game units.\n"
                    "\t-s (num): Scale land height by this number (floating point allowed).\n"
                    "\t-l (num,num): Limit all imported height to within these lower,upper limits (e.g. -2048,32768).\n"
                    "\t-o (num-num): Do not import land whose land is all between these heights (e.g. -2048 or -4096-1024).\n"
                    "\t-g: Draw square cell sized grids on all image exports.\n"
                    "\t-t (num/FormID): Texture the entire landscape in a texture number.\n"
                    "\t-r: (Greyscale 8-bit BMP only). Automatically rescale the Greyscale export between 0-255. (No parameter required).\n"
                    "\t-q : Quiet mode: Outputs less so runs faster in a Windows Console.\n"
                    "\t-v : Display current Version, then exit.\n",
            argv0, TA_VCLR_IN
    );

    exit(1);
}
