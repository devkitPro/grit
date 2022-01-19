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

/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cue_huffman.h"

/*----------------------------------------------------------------------------*/
//#define _CUE_LOG_                // enable log mode (for test purposes)
//#define _CUE_MODES_21_22_        // enable modes 0x21-0x22 (for test purposes)

/*----------------------------------------------------------------------------*/
#define HUF_LNODE     0          // left node
#define HUF_RNODE     1          // right node

#define HUF_SHIFT     1          // bits to shift
#define HUF_MASK      0x80       // first bit to check (1 << 7)
#define HUF_MASK4     0x80000000 // first bit to check (HUF_RNODE << 31)

#define HUF_LCHAR     0x80       // next lnode is a char, bit 7, (1 << 7)
#define HUF_RCHAR     0x40       // next rnode is a char, bit 6, (1 << 6)
#define HUF_NEXT      0x3F       // inc to next node/char (nwords+1), bits 5-0
                                 // * (0xFF & ~(HUF_LCHAR | HUF_RCHAR))

/*----------------------------------------------------------------------------*/
typedef struct _huffman_node {
  unsigned int          symbol;
  unsigned int          weight;
  unsigned int          leafs;
  struct _huffman_node *dad;
  struct _huffman_node *lson;
  struct _huffman_node *rson;
} huffman_node;

typedef struct _huffman_code {
  unsigned int   nbits;
  unsigned char *codework;
} huffman_code;

typedef struct _huffman_data {
  unsigned int   *freqs;
  huffman_node  **tree;
  unsigned char  *codetree, *codemask;
  huffman_code  **codes;
  unsigned int    max_symbols, num_leafs, num_nodes;
} huffman_data;

/*----------------------------------------------------------------------------*/
#define EXIT(text)  { printf(text); exit(-1); }

/*----------------------------------------------------------------------------*/
unsigned char *HUF_Code_Impl(unsigned char *raw_buffer, int raw_len, int *new_len, unsigned int num_bits);

void  HUF_InitFreqs(huffman_data* data);
void  HUF_CreateFreqs(unsigned char *raw_buffer, int raw_len, unsigned int num_bits, huffman_data* data);
void  HUF_FreeFreqs(huffman_data* data);
void  HUF_InitTree(huffman_data* data);
void  HUF_CreateTree(huffman_data* data);
void  HUF_FreeTree(huffman_data* data);
void  HUF_InitCodeTree(huffman_data* data);
void  HUF_CreateCodeTree(huffman_data* data);
int   HUF_CreateCodeBranch(huffman_node *root, unsigned int p, unsigned int q, huffman_data* data);
void  HUF_UpdateCodeTree(huffman_data* data);
void  HUF_FreeCodeTree(huffman_data* data);
void  HUF_InitCodeWorks(huffman_data* data);
void  HUF_CreateCodeWorks(huffman_data* data);
void  HUF_FreeCodeWorks(huffman_data* data);

/*----------------------------------------------------------------------------*/
unsigned char *HUF_Code(unsigned char *raw_buffer, int raw_len, int *new_len, int mode) {
  unsigned char *pak_buffer, *new_buffer;
  int pak_len = INT_MAX;

  unsigned int num_bits = mode & 0xF;
  pak_buffer = NULL;

  if (!num_bits) {
    num_bits = CMD_CODE_28 - CMD_CODE_20;
    new_buffer = HUF_Code_Impl(raw_buffer, raw_len, new_len, num_bits);
    if (*new_len < pak_len) {
      free(pak_buffer);
      pak_buffer = new_buffer;
      pak_len = *new_len;
    }
    else {
        free(new_buffer);
    }
    num_bits = CMD_CODE_24 - CMD_CODE_20;
  }
  new_buffer = HUF_Code_Impl(raw_buffer, raw_len, new_len, num_bits);
  if (*new_len < pak_len) {
    free(pak_buffer);
    pak_buffer = new_buffer;
    pak_len = *new_len;
  }
  else {
      free(new_buffer);
  }

  *new_len = pak_len;
  return pak_buffer;
}

