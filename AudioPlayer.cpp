// AudioPlayer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "wac.h"
#include "wavplay.h"
#define MUTE_LENGTH 128
/*起始地址 占用空间        本地址数字的含义

   00H      4byte          RIFF，资源交换文件标志。

   04H      4byte          从下一个地址开始到文件尾的总字节数。高位字节在后面，这里就是001437ECH，换成十进制是1325036byte，算上这之前的8byte就正好1325044byte了。
  
   08H      4byte          WAVE，代表wav文件格式。
   
   0CH      4byte          FMT ，波形格式标志
   
   10H      4byte          这个fmt的大小

   14H      2byte          为1时表示线性PCM编码，大于1时表示有压缩的编码。这里是0001H。

   16H      2byte          1为单声道，2为双声道，这里是0001H。

   18H      4byte          采样频率，这里是00002B11H，也就是11025Hz。
   
   1CH      4byte          Byte率=采样频率*音频通道数*每次采样得到的样本位数/8，00005622H，也就是22050Byte/s=11025*1*16/2。

   20H      2byte          块对齐=通道数*每次采样得到的样本位数/8，0002H，也就是2=1*16/8。

   22H      2byte          样本数据位数，0010H即16，一个量化样本占2byte。

   24H      4byte          data，一个标志而已。

   28H      4byte          Wav文件实际音频数据所占的大小，这里是001437C8H即1325000，再加上2CH就正好是1325044，整个文件的大小。

   2CH      不定          量化数据。
*/
long filesize( FILE*stream )
{
    long curpos, length;
    curpos = ftell( stream );
    fseek( stream, 0L, SEEK_END );
    length = ftell( stream );
    fseek( stream, curpos, SEEK_SET );
    return length;
}
int ParseWaveFormat( FILE* file,WAVEFORMATEX *pwfx )
{
    char data[ 4 ];
    int size;
    fread( data, 1, 4, file );//riff
    fread( &size, 4, 1, file );//size+8=filesize
    fread( data, 1, 4, file );//wave
    fread( data, 1, 4, file );//fmt 

    fread( &size, 1, 4, file );
    fread( &pwfx->wFormatTag, 1, 2, file );
    fread( &pwfx->nChannels, 2, 1, file );
    fread( &pwfx->nSamplesPerSec,4,1,file );
    fread( &pwfx->nAvgBytesPerSec, 4, 1, file );
    fread( &pwfx->nBlockAlign, 2, 1, file );
    fread( &pwfx->wBitsPerSample, 1, 2, file );
    if (size-16>0)
    {
        fread( &pwfx->cbSize, 1, 2, file );// 填充信息
    }

    fread( data, 1, 4, file );
    fread( &size, 4, 1, file );
    return size;
}
#define BLOCK_SIZE 1024

int main()
{
    ::CoInitialize( NULL );
    FILE *file = fopen("E:\\webrtc\\trunk\\data\\voice_engine\\audio_tiny48.wav","rb");
    if (!file)
    {
       
        return 0;
    }

     //int size = filesize(file);
     fseek( file, 0, SEEK_SET );
    WAVEFORMATEX _wfx;
   int size = ParseWaveFormat( file, &_wfx );
    char* buf = new char[ size ];
    fread( buf, 1, size, file );
    MyAudioSource*pAudio = new MyAudioSource;
    pAudio->PlayAudioStream( &_wfx );
    /*
    CViWavePlay* m_pWavPlay;
    m_pWavPlay = new CViWavePlay;
    m_pWavPlay->Start( &_wfx );
    for ( int i = 0; i < size; i += BLOCK_SIZE )
    {
        if ( !m_pWavPlay->PlayAudio( (char*)buf + i, BLOCK_SIZE ) )
        {
            continue;
        }
    }
    m_pWavPlay->WaitForPlayComplete();
    m_pWavPlay->Stop();
    delete m_pWavPlay;
    */
    ::CoUninitialize();
    return 0;
}




