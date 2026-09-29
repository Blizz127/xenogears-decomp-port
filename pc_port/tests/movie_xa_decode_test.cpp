#include "audio/PsyX_SPUAL.h"
#include "psx/libcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void PsyX_Log_Warning(const char*, ...)
{
}

int PsyX_CdXaSectorAccepted(int mode, int filterFile,
	                         int filterChannel, int file,
	                         int channel, int submode);

static int write_disc_resample(const char* discPath, const char* pcmPath)
{
	static const long lbas[2] = {48, 56};
	unsigned char raw[2352];
	short pcm[2352 * 2];
	FILE* disc = fopen(discPath, "rb");
	FILE* output;
	int sector;
	if (!disc)
	{
		perror("open Disc 1");
		return 1;
	}
	output = fopen(pcmPath, "wb");
	if (!output)
	{
		perror("open XA PCM output");
		fclose(disc);
		return 1;
	}
	for (sector = 0; sector < 2; sector++)
	{
		int frames;
		if (fseek(disc, lbas[sector] * 2352L, SEEK_SET) != 0 ||
		    fread(raw, 1, sizeof(raw), disc) != sizeof(raw))
		{
			fprintf(stderr, "failed reading Disc 1 LBA %ld\n", lbas[sector]);
			fclose(output);
			fclose(disc);
			return 1;
		}
		if (raw[15] != 2 || memcmp(raw + 16, raw + 20, 4) != 0 ||
		    raw[16] != 1 || raw[17] != 1 || raw[18] != 0x64 || raw[19] != 1)
		{
			fprintf(stderr, "unexpected Disc 1 XA header at LBA %ld\n", lbas[sector]);
			fclose(output);
			fclose(disc);
			return 1;
		}
		frames = PsyX_SPUAL_XaDecodeResampleDebug(raw + 24, 2304,
		                                              raw[19], pcm,
		                                              sector == 0);
		if (frames != 2352 ||
		    fwrite(pcm, sizeof(short) * 2, frames, output) != (size_t)frames)
		{
			fprintf(stderr, "Disc 1 XA resample failed at LBA %ld: frames=%d\n",
			        lbas[sector], frames);
			fclose(output);
			fclose(disc);
			return 1;
		}
	}
	fclose(output);
	fclose(disc);
	return 0;
}

int main(int argc, char** argv)
{
	unsigned char sector[2304];
	short pcm[2352 * 2];
	short mixedLeft;
	short mixedRight;
	int frames;

	memset(sector, 0, sizeof(sector));
	/* Four stereo XA parameter bytes per block: filter 0, shift 11. */
	for (int group = 0; group < 18; ++group)
		for (int block = 0; block < 4; ++block)
		{
			sector[group * 128 + 4 + block * 2] = 0x01;
			sector[group * 128 + 5 + block * 2] = 0x01;
		}
	/* First frame: left nibble 1 (2048), right nibble 2 (4096). */
	sector[16] = 0x21;
	frames = PsyX_SPUAL_XaDecodeDebug(sector, sizeof(sector), 0x01, pcm);
	if (frames != 2016 || pcm[0] != 2048 || pcm[1] != 4096)
	{
		fprintf(stderr, "XA impulse mismatch: frames=%d L=%d R=%d\n",
		        frames, pcm[0], pcm[1]);
		return 1;
	}
	for (int i = 2; i < frames * 2; ++i)
		if (pcm[i] != 0)
		{
			fprintf(stderr, "XA zero tail mismatch at sample %d: %d\n", i, pcm[i]);
			return 1;
		}
	if (PsyX_SPUAL_XaDecodeDebug(sector, sizeof(sector), 0x00, pcm) != 0)
	{
		fprintf(stderr, "XA mono coding was not rejected\n");
		return 1;
	}
	if (PsyX_SPUAL_XaDecodeDebug(sector, sizeof(sector), 0x11, pcm) != 0)
	{
		fprintf(stderr, "XA 8-bit coding was not rejected\n");
		return 1;
	}
	if (!PsyX_CdXaSectorAccepted(CdlModeRT, 7, 9, 1, 2, 0x44) ||
	    PsyX_CdXaSectorAccepted(CdlModeRT, 7, 9, 1, 2, 0x04) ||
	    PsyX_CdXaSectorAccepted(CdlModeRT, 7, 9, 1, 2, 0x46) ||
	    PsyX_CdXaSectorAccepted(CdlModeRT, 7, 9, 1, 2, 0x4C) ||
	    PsyX_CdXaSectorAccepted(0, 7, 9, 7, 9, 0x44) ||
	    !PsyX_CdXaSectorAccepted(CdlModeRT | CdlModeSF, 7, 9, 7, 9, 0x44) ||
	    PsyX_CdXaSectorAccepted(CdlModeRT | CdlModeSF, 7, 9, 7, 8, 0x44))
	{
		fprintf(stderr, "XA CdlModeRT/SF/submode routing mismatch\n");
		return 1;
	}
	frames = PsyX_SPUAL_XaDecodeResampleDebug(sector, sizeof(sector),
	                                          0x01, pcm, 1);
	if (frames != 2352)
	{
		fprintf(stderr, "XA zigzag frame count mismatch: %d\n", frames);
		return 1;
	}
	PsyX_SPUAL_XaMixDebug(0, 0x7FFF, 0x7FFF, 2048, -4096,
	                      &mixedLeft, &mixedRight);
	if (mixedLeft != 0 || mixedRight != 0)
	{
		fprintf(stderr, "XA CD mix disable did not mute: L=%d R=%d\n",
		        mixedLeft, mixedRight);
		return 1;
	}
	PsyX_SPUAL_XaMixDebug(1, 0x4000, 0x2000, 2048, -4096,
	                      &mixedLeft, &mixedRight);
	if (mixedLeft != 1024 || mixedRight != -1024)
	{
		fprintf(stderr, "XA CD mix/volume mismatch: L=%d R=%d\n",
		        mixedLeft, mixedRight);
		return 1;
	}
	PsyX_SPUAL_XaAttenuationDebug(0x40, 0x20, 0x10, 0x80,
	                              2048, -4096,
	                              &mixedLeft, &mixedRight);
	if (mixedLeft != 512 || mixedRight != -3584)
	{
		fprintf(stderr, "XA CD attenuation matrix mismatch: L=%d R=%d\n",
		        mixedLeft, mixedRight);
		return 1;
	}
	PsyX_SPUAL_XaAttenuationDebug(0xff, 0, 0xff, 0,
	                              32767, 32767,
	                              &mixedLeft, &mixedRight);
	if (mixedLeft != 32767 || mixedRight != 0)
	{
		fprintf(stderr, "XA CD attenuation saturation mismatch: L=%d R=%d\n",
		        mixedLeft, mixedRight);
		return 1;
	}
	if (argc == 3 && write_disc_resample(argv[1], argv[2]) != 0)
		return 1;
	puts("movie XA decode/resample: PASS");
	return 0;
}
