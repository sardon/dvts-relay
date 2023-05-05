#ifndef _LIB_DV_H_
#define _LIB_DV_H_

#include <sys/types.h>

#define		DV625_50_DSEQ_NUM		12
#define		DV625_50_DIFBLOCK_NUM		150

#define		DV525_60_DSEQ_NUM		10
#define		DV525_60_DIFBLOCK_NUM		150

/* width and height of Super Block */
#define		DV525_60_SB_WIDTH		5
#define		DV525_60_SB_HEIGHT		10

#define		DV_MACROBLOCK_NUM		27

#define		DV625_50_SB_WIDTH		5
#define		DV625_50_SB_HEIGHT		12

#define		DV525_60_WIDTH			720
#define		DV525_60_HEIGHT			480

#define		DV625_50_WIDTH			720
#define		DV625_50_HEIGHT			576

#define		SCT_HEADER		0x0
#define		SCT_SUBCODE		0x1
#define		SCT_VAUX		0x2
#define		SCT_AUDIO		0x3
#define		SCT_VIDEO		0x4

struct dv_difblock {
  u_int32_t data[80/sizeof(u_int32_t)];
};

struct dv_difseq {
  struct dv_difblock header[1];
  struct dv_difblock subcode[2];
  struct dv_difblock vaux[3];

  struct dv_difblock audio[9];
  struct dv_difblock video[135];
};

struct dv_rawdata525_60 {
  struct dv_difseq dseq[DV525_60_DSEQ_NUM];
};

struct dv_rawdata625_50 {
  struct dv_difseq dseq[DV625_50_DSEQ_NUM];
};

struct dv_sct {
  u_int32_t	seq:4;
  u_int32_t	rsv0:1;
  u_int32_t	sct:3;

  u_int32_t	rsv1:4;

  u_int32_t	dseq:4;

  u_int32_t	dbn:8;
  u_int32_t	padding:8;
};

struct macro_block {
  int16_t pix[6][8][8];
};

struct super_block {
  struct macro_block m[DV_MACROBLOCK_NUM];
};

#define		LIBDV_FLAG_COLOR		0x00000001
#define		LIBDV_FLAG_GREY_SCALE		0x00000002

struct pixdata525_60 {
  struct super_block s[DV525_60_DSEQ_NUM][5];
};

struct pixdata625_50 {
  struct super_block s[DV625_50_DSEQ_NUM][5];
};

struct rgb_data525_60 {
  int16_t r[DV525_60_WIDTH][DV525_60_HEIGHT];
  int16_t g[DV525_60_WIDTH][DV525_60_HEIGHT];
  int16_t b[DV525_60_WIDTH][DV525_60_HEIGHT];
};

struct rgb_data625_50 {
  int16_t r[DV625_50_WIDTH][DV625_50_HEIGHT];
  int16_t g[DV625_50_WIDTH][DV625_50_HEIGHT];
  int16_t b[DV625_50_WIDTH][DV625_50_HEIGHT];
};

struct xy {
  int16_t x;
  int16_t y;
};

__BEGIN_DECLS
//void dv_init __P((void));
int dv_525_60_decode __P((const struct dv_rawdata525_60 *, struct pixdata525_60 *));
int dv_525_60_encode __P((const struct pixdata525_60 *, struct dv_rawdata525_60 *));
void dv_525_60_ycrcb2rgb __P((struct pixdata525_60 *, struct rgb_data525_60 *));

int dv_625_50_decode __P((const struct dv_rawdata625_50 *, struct pixdata625_50 *));
int dv_625_50_encode __P((const struct pixdata625_50 *, struct dv_rawdata625_50 *));
void dv_625_50_ycrcb2rgb __P((struct pixdata625_50 *, struct rgb_data625_50 *));
__END_DECLS
#endif /* _LIB_DV_H_ */
