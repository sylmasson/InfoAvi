/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

  InfoAvi utlitity

  Read and display information of all structures of an AVI file

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

  The MIT License (MIT)

  Copyright (c) 2024 Sylvain Masson

  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the “Software”), to deal in 
  the Software without restriction, including without limitation the rights to 
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
  of the Software, and to permit persons to whom the Software is furnished to 
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all 
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
  OTHER DEALINGS IN THE SOFTWARE.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

  Version history

  1.0.1   2024-12-31  Cleaning and minor change

  1.0.0   2024-02-13  First official version

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <ctype.h>
#include    <string.h>
#include    <stdbool.h>

#define     AVIF_HASINDEX           0x00000010
#define     AVIF_MUSTUSEINDEX       0x00000020
#define     AVIF_ISINTERLEAVED      0x00000100
#define     AVIF_TRUSTCKTYPE        0x00000800
#define     AVIF_WASCAPTUREFILE     0x00010000
#define     AVIF_COPYRIGHTED        0x00020000

#define     AVIMAINHEADER_SIZE      56
#define     AVISTREAMHEADER_SIZE    56
#define     BITMAPINFOHEADER_SIZE   40
#define     WAVEFORMATEX_SIZE       18
#define     WAVEFORMAT_SIZE         14
#define     AVIOLDINDEX_SIZE        16

#define     WAVE_FORMAT_UNKNOWN     0
#define     WAVE_FORMAT_PCM         1
#define     WAVE_FORMAT_MULAW       2
#define     WAVE_FORMAT_IMA_ADPCM   3

#define     LIMIT_INFO_DEFAULT      20

#define     FOURCC(str)             (*(uint32_t *)str)

#define     printFmt                printf

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct
{
  uint32_t  dwMicroSecPerFrame;
  uint32_t  dwMaxBytesPerSec;
  uint32_t  dwPaddingGranularity;
  uint32_t  dwFlags;
  uint32_t  dwTotalFrames;
  uint32_t  dwInitialFrames;
  uint32_t  dwStreams;
  uint32_t  dwSuggestedBufferSize;
  uint32_t  dwWidth;
  uint32_t  dwHeight;
  uint32_t  dwReserved[4];
} AVIMAINHEADER;  // 'avih'

typedef struct
{
  uint32_t  fccType;
  uint32_t  fccHandler;
  uint32_t  dwFlags;
  uint16_t  wPriority;
  uint16_t  wLanguage;
  uint32_t  dwInitialFrames;
  uint32_t  dwScale;
  uint32_t  dwRate;
  uint32_t  dwStart;
  uint32_t  dwLength;
  uint32_t  dwSuggestedBufferSize;
  uint32_t  dwQuality;
  uint32_t  dwSampleSize;
  struct
  {
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
  } rcFrame;
} AVISTREAMHEADER;  // 'strh'

typedef struct
{
  uint32_t  biSize;
  int32_t   biWidth;
  int32_t   biHeight;
  uint16_t  biPlanes;
  uint16_t  biBitCount;
  uint32_t  biCompression;
  uint32_t  biSizeImage;
  int32_t   biXPelsPerMeter;
  int32_t   biYPelsPerMeter;
  uint32_t  biClrUsed;
  uint32_t  biClrImportant;
} BITMAPINFOHEADER; // 'strf' for video

typedef struct
{
  uint16_t  wFormatTag;
  uint16_t  nChannels;
  uint32_t  nSamplesPerSec;
  uint32_t  nAvgBytesPerSec;
  uint16_t  nBlockAlign;
  uint16_t  wBitsPerSample;
  uint16_t  cbSize;
} WAVEFORMATEX;     // 'strf' for audio

typedef struct
{
  uint32_t  dwChunkId;
  uint32_t  dwFlags;
  uint32_t  dwOffset;
  uint32_t  dwSize;
} AVIOLDINDEX;      // 'idx1'

typedef struct
{
  uint16_t  Width;
  uint16_t  Height;
  uint8_t   Stream;
  uint8_t   AudioFmt;
  uint8_t   AudioChan;
  uint8_t   SampleDepth;
  uint32_t  SampleRate;
  uint32_t  SampleCount;
  uint32_t  AudioBufSize;
  uint32_t  VideoBufSize;
  float     VideoLength;
  uint32_t  FrameRate;
  uint32_t  FrameCount;
  uint32_t  MoviBegin;
  uint32_t  MoviEnd;
  uint32_t  Idx1Begin;
  uint32_t  Idx1End;
  char      VideoFcc[5];
  char      AudioFcc[5];
} AVIINFO;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

