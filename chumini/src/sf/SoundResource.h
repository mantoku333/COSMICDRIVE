#pragma once
#include "Ref.h"
#include <xaudio2.h>
#include <Windows.h>
#include <x3daudio.h>
#include <string>
#include <map>

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif
#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

namespace sf
{
	namespace sound
	{
		//1つの音データクラス
		class SoundResource :public sf::ref::autoRef<SoundResource>
		{
		public:
			~SoundResource();

			static void Init();
			static void UnInit();

			HRESULT LoadSound(std::string path, bool loop = false);

			WAVEFORMATEXTENSIBLE GetWAVEFORMATEXTENSIBLE()const { return wfx; }
			XAUDIO2_BUFFER GetXAUDIO2_BUFFER()const { return buffer; }

			static IXAudio2* GetIXAudio2() { return pXAudio2; }
			static IXAudio2MasteringVoice* GetIXAudio2MasteringVoice() { return pMasterVoice; }

		private:
			WAVEFORMATEXTENSIBLE wfx;			// WAVフォーマット
			XAUDIO2_BUFFER buffer;
			BYTE* DataBuffer = nullptr;
			IXAudio2SourceVoice* pSourceVoice = nullptr;

			static IXAudio2* pXAudio2;
			static IXAudio2MasteringVoice* pMasterVoice;
		};

		HRESULT FindChunk(HANDLE, DWORD, DWORD&, DWORD&);
		HRESULT ReadChunkData(HANDLE, void*, DWORD, DWORD);

	}
}
