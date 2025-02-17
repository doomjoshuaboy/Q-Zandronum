/*
** i_sound.cpp
** Stubs for sound interfaces.
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "resource.h"
extern HWND Window;
extern HINSTANCE g_hInst;
#define USE_WINDOWS_DWORD
#else
#define FALSE 0
#define TRUE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// [RC] We need this include this before networkshared.h (and the time.h reference there).
#include <time.h>

// [BB] This fixes a weird include problem under Linux and FreeBSD.
#ifndef _WIN32
#include "networkheaders.h"
#endif

#include "doomtype.h"
#include <math.h>

#include "except.h"
#include "fmodsound.h"
#include "oalsound.h"

#include "mpg123_decoder.h"
#include "sndfile_decoder.h"

#include "m_swap.h"
#include "stats.h"
#include "files.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_music.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "i_video.h"
#include "s_sound.h"
#include "v_text.h"
#include "gi.h"

#include "doomdef.h"

EXTERN_CVAR (Float, snd_sfxvolume)
CVAR (Int, snd_samplerate, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (Int, snd_buffersize, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (String, snd_output, "default", CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (Bool, snd_hrtf, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

#ifndef NO_OPENAL
#define DEF_BACKEND "openal"
#elif !defined(NO_FMOD)
#define DEF_BACKEND "fmod"
#else
#define DEF_BACKEND "null"
#endif

void I_CloseSound();

CUSTOM_CVAR(String, snd_backend, DEF_BACKEND, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	I_ShutdownMusic();
	S_EvictAllChannels();
	I_CloseSound();
	I_InitSound();
	S_RestartMusic();
	S_RestoreEvictedChannels();
}

// killough 2/21/98: optionally use varying pitched sounds
CVAR (Bool, snd_pitched, false, CVAR_ARCHIVE)

SoundRenderer *GSnd;
bool nosound;
bool nosfx;


//
// SFX API
//

//==========================================================================
//
// CVAR snd_sfxvolume
//
// Maximum volume of a sound effect.
//==========================================================================

CUSTOM_CVAR (Float, snd_sfxvolume, 0.5f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_NOINITCALL)
{
	if (self < 0.f)
		self = 0.f;
	else if (self > 1.f)
		self = 1.f;
	else if (GSnd != NULL)
	{
		GSnd->SetSfxVolume (self);
	}
}

class NullSoundRenderer : public SoundRenderer
{
public:
	virtual bool IsNull() { return true; }
	void SetSfxVolume (float volume)
	{
	}
	void SetMusicVolume (float volume)
	{
	}
	SoundHandle LoadSound(BYTE *sfxdata, int length, int def_loop_start, int def_loop_end)
	{
		SoundHandle retval = { NULL };
		return retval;
	}
	SoundHandle LoadSoundRaw(BYTE *sfxdata, int length, int frequency, int channels, int bits, int loopstart, int loopend)
	{
		SoundHandle retval = { NULL };
		return retval;
	}
	void UnloadSound (SoundHandle sfx)
	{
	}
	unsigned int GetMSLength(SoundHandle sfx)
	{
		// Return something that isn't 0. This is only used by some
		// ambient sounds to specify a default minimum period.
		return 250;
	}
	unsigned int GetSampleLength(SoundHandle sfx)
	{
		return 0;
	}
	float GetOutputRate()
	{
		return 11025;	// Lies!
	}
	void StopChannel(FISoundChannel *chan)
	{
	}
	void ChannelVolume(FISoundChannel *, float)
	{
	}

	// Streaming sounds.
	SoundStream *CreateStream (SoundStreamCallback callback, int buffbytes, int flags, int samplerate, void *userdata)
	{
		return NULL;
	}
	SoundStream *OpenStream (const char *filename, int flags, int offset, int length)
	{
		return NULL;
	}

	// Starts a sound.
	FISoundChannel *StartSound (SoundHandle sfx, float vol, int pitch, int chanflags, FISoundChannel *reuse_chan)
	{
		return NULL;
	}
	FISoundChannel *StartSound3D (SoundHandle sfx, SoundListener *listener, float vol, FRolloffInfo *rolloff, float distscale, int pitch, int priority, const FVector3 &pos, const FVector3 &vel, int channum, int chanflags, FISoundChannel *reuse_chan)
	{
		return NULL;
	}

	// Marks a channel's start time without actually playing it.
	void MarkStartTime (FISoundChannel *chan)
	{
	}

	// Returns position of sound on this channel, in samples.
	unsigned int GetPosition(FISoundChannel *chan)
	{
		return 0;
	}

	// Gets a channel's audibility (real volume).
	float GetAudibility(FISoundChannel *chan)
	{
		return 0;
	}

	// Synchronizes following sound startups.
	void Sync (bool sync)
	{
	}

	// Pauses or resumes all sound effect channels.
	void SetSfxPaused (bool paused, int slot)
	{
	}

	// Pauses or resumes *every* channel, including environmental reverb.
	void SetInactive(SoundRenderer::EInactiveState inactive)
	{
	}

	// Updates the volume, separation, and pitch of a sound channel.
	void UpdateSoundParams3D (SoundListener *listener, FISoundChannel *chan, bool areasound, const FVector3 &pos, const FVector3 &vel)
	{
	}

	void UpdateListener (SoundListener *)
	{
	}
	void UpdateSounds ()
	{
	}

	bool IsValid ()
	{
		return true;
	}
	void PrintStatus ()
	{
		Printf("Null sound module active.\n");
	}
	void PrintDriversList ()
	{
		Printf("Null sound module uses no drivers.\n");
	}
	FString GatherStats ()
	{
		return "Null sound module has no stats.";
	}

	virtual MIDIDevice* CreateMIDIDevice() const override
	{
		return NULL;
	}
};

void I_InitSound ()
{
	/* Get command line options: */
	nosound = !!Args->CheckParm ("-nosound") || !!Args->CheckParm("-host"); // [BB] No sound for the server
	nosfx = !!Args->CheckParm ("-nosfx") || !!Args->CheckParm("-host"); // [BB]

	if (nosound)
	{
		GSnd = new NullSoundRenderer;
		I_InitMusic ();
		return;
	}

	// This has been extended to allow falling back from FMod to OpenAL and vice versa if the currently active sound system cannot be found.
	if (stricmp(snd_backend, "null") == 0)
	{
		GSnd = new NullSoundRenderer;
	}
	else if (stricmp(snd_backend, "fmod") == 0)
	{
		#ifndef NO_FMOD
		if (IsFModExPresent())
		{
			GSnd = new FMODSoundRenderer;
		}
		#endif
		#ifndef NO_OPENAL
		if ((!GSnd || !GSnd->IsValid()) && IsOpenALPresent())
		{
			Printf( TEXTCOLOR_RED "FMod Ex Sound init failed. Trying OpenAL.\n");
			I_CloseSound();
			GSnd = new OpenALSoundRenderer;
			snd_backend = "openal";
		}
		#endif
	}
	else if (stricmp(snd_backend, "openal") == 0)
	{
		#ifndef NO_OPENAL
		if (IsOpenALPresent())
		{
			GSnd = new OpenALSoundRenderer;
		}
		#endif
		#ifndef NO_FMOD
		if ((!GSnd || !GSnd->IsValid()) && IsFModExPresent())
		{
			Printf( TEXTCOLOR_RED "OpenAL Sound init failed. Trying FMod Ex.\n");
			I_CloseSound();
			GSnd = new FMODSoundRenderer;
			snd_backend = "fmod";
		}
		#endif
	}
	else
	{
		Printf(TEXTCOLOR_RED "%s: Unknown sound system specified\n", *snd_backend);
		snd_backend = "null";
	}
	if (!GSnd || !GSnd->IsValid())
	{
		I_CloseSound();
		GSnd = new NullSoundRenderer;
		Printf(TEXTCOLOR_RED "Sound init failed. Using nosound.\n");
	}
	I_InitMusic ();
	snd_sfxvolume.Callback ();
}