FILE        *File;
uint32_t    FileInd;
uint32_t    FileSize;
char        *FileName;
AVIINFO     AviInfo;
uint32_t    LimitInfo = LIMIT_INFO_DEFAULT;

static const char   Version[] = "1.0.1 (2024-12-31)";
static const char   *sWaveFormat[4] = {"Unknown format", "PCM signed integer format", "Mu-law encoded format", "IMA ADPCM format"};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool        infoAvi(void);
bool        readInfo(uint32_t BlkEnd, uint16_t TabCnt, uint32_t Limit);
bool        readFile(void *Data, uint32_t Pos, uint32_t Count);
void        printAddr(uint32_t Addr);
void        printSize(uint32_t Size);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int         main(int argc, char *argv[])
{
  char      c;
  int       argi, argf;
  bool      argerr = false, usage = false;

  for (argi = 1, argf = 0; argi < argc; argi++)
  {
    if (argv[argi][0] != '-')
      argf++;
    else
    {
      if ((c = argv[argi][1]) == 0)
        argerr = true;
      else if (argv[argi][2] == 0)
        switch (c)
        {
          case 'c': LimitInfo = (uint32_t)-1; break;
          case 'h': usage = true; break;
          default: argerr = true; break;
        }
      else
        switch (c)
        {
          case 'c':
            if (sscanf(&argv[argi][2], "%d%c", &LimitInfo, &c) == 1)
              break;

          default:
            argerr = true;
            break;
        }

      if (argerr || usage)
        break;

      argv[argi][0] = 0;
    }
  }

  if (argerr)
  {
    printFmt("Invalid option '%s'\n", argv[argi]);
    return EXIT_FAILURE;
  }

  if (!argf)
  {
    printFmt("No input files\n");
    usage = true;
  }

  if (usage)
  {
    printFmt("\nUsage: infoavi [option1] [option2] [...] filename1 filename2 ...\n\n");
    printFmt("  -c[xx]  Limit video chunk info. Default is %u. No limit if -c only.\n", LIMIT_INFO_DEFAULT);
    printFmt("  -h      Display help.\n\n");
    printFmt("Version: %s\n\n", Version);
    return EXIT_SUCCESS;
  }

  for (argi = 1; argi < argc; argi++)
  {
    if (argv[argi][0])
    {
      FileName = argv[argi];

      if ((File = fopen(FileName, "rb")) == NULL)
      {
        printFmt("\nFile \"%s\" not exist\n", FileName);
        return EXIT_FAILURE;
      }

      if (infoAvi() == false)
      {
        printFmt(!ferror(File) ? "\n\"%s\" is not valid AVI file\n" : "Unable to read file \"%s\"\n", FileName);
        fclose(File);
        return EXIT_FAILURE;
      }

      fclose(File);
    }
  }

  return EXIT_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool        infoAvi(void)
{
  struct
  {
    uint32_t  fcc;
    uint32_t  cb;
    uint32_t  type;
  } hdr;

  memset(&AviInfo, 0, sizeof(AviInfo));

  if (fseek(File, 0, SEEK_END) != 0 || (FileSize = ftell(File)) == -1)
    return false;

  if (!readFile(&hdr, FileInd = 0, sizeof(hdr)))
    return false;

  if (hdr.fcc != FOURCC("RIFF") || hdr.cb != (FileSize - 8) || hdr.type != FOURCC("AVI "))
    return false;

  printFmt("\nReading file \"%s\"\n", FileName);
  printAddr(FileInd);
  printFmt("'RIFF'('AVI '");
  printSize(hdr.cb);
  FileInd = 12;
  
  if (readInfo(FileSize, 1, LimitInfo) == false)
    return false;

  printAddr(FileInd);
  printFmt("End of file \"%s\"\n", FileName);
  printFmt("\nAviInfo \"%s\"\n", FileName);
  printFmt("{\n");
  printFmt("  Width = %u\n", AviInfo.Width);
  printFmt("  Height = %u\n", AviInfo.Height);
  printFmt("  Stream = %u\n", AviInfo.Stream);
  printFmt("  AudioFmt = %u (%s)\n", AviInfo.AudioFmt, sWaveFormat[AviInfo.AudioFmt]);
  printFmt("  AudioChan = %u\n", AviInfo.AudioChan);
  printFmt("  SampleDepth = %u bits\n", AviInfo.SampleDepth);
  printFmt("  SampleRate  = %u Hz\n", AviInfo.SampleRate);
  printFmt("  SampleCount = %u\n", AviInfo.SampleCount);
  printFmt("  AudioBufSize = %u bytes\n", AviInfo.AudioBufSize);
  printFmt("  VideoBufSize = %u bytes\n", AviInfo.VideoBufSize);
  printFmt("  VideoLength  = %.2f sec\n", AviInfo.VideoLength);
  printFmt("  FrameRate  = %.3f Hz\n", (float)AviInfo.FrameRate / 1000.0);
  printFmt("  FrameCount = %u\n", AviInfo.FrameCount);
  printFmt("  MoviBegin = 0x%08X\n", AviInfo.MoviBegin);
  printFmt("  MoviEnd   = 0x%08X\n", AviInfo.MoviEnd);
  printFmt("  Idx1Begin = 0x%08X\n", AviInfo.Idx1Begin);
  printFmt("  Idx1End   = 0x%08X\n", AviInfo.Idx1End);
  printFmt("  VideoFcc = '%s'\n", AviInfo.VideoFcc);
  printFmt("  AudioFcc = '%s'\n", AviInfo.AudioFcc);
  printFmt("}\n");
  return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool        readInfo(uint32_t BlkEnd, uint16_t TabCnt, uint32_t Limit)
{
  char        str[] = "????";
  char        tab[] = "\t\t\t\t\t\t\t\t\t\t";
  const char  begin[] = " (", end[] = "%s\t       )\n";
  uint32_t    size, limit = Limit;
  struct
  {
    uint32_t  fcc;
    uint32_t  cb;
  } chunk;

  if (BlkEnd > FileSize)
    return false;

  if (TabCnt < sizeof(tab))
    tab[TabCnt] = 0;

  for (; FileInd < BlkEnd; FileInd += size)
  {
    if (!readFile(&chunk, FileInd, 8))
      return false;

    FileInd += 8;
    size = chunk.cb;

    FOURCC(str) = chunk.fcc;
    printAddr(FileInd - 8);
    printFmt("%s'%s'", tab, str);

    if (chunk.fcc == FOURCC("LIST"))
    {
      if (!readFile(str, FileInd, 4))
        return false;

      printFmt(begin);
      printFmt("'%s'", str);
      printSize(size);
      FileInd += 4;

      if (FOURCC(str) == FOURCC("movi"))
      {
        AviInfo.MoviBegin = FileInd;
        AviInfo.MoviEnd = FileInd + size - 4;
      }

      if (readInfo(FileInd + size - 4, TabCnt + 1, (FOURCC(str) == FOURCC("movi")) ? Limit : (uint32_t)-1) == false)
        return false;

      printFmt(end, tab);
      size = 0;
    }
    else if (chunk.fcc == FOURCC("avih") && size == AVIMAINHEADER_SIZE)
    {
      AVIMAINHEADER   avih;

      if (!readFile(&avih, FileInd, AVIMAINHEADER_SIZE))
        return false;

      AviInfo.Width = avih.dwWidth;
      AviInfo.Height = avih.dwHeight;
      AviInfo.FrameCount = avih.dwTotalFrames;

      printFmt(begin);
      printSize(size);
      printFmt("%s\t\tdwMicroSecPerFrame = %u\n", tab, avih.dwMicroSecPerFrame);
      printFmt("%s\t\tdwMaxBytesPerSec = %u\n", tab, avih.dwMaxBytesPerSec);
      printFmt("%s\t\tdwPaddingGranularity = %u\n", tab, avih.dwPaddingGranularity);
      printFmt("%s\t\tdwFlags = 0x%x%s", tab, avih.dwFlags, avih.dwFlags & AVIF_HASINDEX ? " (has index)\n" : "/n");
      printFmt("%s\t\tdwTotalFrames = %u\n", tab, avih.dwTotalFrames);
      printFmt("%s\t\tdwInitialFrames = %u\n", tab, avih.dwInitialFrames);
      printFmt("%s\t\tdwStreams = %u\n", tab, avih.dwStreams);
      printFmt("%s\t\tdwSuggestedBufferSize = %u\n", tab, avih.dwSuggestedBufferSize);
      printFmt("%s\t\tdwWidth = %u\n", tab, avih.dwWidth);
      printFmt("%s\t\tdwHeight = %u\n", tab, avih.dwHeight);
      printFmt(end, tab);
    }
    else if (chunk.fcc == FOURCC("strh") && size == AVISTREAMHEADER_SIZE)
    {
      AVISTREAMHEADER   strh;

      if (!readFile(&strh, FileInd, AVISTREAMHEADER_SIZE))
        return false;

      if (strh.fccType == FOURCC("auds"))
      {
        AviInfo.AudioBufSize = strh.dwSuggestedBufferSize;
        AviInfo.SampleRate = strh.dwRate / strh.dwScale;
        AviInfo.SampleCount = strh.dwLength;

        if (!FOURCC(AviInfo.AudioFcc))
          snprintf(AviInfo.AudioFcc, sizeof(AviInfo.AudioFcc), "%02uwb", AviInfo.Stream & 0x0F);
      }
      else if (strh.fccType == FOURCC("vids") && strh.fccHandler == FOURCC("MJPG"))
      {
        AviInfo.VideoBufSize = strh.dwSuggestedBufferSize;
        AviInfo.FrameRate = (strh.dwScale == 0) ? 0 : ((float)strh.dwRate * 1000.0) / (float)strh.dwScale;
        AviInfo.VideoLength = (AviInfo.FrameRate == 0) ? 0 : ((float)AviInfo.FrameCount * 1000.0) / AviInfo.FrameRate;

        if (!FOURCC(AviInfo.VideoFcc))
          snprintf(AviInfo.VideoFcc, sizeof(AviInfo.VideoFcc), "%02udc", AviInfo.Stream & 0x0F);
      }

      AviInfo.Stream++;
      printFmt(begin);
      printSize(size);
      FOURCC(str) = strh.fccType;
      printFmt("%s\t\tfccType = \"%s\"\n", tab, str);
      FOURCC(str) = strh.fccHandler;
      printFmt("%s\t\tfccHandler = \"%s\"\n", tab, str);
      printFmt("%s\t\tdwFlags = 0x%x\n", tab, strh.dwFlags);
      printFmt("%s\t\twPriority = %u\n", tab, strh.wPriority);
      printFmt("%s\t\twLanguage = %u\n", tab, strh.wLanguage);
      printFmt("%s\t\tdwInitialFrames = %u\n", tab, strh.dwInitialFrames);
      printFmt("%s\t\tdwScale = %u\n", tab, strh.dwScale);
      printFmt("%s\t\tdwRate = %u\n", tab, strh.dwRate);
      printFmt("%s\t\tdwStart = %u\n", tab, strh.dwStart);
      printFmt("%s\t\tdwLength = %u\n", tab, strh.dwLength);
      printFmt("%s\t\tdwSuggestedBufferSize = %u\n", tab, strh.dwSuggestedBufferSize);
      printFmt("%s\t\tdwQuality = %d\n", tab, strh.dwQuality);
      printFmt("%s\t\tdwSampleSize = %u\n", tab, strh.dwSampleSize);
      printFmt("%s\t\trcFrame.left = %d\n", tab, strh.rcFrame.left);
      printFmt("%s\t\trcFrame.top = %d\n", tab, strh.rcFrame.top);
      printFmt("%s\t\trcFrame.right = %d\n", tab, strh.rcFrame.right);
      printFmt("%s\t\trcFrame.bottom = %d\n", tab, strh.rcFrame.bottom);
      printFmt(end, tab);
    }
    else if (chunk.fcc == FOURCC("strf"))
    {
      if (size == BITMAPINFOHEADER_SIZE)
      {
        BITMAPINFOHEADER  strf;

        if (!readFile(&strf, FileInd, BITMAPINFOHEADER_SIZE))
          return false;

        printFmt(begin);
        printSize(size);
        printFmt("%s\t\tbiSize = %u\n", tab, strf.biSize);
        printFmt("%s\t\tbiWidth = %d\n", tab, strf.biWidth);
        printFmt("%s\t\tbiHeight = %d\n", tab, strf.biHeight);
        printFmt("%s\t\tbiPlanes = %u\n", tab, strf.biPlanes);
        printFmt("%s\t\tbiBitCount = %u\n", tab, strf.biBitCount);
        printFmt("%s\t\tbiCompression = %u\n", tab, strf.biCompression);
        printFmt("%s\t\tbiSizeImage = %u\n", tab, strf.biSizeImage);
        printFmt("%s\t\tbiXPelsPerMeter = %d\n", tab, strf.biXPelsPerMeter);
        printFmt("%s\t\tbiYPelsPerMeter = %d\n", tab, strf.biYPelsPerMeter);
        printFmt("%s\t\tbiClrUsed = %d\n", tab, strf.biClrUsed);
        printFmt("%s\t\tbiClrImportant = %d\n", tab, strf.biClrImportant);
        printFmt(end, tab);
      }
      else
      {
        WAVEFORMATEX  strf;
        int16_t       strfSize;

        strf.cbSize = 0;
        strfSize = (size > WAVEFORMATEX_SIZE) ? WAVEFORMATEX_SIZE : size;

        if (!readFile(&strf, FileInd, strfSize))
          return false;

        if (strfSize == WAVEFORMAT_SIZE)
          strf.wBitsPerSample = 8 * strf.nAvgBytesPerSec / strf.nSamplesPerSec / strf.nChannels;

        switch (strf.wFormatTag)
        {
          case 0x01: AviInfo.AudioFmt = WAVE_FORMAT_PCM; break;
          case 0x07: AviInfo.AudioFmt = WAVE_FORMAT_MULAW; break;
          case 0x11: AviInfo.AudioFmt = WAVE_FORMAT_IMA_ADPCM; break;
          default: AviInfo.AudioFmt = WAVE_FORMAT_UNKNOWN;
        }

        AviInfo.AudioChan = strf.nChannels;
        AviInfo.SampleDepth = strf.wBitsPerSample;

        printFmt(begin);
        printSize(size);
        printFmt("%s\t\twFormatTag = %u (%s)\n", tab, strf.wFormatTag, sWaveFormat[AviInfo.AudioFmt]);
        printFmt("%s\t\tnChannels = %u\n", tab, strf.nChannels);
        printFmt("%s\t\tnSamplesPerSec = %u\n", tab, strf.nSamplesPerSec);
        printFmt("%s\t\tnAvgBytesPerSec = %u\n", tab, strf.nAvgBytesPerSec);
        printFmt("%s\t\tnBlockAlign = %u\n", tab, strf.nBlockAlign);
        printFmt("%s\t\twBitsPerSample = %u\n", tab, strf.wBitsPerSample);
        printFmt(end, tab);
      }
    }
    else if (chunk.fcc == FOURCC("idx1"))
    {
      AVIOLDINDEX   idx;

      if ((size & 3) != 0)
        return false;

      AviInfo.Idx1Begin = FileInd;
      AviInfo.Idx1End = FileInd + size;
      printFmt(begin);
      printSize(size);

      if (Limit < (limit = size / 4))
        limit = Limit;

      for (uint32_t frame = 0; limit > 0 && FileInd < BlkEnd; FileInd += AVIOLDINDEX_SIZE, limit--)
      {
        if (!readFile(&idx, FileInd, AVIOLDINDEX_SIZE))
          return false;

        printAddr(FileInd);
        FOURCC(str) = idx.dwChunkId;
        printFmt("%s\t'%s' +%08X%7u", tab, str, idx.dwOffset, idx.dwSize);

        if (idx.dwChunkId == FOURCC(AviInfo.VideoFcc))
          printFmt("  #%u\n", ++frame);
        else
          printFmt("\n");
      }

      if (limit == 0)
      {
        printAddr(FileInd);
        printFmt("%s\t ....", tab);
        printSize(BlkEnd - FileInd);
      }

      printFmt(end, tab);
      FileInd = BlkEnd;
      size = 0;
    }
    else if (chunk.fcc == FOURCC("ISFT"))
    {
      char    package[128];

      if (size > sizeof(package) - 1)
        size = sizeof(package) - 1;

      if (!readFile(package, FileInd, size))
        return false;

      package[size + 1] = 0;
      printFmt(" %s\n", package);
      FileInd = BlkEnd;
      size = 0;
    }
    else
    {
      printSize(size);

      if (!--limit)
      {
        printAddr(FileInd);
        printFmt("%s ....", tab);
        printSize(BlkEnd - FileInd);
        FileInd = BlkEnd;
        size = 0;
      }
    }

    if (size & 1)
      size++;
  }

  return true;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool        readFile(void *Data, uint32_t Pos, uint32_t Count)
{
  return (fseek(File, Pos, SEEK_SET) == 0 && fread(Data, 1, Count, File) == Count);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void        printAddr(uint32_t Addr)
{
  printFmt("%08X ", Addr);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void        printSize(uint32_t Size)
{
  printFmt(" %u\n", Size);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

