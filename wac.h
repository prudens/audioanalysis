#pragma once
class MyAudioSource
{
public:
    HRESULT PlayAudioStream( WAVEFORMATEX *Mypwfx );
    HRESULT SetFormat( WAVEFORMATEX *pwfx );
    HRESULT LoadData( UINT, BYTE*pData, DWORD*dwFlag );
};