/*----------------------------------------------------------------------------*/
static char *Memory(int length, int size) {
  char *fb;

  fb = (char *) calloc(length, size);
  if (fb == NULL) EXIT("\nMemory error\n");

  return(fb);
}

/*----------------------------------------------------------------------------*/
unsigned char *HUF_Code_Impl(unsigned char *raw_buffer, int raw_len, int *new_len, unsigned int num_bits) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *cod;
  unsigned int   pak_len, len;
  huffman_code  *code;
  unsigned char *cwork, mask;
  unsigned int  *pk4, mask4, ch, nbits;

  huffman_data data;
  data.max_symbols = 1 << num_bits;

  pak_len = 4 + (data.max_symbols << 1) + raw_len + 3;
  pak_buffer = (unsigned char *) Memory(pak_len, sizeof(char));

  *(unsigned int *)pak_buffer = (CMD_CODE_20 + num_bits) | (raw_len << 8);

  pak = pak_buffer + 4;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  HUF_InitFreqs(&data);
  HUF_CreateFreqs(raw_buffer, raw_len, num_bits, &data);

  HUF_InitTree(&data);
  HUF_CreateTree(&data);

  HUF_InitCodeTree(&data);
  HUF_CreateCodeTree(&data);

  HUF_InitCodeWorks(&data);
  HUF_CreateCodeWorks(&data);

  cod = data.codetree;
  len = (*cod + 1) << 1;
  while (len--) *pak++ = *cod++;

  mask4 = 0;
  while (raw < raw_end) {
    ch = *raw++;


    for (nbits = 8; nbits; nbits -= num_bits) {
      code = data.codes[ch & ((1 << num_bits) - 1)];
////  code = codes[ch >> (8 - num_bits)];
      if (code == NULL) EXIT(", ERROR: code without codework!"); // never!

      len   = code->nbits;
      cwork = code->codework;

      mask = HUF_MASK;
      while (len--) {
        if (!(mask4 >>= HUF_SHIFT)) {
          mask4 = HUF_MASK4;
          *(pk4 = (unsigned int *)pak) = 0;
          pak += 4;
        }
        if (*cwork & mask) *pk4 |= mask4;
        if (!(mask >>= HUF_SHIFT)) {
          mask = HUF_MASK;
          cwork++;
        }
      }

      ch >>= num_bits;
////  ch = (ch << num_bits) & 0xFF;
    }
  }

  pak_len = pak - pak_buffer;

#ifdef _CUE_LOG_
  printf("\n--- CreateCode --------------------------------\n");
  pak = pak_buffer;
  printf("%02X %02X %02X %02X\n", pak[0], pak[1], pak[2], pak[3]);
  printf("(+CodeTree)\n");
  pak += 4 + ((pak[4] + 1) << 1);
  for (len = 0; len < 256; len++) {
    printf("%02X", *pak++);
    printf((len & 0xF) == 0xF ? "\n" : " ");
    if (pak == pak_buffer + pak_len) break;
  }
  if (pak < pak_buffer + pak_len) {
    printf("(+%X more byte", pak_buffer + pak_len - pak);
    if (pak + 1 !=  pak_buffer + pak_len) printf("s");
    printf(")\n");
  }
  printf("\n");
#endif

  HUF_FreeCodeWorks(&data);
  HUF_FreeCodeTree(&data);
  HUF_FreeTree(&data);
  HUF_FreeFreqs(&data);

  *new_len = pak_len;

  return(pak_buffer);
}

/*----------------------------------------------------------------------------*/
void HUF_InitFreqs(huffman_data* data) {
  unsigned int i;

  data->freqs = (unsigned int *) Memory(data->max_symbols, sizeof(int));

  for (i = 0; i < data->max_symbols; i++) data->freqs[i] = 0;
}