void I_CloseSound ()
{
	// Free all loaded samples
	for (unsigned i = 0; i < S_sfx.Size(); i++)
	{
		S_UnloadSound(&S_sfx[i]);
	}

	delete GSnd;
	GSnd = NULL;
}

void I_ShutdownSound()
{
	if (GSnd != NULL)
	{
		S_StopAllChannels();
		I_CloseSound();
	}
}

CCMD (snd_status)
{
	GSnd->PrintStatus ();
}

CCMD (snd_reset)
{
	I_ShutdownMusic();
	S_EvictAllChannels();
	I_CloseSound();
	I_InitSound();
	S_RestartMusic();
	S_RestoreEvictedChannels();
}

CCMD (snd_listdrivers)
{
	GSnd->PrintDriversList ();
}

ADD_STAT (sound)
{
	return GSnd->GatherStats ();
}

SoundRenderer::SoundRenderer ()
{
}

SoundRenderer::~SoundRenderer ()
{
}

FString SoundRenderer::GatherStats ()
{
	return "No stats for this sound renderer.";
}

short *SoundRenderer::DecodeSample(int outlen, const void *coded, int sizebytes, ECodecType ctype)
{
    MemoryReader reader((const char*)coded, sizebytes);
    short *samples = (short*)calloc(1, outlen);
    ChannelConfig chans;
    SampleType type;
    int srate;

    SoundDecoder *decoder = CreateDecoder(&reader);
    if(!decoder) return samples;

    decoder->getInfo(&srate, &chans, &type);
    if(chans != ChannelConfig_Mono || type != SampleType_Int16)
    {
        DPrintf("Sample is not 16-bit mono\n");
        delete decoder;
        return samples;
    }

    decoder->read((char*)samples, outlen);
    delete decoder;
    return samples;
}

