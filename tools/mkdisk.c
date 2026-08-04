/*
* Copyright (c) 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Project Oberon System project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

/* Build a Project Oberon disk image: the file system consists of 1 KByte
   sectors, whose numbers are multiples of 29 (Kernel.AllocSector); sector 29
   is the root page of the directory B-tree (FileDir.DirRootAdr) and each file
   starts with a header sector carrying the name and the page table.
   The files to include are read from a list, one host path per line; the file
   name on the disk is the last path component. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

enum { SS = 1024 };             /* FileDir.SectorSize */
enum { HS = 352 };              /* FileDir.HeaderSize */
enum { STS = 64 };              /* FileDir.SecTabSize */
enum { FnLength = 32 };         /* FileDir.FnLength */
enum { DirPgSize = 24 };        /* FileDir.DirPgSize, entries per page */
enum { SectorMult = 29 };       /* disk addresses are multiples of 29 */
enum { DirRootAdr = 29 };       /* FileDir.DirRootAdr, i.e. sector 1 */
enum { DirMark = 0x9B1EA38D };  /* FileDir.DirMark */
enum { HeaderMark = 0x9BA71D86 };
enum { DEFAULT_SECTORS = 1024 };/* 1 MByte */

typedef struct Entry
{
    char name[FnLength];
    uint32_t adr;               /* sector number of the file header */
} Entry;

static uint8_t* disk = NULL;    /* the whole image, sector 0 unused */
static uint32_t nofSectors = 0; /* size of the image in sectors */
static uint32_t nextSector = 2; /* sector 1 is the root directory page */

static Entry dir[DirPgSize];
static int nofEntries = 0;

static uint8_t* sector(uint32_t s)
{
    if( s == 0 || s >= nofSectors )
    {
        fprintf(stderr, "the image is too small, use --size\n");
        exit(1);
    }
    return disk + (size_t)s * SS;
}

static void putWord(uint8_t* at, uint32_t w)
{
    at[0] = (uint8_t)w;
    at[1] = (uint8_t)(w >> 8);
    at[2] = (uint8_t)(w >> 16);
    at[3] = (uint8_t)(w >> 24);
}

/* the Oberon clock as used by Kernel.Clock and Files.Date */
static uint32_t oberonTime(time_t t)
{
    const struct tm* d = localtime(&t);
    return (uint32_t)((((((d->tm_year - 100) * 16 + d->tm_mon + 1) * 32 +
                       d->tm_mday) * 32 + d->tm_hour) * 64 + d->tm_min) * 64 +
                       d->tm_sec);
}

static const char* baseName(const char* path)
{
    const char* s = strrchr(path, '/');
    return s != NULL ? s + 1 : path;
}

static void addFile(const char* path)
{
    struct stat st;
    FILE* in = fopen(path, "rb");
    if( in == NULL )
    {
        fprintf(stderr, "cannot open %s\n", path);
        exit(1);
    }
    if( stat(path, &st) != 0 )
        st.st_mtime = time(NULL);

    const char* name = baseName(path);
    if( strlen(name) >= FnLength )
    {
        fprintf(stderr, "name too long: %s\n", name);
        exit(1);
    }
    if( nofEntries == DirPgSize )
    {
        fprintf(stderr, "more than %d files require a B-tree of more than one "
                        "page, which this tool cannot build\n", DirPgSize);
        exit(1);
    }

    const uint32_t head = nextSector++;
    uint8_t* hd = sector(head);
    uint32_t total = HS;        /* the file length includes the header */
    uint32_t page = 0;          /* index in the sector table */
    uint32_t n;
    int i;

    putWord(hd, HeaderMark);
    strcpy((char*)hd + 4, name);
    putWord(hd + 44, oberonTime(st.st_mtime));
    putWord(hd + 96, head * SectorMult); /* sec[0] is the header sector */

    /* the first page carries SS - HS bytes of data after the header */
    n = (uint32_t)fread(hd + HS, 1, SS - HS, in);
    total += n;
    while( (i = getc(in)) != EOF ) /* another page is needed */
    {
        uint32_t s;
        ungetc(i, in);
        if( ++page == STS )
        {
            fprintf(stderr, "%s is longer than %d bytes, which would require "
                            "extension sectors\n", name, STS * SS - HS);
            exit(1);
        }
        s = nextSector++;
        putWord(hd + 96 + 4 * page, s * SectorMult);
        n = (uint32_t)fread(sector(s), 1, SS, in);
        total += n;
    }
    fclose(in);

    /* aleng * SS + bleng = length, and bleng = SS if the last page is full */
    if( total % SS == 0 )
    {
        putWord(hd + 36, total / SS - 1);
        putWord(hd + 40, SS);
    }
    else
    {
        putWord(hd + 36, total / SS);
        putWord(hd + 40, total % SS);
    }

    strcpy(dir[nofEntries].name, name);
    dir[nofEntries].adr = head * SectorMult;
    nofEntries++;
}

static int compare(const void* a, const void* b)
{
    /* Oberon compares strings by unsigned character values */
    return strcmp(((const Entry*)a)->name, ((const Entry*)b)->name);
}

static void writeDir(void)
{
    uint8_t* a = sector(1);
    int i;

    qsort(dir, nofEntries, sizeof(Entry), compare);
    putWord(a, DirMark);
    putWord(a + 4, nofEntries); /* m, the number of entries */
    putWord(a + 8, 0);          /* p0, no descendant */
    for( i = 0; i < nofEntries; i++ )
    {
        uint8_t* e = a + 64 + 40 * i; /* mark, m, p0, fill[52] = 64 bytes */
        strcpy((char*)e, dir[i].name);
        putWord(e + 32, dir[i].adr);
        putWord(e + 36, 0);     /* p, no descendant */
    }
}

static void usage(const char* prog)
{
    fprintf(stderr,
            "usage: %s [options] <list> <image>\n"
            "  <list>          text file with one host path per line\n"
            "  --size <kb>     size of the image in KBytes (default %d)\n",
            prog, DEFAULT_SECTORS);
    exit(1);
}

int main(int argc, char** argv)
{
    const char* listFile = NULL;
    const char* imageFile = NULL;
    char line[512];
    FILE* list;
    FILE* out;
    int i;

    nofSectors = DEFAULT_SECTORS;
    for( i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i], "--size") == 0 && i + 1 < argc )
            nofSectors = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if( argv[i][0] == '-' )
            usage(argv[0]);
        else if( listFile == NULL )
            listFile = argv[i];
        else
            imageFile = argv[i];
    }
    if( listFile == NULL || imageFile == NULL || nofSectors < 4 )
        usage(argv[0]);

    disk = calloc(nofSectors, SS);
    if( disk == NULL )
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    list = fopen(listFile, "r");
    if( list == NULL )
    {
        fprintf(stderr, "cannot open %s\n", listFile);
        return 1;
    }
    while( fgets(line, sizeof(line), list) != NULL )
    {
        char* p = line + strlen(line);
        while( p > line && (p[-1] == '\n' || p[-1] == '\r' || p[-1] == ' ') )
            *--p = 0;
        if( line[0] != 0 && line[0] != '#' )
            addFile(line);
    }
    fclose(list);

    writeDir();

    out = fopen(imageFile, "wb");
    if( out == NULL )
    {
        fprintf(stderr, "cannot create %s\n", imageFile);
        return 1;
    }
    fwrite(disk, SS, nofSectors, out);
    fclose(out);
    free(disk);

    printf("%s: %d files, %d of %d sectors used\n",
           imageFile, nofEntries, nextSector - 1, nofSectors);
    return 0;
}