/*----------------------------------------------------------------------------*/
void HUF_CreateFreqs(unsigned char *raw_buffer, int raw_len, unsigned int num_bits, huffman_data* data) {
  unsigned int ch, nbits;
  unsigned int i;

  for (i = 0; i < raw_len; i++) {
    ch = *raw_buffer++;
    for (nbits = 8; nbits; nbits -= num_bits) {
      data->freqs[ch >> (8 - num_bits)]++;
      ch = (ch << num_bits) & 0xFF;
    }
  }

  data->num_leafs = 0;
  for (i = 0; i < data->max_symbols; i++) if (data->freqs[i]) data->num_leafs++;

#ifdef _CUE_LOG_
  printf("\n--- CreateFreqs -------------------------------\n");
  for (i = 0; i < max_symbols; i++) {
    if (freqs[i]) printf("s:%03X w:%06X\n", i, freqs[i]);
  }
#endif

  if (data->num_leafs < 2) {
    if (data->num_leafs == 1) {
      for (i = 0; i < data->max_symbols; i++) {
        if (data->freqs[i]) {
          data->freqs[i] = 1;
          break;
        }
      }
    }

    while (data->num_leafs++ < 2) {
      for (i = 0; i < data->max_symbols; i++) {
        if (!data->freqs[i]) {
          data->freqs[i] = 2;
#ifdef _CUE_LOG_
          printf("s:%03X w:******\n", i);
#endif
          break;
        }
      }
    }
  }

  data->num_nodes = (data->num_leafs << 1) - 1;
}

/*----------------------------------------------------------------------------*/
void HUF_FreeFreqs(huffman_data* data) {
  free(data->freqs);
}

/*----------------------------------------------------------------------------*/
void HUF_InitTree(huffman_data* data) {
  unsigned int i;

  data->tree = (huffman_node **) Memory(data->num_nodes, sizeof(huffman_node *));

  for (i = 0; i < data->num_nodes; i++) data->tree[i] = NULL;
}

/*----------------------------------------------------------------------------*/
void HUF_CreateTree(huffman_data* data) {
  huffman_node *node, *lnode, *rnode;
  unsigned int  lweight, rweight, num_node;
  unsigned int  i;

  num_node = 0;
  for (i = 0; i < data->max_symbols; i++) {
    if (data->freqs[i]) {
      node = (huffman_node *) Memory(1, sizeof(huffman_node));
      data->tree[num_node++] = node;

      node->symbol = i;
      node->weight = data->freqs[i];
      node->leafs  = 1;
      node->dad    = NULL;
      node->lson   = NULL;
      node->rson   = NULL;
    }
  }

  while (num_node < data->num_nodes) {
    lnode = rnode = NULL;
    lweight = rweight = 0;

    for (i = 0; i < num_node; i++) {
      if (data->tree[i]->dad == NULL) {
        if (!lweight || (data->tree[i]->weight < lweight)) {
          rweight = lweight;
          rnode   = lnode;
          lweight = data->tree[i]->weight;
          lnode   = data->tree[i];
        } else if (!rweight || (data->tree[i]->weight < rweight)) {
          rweight = data->tree[i]->weight;
          rnode   = data->tree[i];
        }
      }
    }

    node = (huffman_node *) Memory(1, sizeof(huffman_node));
    data->tree[num_node++] = node;

    node->symbol = num_node - data->num_leafs + data->max_symbols;
    node->weight = lnode->weight + rnode->weight;
    node->leafs  = lnode->leafs + rnode->leafs;
    node->dad    = NULL;
    node->lson   = lnode;
    node->rson   = rnode;

    lnode->dad = rnode->dad = node;
  }

#ifdef _CUE_LOG_
  printf("\n--- CreateTree --------------------------------\n");
  for (i = 0; i < num_nodes; i++) {
    printf("s:%03X w:%06X n:%03X", tree[i]->symbol, tree[i]->weight, i);
    for (j = 0; j < num_nodes; j++)
      if (tree[i]->dad == tree[j]) { printf(" d:%03X", j); break; }
    if (j == num_nodes) printf(" d:---");
    for (j = 0; j < num_nodes; j++)
      if (tree[i]->lson == tree[j]) { printf(" l:%03X", j); break; }
    if (j == num_nodes) printf(" l:---");
    for (j = 0; j < num_nodes; j++)
      if (tree[i]->rson == tree[j]) { printf(" r:%03X", j); break; }
    if (j == num_nodes) printf(" r:---");
    printf(" nl:%03X", tree[i]->leafs);
    printf("\n");
  }
#endif
}

