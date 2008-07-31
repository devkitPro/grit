//
// cldib_test.cpp
//   These are just tests for various functions. Feel free to 
//   remove from compilation.
//

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "cldib.h"
#include "cldib_quant.h"

extern HBITMAP hTiles;
extern CLDIB *dibTiles;

void test_bup();
void test_bit_rev();
void test_byte_rev();
void test_pack();
void test_convert();
void test_copy();
void test_redim();
void test_pcx();
void test_png();
void test_tga();
void test_quant();

void test_main()
{
	//test_byte_rev();	// CHK (odd, even & inplace)
	//test_bit_rev();	// CHK (2,4,8,inplace)
	//test_bup();		// CHK (x<=8 bpp ; LE vs BE)
	//test_pack();		// CHK (x<=8 bpp, LE vs BE)
	//test_convert();	// CHK (p2p, p2t, 24_t, t2p)
	//test_copy();		// CHK ... mostly
	//test_redim();		// CHK basics; no pad-tests yet
	//test_pcx();		// CHK save(4,8,24)
	//test_png();		// CHK save(4,8)
	//test_tga();		// CHK save(8,16,24) (1 failed!?!)
	//test_quant();		// CHK with diff palsizes too, w00t!
}

void test_quant()
{
	CLDIB *src= NULL, *dst= NULL;
	src= BmpLoad("bastion.bmp");
	if(src==NULL)
		return;
	
	dst= dib_true_to_8_copy(src, 64);
	BmpSave("bastion8.bmp", dst);

	dib_free(dst);
	dib_free(src);
}

void test_tga()
{
	CLDIB *dib=NULL;

	//TgaSave("tiles8.tga", dibTiles);

	dib= dib_convert_copy(dibTiles, 24, 0);
	TgaSave("tiles24.tga", dib);
	dib_free(dib);
}

void test_png()
{
	CLDIB *dib=NULL;

	PngSave("tiles8.png", dibTiles);

	dib= dib_convert_copy(dibTiles, 4, 0);
	PngSave("tiles4.png", dib);
	dib_free(dib);
}

void test_pcx()
{
	CLDIB *dib=NULL;
	CPcxFile pcx;

	PcxSave("tiles8.pcx", dibTiles);

	dib= dib_convert_copy(dibTiles, 4, 0);
	PcxSave("tiles4.pcx", dib);
	dib_free(dib);
}

void test_redim()
{
	CLDIB *dib=NULL, *dib2=NULL;
	CBmpFile bmp;

	dib= dib_redim_copy(dibTiles, 8, 8, 0);
	bmp.Attach(dib);
	bmp.Save("tiles_rd8.bmp");
	bmp.Destroy();

	dib= dib_redim_copy(dibTiles, 16, 16, 0);
	bmp.Attach(dib);
	bmp.Save("tiles_rd16.bmp");
	bmp.Destroy();

	dib= dib_redim_copy(dibTiles, 16, 16, 0);
	dib2= dib_redim_copy(dib, 8, 8, 0);
	bmp.Attach(dib2);
	bmp.Save("tiles_rd16_8.bmp");
	bmp.Destroy();
	dib_free(dib);

	dib= dib_redim_copy(dibTiles, 8, 8, 0);
	dib2= dib_redim_copy(dib, 32, 8, 0);
	bmp.Attach(dib2);
	bmp.Save("tiles_rd_inv.bmp");
	bmp.Destroy();
	dib_free(dib);
}

#if 0
// tested for:
// clipping y/n ; oob rects; 4,8,24bpp, (un)aligned
void test_copy()
{
	CLDIB *dib=NULL, *dib2=NULL;

	dib= dib_copy(dibTiles, 0, 0, 16, 16, true);
	dib_save_bmp("tiles0-0-16-16.bmp", dib);
	dib_free(dib);


	dib= dib_copy(dibTiles, -8, -8, 64+8, 16+8, false);
	dib_save_bmp("tilesn8-n8-72-24.bmp", dib);
	dib_free(dib);	

	
	dib2= dib_convert(dibTiles, 4, 0);
	dib= dib_copy(dib2, 3, 8, 16, 15, true);
	dib_save_bmp("tiles3-8-16-15.bmp", dib);
	dib_free(dib2);	
	dib_free(dib);	
}

// pal->pal; pal->true work
void test_convert()
{
	CLDIB *dib=NULL;
	dib= dib_convert(dibTiles, 8, 0);
	dib_save_bmp("tiles8.bmp", dib);
	dib_free(dib);
	dib= dib_convert(dibTiles, 16, 0);
	dib_save_bmp("tiles16.bmp", dib);
	dib_free(dib);

	dib= dib_convert(dibTiles, 24, 0);
	BmpSave("tiles24.bmp", dib);
	dib2= dib_convert(dib,16,0);
	BmpSave("tiles24_16.bmp", dib2);
	dib_free(dib2);
	dib2= dib_convert(dib,32,0);
	BmpSave("tiles24_32.bmp", dib2);
	dib_free(dib2);
	dib_free(dib);

	dib= dib_convert(dibTiles, 32, 0);
	dib_save_bmp("tiles32.bmp", dib);
	dib_free(dib);

	dib= dib_convert(dibTiles, 4, 0);
	dib_save_bmp("tiles4.bmp", dib);
	CLDIB *dib2= dib_convert(dib, 24, 0);
	dib_save_bmp("tiles4_24.bmp", dib2);
	dib_free(dib2);
	dib_free(dib);

	dib= dib_convert(dibTiles, 1, 2);
	dib_save_bmp("tiles1.bmp", dib);
	dib_free(dib);
}
#endif

