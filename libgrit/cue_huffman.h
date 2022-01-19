/*----------------------------------------------------------------------------*/
/*--  huffman.c - Huffman coding for Nintendo GBA/DS                        --*/
/*--  Copyright (C) 2011 CUE                                                --*/
/*--                                                                        --*/
/*--  This program is free software: you can redistribute it and/or modify  --*/
/*--  it under the terms of the GNU General Public License as published by  --*/
/*--  the Free Software Foundation, either version 3 of the License, or     --*/
/*--  (at your option) any later version.                                   --*/
/*--                                                                        --*/
/*--  This program is distributed in the hope that it will be useful,       --*/
/*--  but WITHOUT ANY WARRANTY; without even the implied warranty of        --*/
/*--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          --*/
/*--  GNU General Public License for more details.                          --*/
/*--                                                                        --*/
/*--  You should have received a copy of the GNU General Public License     --*/
/*--  along with this program. If not, see <http://www.gnu.org/licenses/>.  --*/
/*----------------------------------------------------------------------------*/

#ifndef CUE_HUFFMAN_H
#define CUE_HUFFMAN_H

#define CMD_CODE_20   0x20       // Huffman magic number (to find best mode)
#define CMD_CODE_28   0x28       // 8-bits Huffman magic number
#define CMD_CODE_24   0x24       // 4-bits Huffman magic number

unsigned char *HUF_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int mode);

#endif