/*----------------------------------------------------------------------------*/
void HUF_FreeTree(huffman_data* data) {
  unsigned int i;

  for (i = 0; i < data->num_nodes; i++) free(data->tree[i]);
  free(data->tree);
}

/*----------------------------------------------------------------------------*/
void HUF_InitCodeTree(huffman_data* data) {
  unsigned int max_nodes;
  unsigned int i;

  max_nodes = (((data->num_leafs - 1) | 1) + 1) << 1;

  data->codetree = (unsigned char *) Memory(max_nodes, sizeof(char));
  data->codemask = (unsigned char *) Memory(max_nodes, sizeof(char));

  for (i = 0; i < max_nodes; i++) {
    data->codetree[i] = 0;
    data->codemask[i] = 0;
  }
}

/*----------------------------------------------------------------------------*/
void HUF_CreateCodeTree(huffman_data* data) {
  unsigned int i;

  i = 0;

  data->codetree[i] = (data->num_leafs - 1) | 1;
  data->codemask[i] = 0;

  HUF_CreateCodeBranch(data->tree[data->num_nodes - 1], i + 1, i + 2, data);
  HUF_UpdateCodeTree(data);

  i = (data->codetree[0] + 1) << 1;
  while (--i) if (data->codemask[i] != 0xFF) data->codetree[i] |= data->codemask[i];

#ifdef _CUE_LOG_
  unsigned char tbl[256][48];
  unsigned int  ln, li, ll, lr;
  unsigned int  rn, ri, rl, rr;
  unsigned char ls[16], rs[16], is[16], jl[16], jr[16];
  unsigned int  j, k;

  printf("\n--- CreateCodeTree ----------------------------\n");
  for (i = 0; i < num_leafs; i++) {
    li = codetree[2*i];
    ln = codemask[2*i];
    if (!i) {
      ll = 'X';
      lr = 'X';
    } else {
      li = codetree[2*i] & HUF_NEXT;
      ll = ln & HUF_LCHAR ? 'L' : '-';
      lr = ln & HUF_RCHAR ? 'R' : '-';
    }

    if ((ll == 'L') || (lr == 'R')) ln = li;
    sprintf(ls, "%c%c|%03X", ll, lr, li + i + 1);

    ri = codetree[2*i+1] & HUF_NEXT;
    rn = codemask[2*i+1];
    rl = rn & HUF_LCHAR ? 'L' : '-';
    rr = rn & HUF_RCHAR ? 'R' : '-';

    if ((rl == 'L') || (rr == 'R')) rn = ri;
    sprintf(rs, "%c%c|%03X", rl, rr, ri + i + 1);

    sprintf(tbl[i], "N:%03X L:%02X R:%02X %s %s", i, li, ri, ls, rs);

    if ((i && (li > HUF_NEXT) && (ln != 0xFF)) ||
              (ri > HUF_NEXT) && (rn != 0xFF))
      sprintf(tbl[i], "%s * WRONG CODE *", tbl[i]);


    sprintf(is, "%03X", i);
    for (j = 0; j < i; j++) {
      for (k = 0; k < 3; k++) jl[k] = tbl[j][k+0x13]; jl[k] = 0;
      for (k = 0; k < 3; k++) jr[k] = tbl[j][k+0x1A]; jr[k] = 0;

      if ((!strcmpi(is, jl) && (tbl[j][0x10] == 'L')) ||
          (!strcmpi(is, jr) && (tbl[j][0x17] == 'L'))) {
        tbl[i][0x06] = 's';
        for (k = 0; k < 6; k++) tbl[i][k+0x10] = '-';
      }

      if ((!strcmpi(is, jl) && (tbl[j][0x11] == 'R')) ||
          (!strcmpi(is, jr) && (tbl[j][0x18] == 'R'))) {
        tbl[i][0x0B] = 's';
        for (k = 0; k < 6; k++) tbl[i][k+0x17] = '-';
      }
    }
  }
  if (i + 1 != codetree[0]) sprintf(tbl[i], "N:%03X L:00 R:00", num_leafs);

  for (i = 0; i <= codetree[0]; i++) printf("%s\n", tbl[i]);
#endif
}

