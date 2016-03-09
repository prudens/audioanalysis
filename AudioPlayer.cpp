// AudioPlayer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "wac.h"
#include "wavplay.h"
#define MUTE_LENGTH 128
/*��ʼ��ַ ռ�ÿռ�        ����ַ���ֵĺ���

   00H      4byte          RIFF����Դ�����ļ���־��

   04H      4byte          ����һ����ַ��ʼ���ļ�β�����ֽ�������λ�ֽ��ں��棬�������001437ECH������ʮ������1325036byte��������֮ǰ��8byte������1325044byte�ˡ�
  
   08H      4byte          WAVE������wav�ļ���ʽ��
   
   0CH      4byte          FMT �����θ�ʽ��־
   
   10H      4byte          ���fmt�Ĵ�С

   14H      2byte          Ϊ1ʱ��ʾ����PCM���룬����1ʱ��ʾ��ѹ���ı��롣������0001H��

   16H      2byte          1Ϊ��������2Ϊ˫������������0001H��

   18H      4byte          ����Ƶ�ʣ�������00002B11H��Ҳ����11025Hz��
   
   1CH      4byte          Byte��=����Ƶ��*��Ƶͨ����*ÿ�β����õ�������λ��/8��00005622H��Ҳ����22050Byte/s=11025*1*16/2��

   20H      2byte          �����=ͨ����*ÿ�β����õ�������λ��/8��0002H��Ҳ����2=1*16/8��

   22H      2byte          ��������λ����0010H��16��һ����������ռ2byte��

   24H      4byte          data��һ����־���ѡ�

   28H      4byte          Wav�ļ�ʵ����Ƶ������ռ�Ĵ�С��������001437C8H��1325000���ټ���2CH��������1325044�������ļ��Ĵ�С��

   2CH      ����          �������ݡ�
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
        fread( &pwfx->cbSize, 1, 2, file );// �����Ϣ
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