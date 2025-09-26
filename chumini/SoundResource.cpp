#include "SoundResource.h"

//ユーティリティ関数群
HRESULT sf::sound::FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;
	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());
		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());
		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}
		dwOffset += sizeof(DWORD) * 2;
		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}
		dwOffset += dwChunkDataSize;
		if (bytesRead >= dwRIFFDataSize) return S_FALSE;
	}
	return S_OK;
}

HRESULT sf::sound::ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}

IXAudio2* sf::sound::SoundResource::pXAudio2 = nullptr;
IXAudio2MasteringVoice* sf::sound::SoundResource::pMasterVoice = nullptr;

sf::sound::SoundResource::~SoundResource()
{
	delete[] DataBuffer;
	DataBuffer = nullptr;
}

void sf::sound::SoundResource::Init()
{
	XAudio2Create(&pXAudio2, 0);
	pXAudio2->CreateMasteringVoice(&pMasterVoice);
}

void sf::sound::SoundResource::UnInit()
{
	pMasterVoice->DestroyVoice();
	pMasterVoice = nullptr;

	pXAudio2->Release();
	pXAudio2 = nullptr;
}

HRESULT sf::sound::SoundResource::LoadSound(std::string path, bool loop)
{
	HANDLE hFile;
	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	DWORD filetype;

	memset(&wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
	memset(&buffer, 0, sizeof(XAUDIO2_BUFFER));

	hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)		return S_FALSE;

	FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
	DataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(hFile, DataBuffer, dwChunkSize, dwChunkPosition);

	CloseHandle(hFile);

	// 	サブミットボイスで利用するサブミットバッファの設定
	buffer.AudioBytes = dwChunkSize;
	buffer.pAudioData = DataBuffer;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	if (loop)
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	else
		buffer.LoopCount = 0;

	pXAudio2->CreateSourceVoice(&pSourceVoice, &(wfx.Format));

	return S_OK;
}