/*----------------------------------------------------------------------------*/
int HUF_CreateCodeBranch(huffman_node *root, unsigned int p, unsigned int q, huffman_data* data) {
  huffman_node **stack, *node;
  unsigned int  r, s, mask;
  unsigned int  l_leafs, r_leafs;

  if (root->leafs <= HUF_NEXT + 1) {
    stack = (huffman_node **) Memory(2*root->leafs, sizeof(huffman_node *));

    s = r = 0;
    stack[r++] = root;

    while (s < r) {
      if ((node = stack[s++])->leafs == 1) {
        if (s == 1) { data->codetree[p] = node->symbol; data->codemask[p]   = 0xFF; }
        else        { data->codetree[q] = node->symbol; data->codemask[q++] = 0xFF; }
      } else {
        mask = 0;
        if (node->lson->leafs == 1) mask |= HUF_LCHAR;
        if (node->rson->leafs == 1) mask |= HUF_RCHAR;

        if (s == 1) { data->codetree[p] = (r - s) >> 1; data->codemask[p]   = mask; }
        else        { data->codetree[q] = (r - s) >> 1; data->codemask[q++] = mask; }

        stack[r++] = node->lson;
        stack[r++] = node->rson;
      }
    }

    free(stack);
  } else {
    mask = 0;
    if (root->lson->leafs == 1) mask |= HUF_LCHAR;
    if (root->rson->leafs == 1) mask |= HUF_RCHAR;

    data->codetree[p] = 0; data->codemask[p] = mask;

    if (root->lson->leafs <= root->rson->leafs) {
      l_leafs = HUF_CreateCodeBranch(root->lson, q,     q + 2, data);
      r_leafs = HUF_CreateCodeBranch(root->rson, q + 1, q + (l_leafs << 1), data);
      data->codetree[q + 1] = l_leafs - 1;
    } else {
      r_leafs = HUF_CreateCodeBranch(root->rson, q + 1, q + 2, data);
      l_leafs = HUF_CreateCodeBranch(root->lson, q,     q + (r_leafs << 1), data);
      data->codetree[q] = r_leafs - 1;
    }
  }

  return(root->leafs);
}