void test_pack()
{
	BYTE src[64]= 
	{
		0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 1, 1, 1, 0, 0, 
		0, 1, 0, 0, 0, 0, 1, 0, 
		0, 1, 1, 1, 1, 1, 0, 0, 

		0, 1, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 1, 0, 
		0, 1, 1, 1, 1, 1, 0, 0, 
		2, 2, 2, 2, 2, 2, 2, 2
	};
	DWORD dst[64];
	DWORD dst2[64];

	int ix, iy;

	FILE *fp= fopen("log.txt", "w");

	// printf src
	for(iy=0; iy<64; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", src[iy+ix]);
		fputc('\n', fp);
	}

	data_bit_pack(dst, src, 64, 8, 1, 0);

	fputs("\n 8->1 LEbit, bytes\n", fp);
	// print dst
	for(iy=0; iy<8; iy++)
		fprintf(fp, "%02x ", ((BYTE*)dst)[iy]);
	fputc('\n', fp);

	fputs("\n 8->1 LEbit, bits\n", fp);
	data_bit_unpack(dst2, dst, 8, 1, 4, 0);

	// print dst
	for(iy=0; iy<8; iy++)
	{
		fprintf(fp, "%08x ", dst2[iy]);
		fputc('\n', fp);
	}

	fputs("\n 8->1 BEbit, bits\n", fp);
	data_bit_pack(dst, src, 64, 8, 1, (1<<30));
	data_bit_unpack(dst2, dst, 8, 1, 4, 0);
	// print dst
	for(iy=0; iy<8; iy++)
	{
		fprintf(fp, "%08x ", dst2[iy]);
		fputc('\n', fp);
	}

	fputs("\n 8->4 LEbit, bits\n", fp);
	data_bit_pack(dst2, src, 64, 8, 4, 0);
	// print dst
	for(iy=0; iy<8; iy++)
	{
		fprintf(fp, "%08x ", dst2[iy]);
		fputc('\n', fp);
	}
		
	fputs("\n 8->1 BEbit + 4->1 BEbit, bits\n", fp);
	data_bit_pack(dst2, src, 64, 8, 4, (1<<30));
	data_bit_pack(dst, dst2, 32, 4, 1, (1<<30));
	data_bit_unpack(dst2, dst, 8, 1, 4, 0);

	// print dst
	for(iy=0; iy<8; iy++)
	{
		fprintf(fp, "%08x ", dst2[iy]);
		fputc('\n', fp);
	}
}


// NOTES: On unpacking
// LEbit + LEbyte is easiest; If you do a 1->4 of these, the u32 
//   you get are the component bits of the bytes, in math order (==LE)
//   Pretty useful for debugging bit-fiddling
// BEbit + LEbyte is almost the same, but you need to use 
//   xShift^(8-xBpp) in the conversion for both x=src and dst.
// BEbit+LeByte is the standard for windows DIBs. Which sucks >_<, but 
//  there's little I can do about that.

void test_bup()
{
	BYTE src[32];
	DWORD dst[64];

	int ii, ix, iy;
	for(ii=0; ii<32; ii++)
		src[ii]= 16+ii;

	data_bit_unpack(dst, src, 32, 1, 4, 0);

	FILE *fp= fopen("log.txt", "w");

	// printf src
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", src[iy+ix]);
		fputc('\n', fp);
	}

	fputc('\n', fp);
	// print dst
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%08x ", dst[iy+ix]);
		fputc('\n', fp);
	}

	// test bup with BE-bits
	BYTE dst2[64];

	// the long way round
	//data_bit_rev(dst, src, 8, 1);
	//data_bit_unpack(dst2, dst, 8, 1, 4, 0);
	//data_bit_rev(dst2, dst2, 32, 4);
	data_bit_unpack(dst2, src, 8, 1, 4, 0);

	data_bit_unpack(dst, dst2, 64, 1, 4, 0);

	fputc('\n', fp);
	// printf src
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", ((BYTE*)dst2)[iy+ix]);
		fputc('\n', fp);
	}
	// print dst
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%08x ", dst[iy+ix]);
		fputc('\n', fp);
	}

	// BE-nits in one go
	data_bit_unpack(dst2, src, 8, 1, 4, (1<<30));

	// and see if the bit-rev is ok.
	// dib_bit_rev(src, src, 32, 2);
	data_bit_unpack(dst, dst2, 64, 1, 4, 0);

	fputc('\n', fp);
	// printf src
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", ((BYTE*)dst2)[iy+ix]);
		fputc('\n', fp);
	}
	// print dst
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%08x ", dst[iy+ix]);
		fputc('\n', fp);
	}

	fclose(fp);
}

void test_bit_rev()
{
	BYTE src[32], dst[32];

	int ii, ix, iy;
	for(ii=0; ii<32; ii++)
		src[ii]= (ii&15) | ((ii^15)<<4);

	//memcpy(dst, src, 32);

	data_bit_rev(dst, dst, 32, 4);

	FILE *fp= fopen("log.txt", "w");

	// printf src
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", src[iy+ix]);
		fputc('\n', fp);
	}

	fputc('\n', fp);
	// print dst
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", dst[iy+ix]);
		fputc('\n', fp);
	}

	fclose(fp);
}

void test_byte_rev()
{
	BYTE src[32], dst[32];

	int ii, ix, iy;
	for(ii=0; ii<32; ii++)
		src[ii]= ii;
	memcpy(dst, src, 32);

	data_byte_rev(dst, dst, 32, 4);

	FILE *fp= fopen("log.txt", "w");

	// printf src
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", src[iy+ix]);
		fputc('\n', fp);
	}

	fputc('\n', fp);
	// print dst
	for(iy=0; iy<32; iy += 8)
	{
		for(ix=0; ix<8; ix++)
			fprintf(fp, "%02x ", dst[iy+ix]);
		fputc('\n', fp);
	}

	fclose(fp);
}

// EOF