#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
/*
* some good values for block size and count
*/
//#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20
/*
* function prototypes
*/
static void CALLBACK waveOutProc( HWAVEOUT, UINT, DWORD, DWORD, DWORD );
static WAVEHDR* allocateBlocks( int size, int count );
static void freeBlocks( WAVEHDR* blockArray );
static void writeAudio( HWAVEOUT hWaveOut, LPSTR data, int size );
/*
* module level variables
*/
static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR* waveBlocks;
static volatile int waveFreeBlockCount;
static int waveCurrentBlock;
int main1( int argc, char* argv[] )
{
    FILE *file = fopen( "E:\\webrtc\\trunk\\data\\voice_engine\\audio_tiny48.wav", "rb" );
    if ( !file )
    {

        return 0;
    }
    //int size = filesize(file);
    fseek( file, 0, SEEK_SET );
    WAVEFORMATEX _wfx;
    int size = ParseWaveFormat( file, &_wfx );
//     char* buf = new char[ size ];
//     fread( buf, 1, size, file );


    HWAVEOUT hWaveOut; /* device handle */
    char buffer[ 1024 ]; /* intermediate buffer for reading */
    int i;
    
    /*
    * initialise the module variables
    */
    waveBlocks = allocateBlocks( BLOCK_SIZE, BLOCK_COUNT );
    waveFreeBlockCount = BLOCK_COUNT;
    waveCurrentBlock = 0;
    InitializeCriticalSection( &waveCriticalSection );
 
    if ( waveOutOpen(

        &hWaveOut,
        WAVE_MAPPER,
        &_wfx,
        (DWORD_PTR)waveOutProc,
        (DWORD_PTR)&waveFreeBlockCount,
        CALLBACK_FUNCTION
        ) != MMSYSERR_NOERROR )

    {

        fprintf( stderr, "%s: unable toopen wave mapper device\n", argv[ 0 ] );
        ExitProcess( 1 );

    }
    /*
    * playback loop
    */
    while ( size > 0 )
    {

        size_t readBytes;
        if ( (readBytes = fread( buffer, 1, sizeof( buffer ), file )) == 0 )

            break;
        size -= sizeof( buffer );

        if ( readBytes < sizeof( buffer ) )
        {

            printf( "at end of buffer\n" );
            memset( buffer + readBytes, 0, sizeof( buffer ) - readBytes );
            printf( "after memcpy\n" );

        }
        writeAudio( hWaveOut, buffer, sizeof( buffer ) );

    }
    /*
    * wait for all blocks to complete
    */
    while ( waveFreeBlockCount < BLOCK_COUNT )

        Sleep( 10 );

    /*
    * unprepare any blocks that are still prepared
    */
    for ( i = 0; i < waveFreeBlockCount; i++ )

        if ( waveBlocks[ i ].dwFlags &WHDR_PREPARED )

            waveOutUnprepareHeader( hWaveOut, &waveBlocks[ i ], sizeof( WAVEHDR ) );

    DeleteCriticalSection( &waveCriticalSection );
    freeBlocks( waveBlocks );
    waveOutClose( hWaveOut );

    return 0;

}

WAVEHDR* allocateBlocks( int size, int count )
{

    unsigned char* buffer;
    int i;
    WAVEHDR* blocks;
    DWORD totalBufferSize = ( size + sizeof( WAVEHDR ) ) * count;
    /*
    * allocate memory for the entire set in one go
    */
    if ( ( buffer = (unsigned char*)HeapAlloc(

        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        totalBufferSize
        ) ) == NULL )

    {

        fprintf( stderr, "Memory allocationerror\n" );
        ExitProcess( 1 );

    }
    /*
    * and set up the pointers to each bit
    */
    blocks = (WAVEHDR*)buffer;
    buffer += sizeof( WAVEHDR ) * count;
    for ( i = 0; i < count; i++ )
    {

        blocks[ i ].dwBufferLength = size;
        blocks[ i ].lpData =(LPSTR) buffer;
        buffer += size;

    }
    return blocks;

}
void freeBlocks( WAVEHDR* blockArray )
{

    /*
    * and this is why allocateBlocks works the way it does
    */
    HeapFree( GetProcessHeap(), 0, blockArray );

}

static void CALLBACK waveOutProc(

    HWAVEOUT hWaveOut,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )

{

    /*
    * pointer to free block counter
    */
    int* freeBlockCounter = (int*)dwInstance;
    /*
    * ignore calls that occur due to openining and closing the
    * device.
    */
    if ( uMsg != WOM_DONE )

        return;

    EnterCriticalSection( &waveCriticalSection );
    ( *freeBlockCounter )++;
    LeaveCriticalSection( &waveCriticalSection );

}

void writeAudio( HWAVEOUT hWaveOut, LPSTR data, int size )
{

    WAVEHDR* current;
    int remain;
    current = &waveBlocks[ waveCurrentBlock ];
    while ( size > 0 )
    {

        /*
        * first make sure the header we're going to use is unprepared
        */
        if ( current->dwFlags & WHDR_PREPARED )

            waveOutUnprepareHeader( hWaveOut, current, sizeof( WAVEHDR ) );

        if ( size < (int)( BLOCK_SIZE - current->dwUser ) )
        {

            memcpy( current->lpData + current->dwUser, data, size );
            current->dwUser += size;
            break;

        }
        remain = BLOCK_SIZE - current->dwUser;
        memcpy( current->lpData + current->dwUser, data, remain );
        size -= remain;
        data += remain;
        current->dwBufferLength = BLOCK_SIZE;
        waveOutPrepareHeader( hWaveOut, current, sizeof( WAVEHDR ) );
        waveOutWrite( hWaveOut, current, sizeof( WAVEHDR ) );
        EnterCriticalSection( &waveCriticalSection );
        waveFreeBlockCount--;
        LeaveCriticalSection( &waveCriticalSection );
        /*
        * wait for a block to become free
        */
        while ( !waveFreeBlockCount )

            Sleep( 0 );

        /*
        * point to the next block
        */
        waveCurrentBlock++;
        waveCurrentBlock %= BLOCK_COUNT;
        current = &waveBlocks[ waveCurrentBlock ];
        current->dwUser = 0;

    }

}