/*----------------------------------------------------------------------------*/
void HUF_UpdateCodeTree(huffman_data* data) {
  unsigned int max, inc, n0, n1, l0, l1, tmp0, tmp1;
  unsigned int i, j, k;

  max = (data->codetree[0] + 1) << 1;

  for (i = 1; i < max; i++) {
    if ((data->codemask[i] != 0xFF) && (data->codetree[i] > HUF_NEXT)) {
      if ((i & 1) && (data->codetree[i-1] == HUF_NEXT)) {
        i--;
        inc = 1;
      } else if (!(i & 1) && (data->codetree[i+1] == HUF_NEXT)) {
        i++;
        inc = 1;
      } else {
        inc = data->codetree[i] - HUF_NEXT;
      }

      n1 = (i >> 1) + 1 + data->codetree[i];
      n0 = n1 - inc;

      l1 = n1 << 1;
      l0 = n0 << 1;

      tmp0 = *(short *)(data->codetree + l1);
      tmp1 = *(short *)(data->codemask + l1);
      for (j = l1; j > l0; j -= 2) {
        *(short *)(data->codetree + j) = *(short *)(data->codetree + j - 2);
        *(short *)(data->codemask + j) = *(short *)(data->codemask + j - 2);
      }
      *(short *)(data->codetree + l0) = tmp0;
      *(short *)(data->codemask + l0) = tmp1;

      data->codetree[i] -= inc;

      for (j = i + 1; j < l0; j++) {
        if (data->codemask[j] != 0xFF) {
          k = (j >> 1) + 1 + data->codetree[j];
          if ((k >= n0) && (k < n1)) data->codetree[j]++;
        }
      }

      if (data->codemask[l0 + 0] != 0xFF) data->codetree[l0 + 0] += inc;
      if (data->codemask[l0 + 1] != 0xFF) data->codetree[l0 + 1] += inc;

      for (j = l0 + 2; j < l1 + 2; j++) {
        if (data->codemask[j] != 0xFF) {
          k = (j >> 1) + 1 + data->codetree[j];
          if (k > n1) data->codetree[j]--;
        }
      }

      i = (i | 1) - 2;
    }
  }
}

/*----------------------------------------------------------------------------*/
void HUF_FreeCodeTree(huffman_data* data) {
  free(data->codemask);
  free(data->codetree);
}

/*----------------------------------------------------------------------------*/
void HUF_InitCodeWorks(huffman_data* data) {
  unsigned int i;

  data->codes = (huffman_code **) Memory(data->max_symbols, sizeof(huffman_code *));

  for (i = 0; i < data->max_symbols; i++) data->codes[i] = NULL;
}

/*----------------------------------------------------------------------------*/
void HUF_CreateCodeWorks(huffman_data* data) {
  huffman_node  *node;
  huffman_code  *code;
  unsigned int   symbol, nbits, maxbytes, nbit;
  unsigned char  scode[100], mask;
  unsigned int   i, j;

  for (i = 0; i < data->num_leafs; i++) {
    node   = data->tree[i];
    symbol = node->symbol;

    nbits = 0;
    while (node->dad != NULL) {
      scode[nbits++] = node->dad->lson == node ? HUF_LNODE : HUF_RNODE;
      node = node->dad;
    }
    maxbytes = (nbits + 7) >> 3;

    code = (huffman_code *) Memory(1, sizeof(huffman_code));

    data->codes[symbol]  = code;
    code->nbits    = nbits;
    code->codework = (unsigned char *) Memory(maxbytes, sizeof(char));

    for (j = 0; j < maxbytes; j++) code->codework[j] = 0;

    mask = HUF_MASK;
    j = 0;
    for (nbit = nbits; nbit; nbit--) {
      if (scode[nbit-1]) code->codework[j] |= mask;
      if (!(mask >>= HUF_SHIFT)) {
        mask = HUF_MASK;
        j++;
      }
    }
  }

#ifdef _CUE_LOG_
  printf("\n--- CreateCodeWorks ---------------------------\n");
  for (i = 0; i < num_leafs; i++) {
    code = codes[tree[i]->symbol];
    printf("s:%03X b:%02X c:", tree[i]->symbol, code->nbits);
    mask = HUF_MASK;
    j = 0;
    for (nbit = code->nbits; nbit; nbit--) {
      printf(code->codework[j] & mask ? "1" : "0");
      if (!(mask >>= HUF_SHIFT)) { mask = HUF_MASK; j++; }
    }
    printf("\n");
  }
#endif
}

/*----------------------------------------------------------------------------*/
void HUF_FreeCodeWorks(huffman_data* data) {
  unsigned int i;

  for (i = 0; i < data->max_symbols; i++) {
    if (data->codes[i] != NULL) {
      free(data->codes[i]->codework);
      free(data->codes[i]);
    }
  }
  free(data->codes);
}

/*----------------------------------------------------------------------------*/
/*--  EOF                                           Copyright (C) 2011 CUE  --*/
/*----------------------------------------------------------------------------*/