void SoundRenderer::DrawWaveDebug(int mode)
{
}

SoundStream::~SoundStream ()
{
}

bool SoundStream::SetPosition(unsigned int pos)
{
	return false;
}

bool SoundStream::SetOrder(int order)
{
	return false;
}

FString SoundStream::GetStats()
{
	return "No stream stats available.";
}

//==========================================================================
//
// SoundRenderer :: LoadSoundVoc
//
//==========================================================================

SoundHandle SoundRenderer::LoadSoundVoc(BYTE *sfxdata, int length)
{
	BYTE * data = NULL;
	int len, frequency, channels, bits, loopstart, loopend;
	len = frequency = channels = bits = 0;
	loopstart = loopend = -1;
	do if (length > 26)
	{
		// First pass to parse data and validate the file
		if (strncmp ((const char *)sfxdata, "Creative Voice File", 19))
			break;
		int i = 26, blocktype = 0, blocksize = 0, codec = -1;
		bool noextra = true, okay = true;
		while (i < length)
		{
			// Read block header
			blocktype = sfxdata[i];
			if (blocktype == 0)
				break;
			blocksize = sfxdata[i+1] + (sfxdata[i+2]<<8) + (sfxdata[i+3]<<16);
			i += 4;
			if (i + blocksize > length)
			{
				okay = false;
				break;
			}

			// Read block data
			switch (blocktype)
			{
			case 1: // Sound data
				if (noextra && (codec == -1 || codec == sfxdata[i+1]))
				{
					frequency = 1000000/(256 - sfxdata[i]);
					channels = 1;
					codec = sfxdata[i+1];
					if (codec == 0)
						bits = 8;
					else if (codec == 4)
						bits = -16;
					else okay = false;
					len += blocksize - 2;
				}
				break;
			case 2: // Sound data continuation
				if (codec == -1)
					okay = false;
				len += blocksize;
				break;
			case 3: // Silence
				if (frequency == 1000000/(256 - sfxdata[i+2]))
				{
					int silength = 1 + sfxdata[i] + (sfxdata[i+1]<<8);
					if (codec == 0) // 8-bit unsigned PCM
						len += silength;
					else if (codec == 4) // 16-bit signed PCM
						len += silength<<1;
					else okay = false;
				} else okay = false;
				break;
			case 4: // Mark (ignored)
			case 5: // Text (ignored)
				break;
			case 6: // Repeat start
				loopstart = len;
				break;
			case 7: // Repeat end
				loopend = len;
				if (loopend < loopstart)
					okay = false;
				break;
			case 8: // Extra info
				noextra = false;
				if (codec == -1)
				{
					codec = sfxdata[i+2];
					channels = 1+sfxdata[i+3];
					frequency = 256000000/(channels * (65536 - (sfxdata[i]+(sfxdata[i+1]<<8))));
				} else okay = false;
				break;
			case 9: // Sound data in new format
				if (codec == -1)
				{
					frequency = sfxdata[i] + (sfxdata[i+1]<<8) + (sfxdata[i+2]<<16) + (sfxdata[i+3]<<24);
					bits = sfxdata[i+4];
					channels = sfxdata[i+5];
					codec = sfxdata[i+6] + (sfxdata[i+7]<<8);
					if (codec == 0)
						bits = 8;
					else if (codec == 4)
						bits = -16;
					else okay = false;
					len += blocksize - 12;
				} else okay = false;
				break;
			default: // Unknown block type
				okay = false;
				DPrintf ("Unknown VOC block type %i\n", blocktype);
				break;
			}
			// Move to next block
			i += blocksize;
		}

		// Second pass to write the data
		if (okay)
		{
			data = new BYTE[len];
			i = 26;
			int j = 0;
			while (i < length)
			{
				// Read block header again
				blocktype = sfxdata[i];
				if (blocktype == 0) break;
				blocksize = sfxdata[i+1] + (sfxdata[i+2]<<8) + (sfxdata[i+3]<<16);
				i += 4;
				switch (blocktype)
				{
				case 1: memcpy(data+j, sfxdata+i+2,  blocksize-2 ); j += blocksize-2;	break;
				case 2: memcpy(data+j, sfxdata+i,    blocksize   ); j += blocksize;		break;
				case 9: memcpy(data+j, sfxdata+i+12, blocksize-12); j += blocksize-12;	break;
				case 3:
					{
						int silength = 1 + sfxdata[i] + (sfxdata[i+1]<<8);
						if (bits == 8)
						{
							memset(data+j, 128, silength);
							j += silength;
						}
						else if (bits == -16)
						{
							memset(data+j, 0, silength<<1);
							j += silength<<1;
						}
					}
					break;
				default: break;
				}
			}
		}

	} while (false);
	SoundHandle retval = LoadSoundRaw(data, len, frequency, channels, bits, loopstart, loopend);
	if (data) delete[] data;
	return retval;
}

SoundDecoder* SoundRenderer::CreateDecoder(FileReader* reader)
{
	SoundDecoder* decoder = NULL;
	int pos = reader->Tell();

#ifdef HAVE_SNDFILE
	decoder = new SndFileDecoder;
	if (decoder->open(reader))
		return decoder;
	reader->Seek(pos, SEEK_SET);

	delete decoder;
	decoder = NULL;
#endif
#ifdef HAVE_MPG123
	decoder = new MPG123Decoder;
	if (decoder->open(reader))
		return decoder;
	reader->Seek(pos, SEEK_SET);

	delete decoder;
	decoder = NULL;
#endif
	return decoder;
}


// Default readAll implementation, for decoders that can't do anything better
TArray<char> SoundDecoder::readAll()
{
    TArray<char> output;
    size_t total = 0;
    size_t got;

    output.Resize(total+32768);
    while((got=read(&output[total], output.Size()-total)) > 0)
    {
        total += got;
        output.Resize(total*2);
    }
    output.Resize(total);
    return output;
}
