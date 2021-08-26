/* DEMO(for PC Engine)  by takuya matsubara*/
/* https://sites.google.com/site/yugenkaisyanico/diy-pce-card */

#include "huc.h"

#define VRAM_MAP     0x1000
#define VRAM_PANEL   0x2000
#define VRAM_BAT2    0x0600
#define VRAM_SPRITE  0x6F00

#incbin(sp_pattern,"sp_pattern.bin");
#incbin(sp_palette,"sp_palette.bin");
#incbin(panel_pattern, "bg_pattern.bin")
#incbin(panel_pal, "bg_palette.bin")
#incbin(panel_bat, "bg_bat.bin")
#incbin(map_pattern, "map_pattern.bin")
#incbin(map_pal, "map_palette.bin")
#incbin(demo_map,"mapdata.bin")

#define SPMAX 64

#define MAP_PAL      0
#define PANEL_PAL    2

#define MAP_WIDTH  48
#define MAP_HEIGHT 12

const char map_pal_ref[MAP_HEIGHT] = {
	MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4,
	MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4,
	MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4, MAP_PAL<<4
};

/*waveform  (based by "huc-3.21-win\doc\pce\psg.txt" )*/
const char waveform[]={
	18,22,24,26,28,28,30,30,30,30,28,28,26,24,22,18,
	12, 8, 6, 4, 2, 2, 0, 0, 0, 0, 2, 2, 4, 6, 8,12
};

/*Frequency Table((3.8MHz / 32)/Freq.)*/
const int freqtbl[]={
	428,	/* note60	262Hz */
	404,	/* note61	277Hz */
	381,	/* note62	294Hz */
	360,	/* note63	311Hz */
	339,	/* note64	330Hz */
	320,	/* note65	349Hz */
	302,	/* note66	370Hz */
	285,	/* note67	392Hz */
	269,	/* note68	415Hz */
	254,	/* note69	440Hz */
	240,	/* note70	466Hz */
	227	/* note71	494Hz */
};

char *psgch;    /*Channel*/
char *psgmvol;  /*Master Volume*/
char *psgfreql; /*Frequency L*/
char *psgfreqh; /*Frequency H*/
char *psgctrl;  /*control*/
char *psglrvol; /*L/R Volume*/
char *psgdata;  /*Waveform*/

char wave_cnt;

int sx,map_x;
int score;
char x[SPMAX],y[SPMAX],x1[SPMAX],y1[SPMAX];

/**/
wave_init()
{
	int i;

	psgch   = 0x800;
	psgmvol = 0x801;
	psgfreql= 0x802;
	psgfreqh= 0x803;
	psgctrl = 0x804;
	psglrvol= 0x805;
	psgdata = 0x806;

	vsync();

	*psgch = 0;		/*Channel*/

	*psgctrl = (1<<6); 	/*reset*/
	*psgctrl = (0<<6); 	/*waveform transfer*/
	for(i=0;i<32;i++){
		*psgdata = waveform[i];	/*Waveform*/
	}
	*psgmvol = 0xee;	/*Master Volume*/
	wave_cnt = 0;
}

/**/
wave_play(note, volume, length)
char note, volume, length;
{
	int reg;

	reg = freqtbl[note % 12];

	note -= 60;
	while(note < 0){
		note += 12;
		reg <<= 1;
	}
	while(note >= 12){
		note -= 12;
		reg >>= 1;
	}

	*psgfreql = reg & 0xff;	/*Frequency L*/
	*psgfreqh = reg >> 8;	/*Frequency H*/

	*psglrvol = volume;	/*L/R Volume*/
	*psgctrl = (1<<7) + 0x1f; /*Channel on(volume=31)*/

	wave_cnt = length;
}
/**/
wave_driver()
{
	if(wave_cnt > 0){
		wave_cnt--;
		if(wave_cnt==0){
/*			*psglrvol = 0;*/
			*psgctrl = (0<<7) + 0x0;
		}
	}
}
/**/
main()
{
	char test,note,volume;
	char newx,newy;
	int num;

	disp_off();
	cls();
	wave_init();

	load_vram(VRAM_SPRITE,sp_pattern,64*64);
	init_satb();
	for(num=0; num<SPMAX; num++){
		spr_set(num);
		spr_pattern(VRAM_SPRITE+(64*num));
		spr_ctrl(SIZE_MAS|FLIP_MAS, SZ_16x16|NO_FLIP);
		spr_pal(0);
		spr_pri(1);	/*Priority*/
		x[num]=16+(num % 14)*16;
		y[num]=16+(num / 14)*16;
		x1[num]=(num % 7);
		y1[num]=(num % 5);
		if((x1[num]==0)&&(y1[num]==0))y1[num]=1;
	}

	set_map_data(demo_map, MAP_WIDTH, MAP_HEIGHT);
	set_tile_data(map_pattern, MAP_HEIGHT, map_pal_ref);
	load_tile(VRAM_MAP);
	load_palette(MAP_PAL, map_pal, 1);
	load_map(0, 0, 0, 0, 34/2, 24/2);
	set_font_pal(PANEL_PAL);

	load_vram(VRAM_PANEL, panel_pattern, 32*4*16);
	load_bat(VRAM_BAT2, panel_bat, 32, 4);
	load_palette(PANEL_PAL, panel_pal, 1);

	put_string("SCROLL X:", 1, 25);
	put_string("PSG NOTE:0x",1,26);
	put_string("PSG VOL :0x",1,27);
	test = 0;

	scroll(0, 0,    0,    0, (24*8)-1, 0xC0);
	scroll(1, 0, 24*8, 24*8, (28*8)-1, 0x80);

	vsync();
	set_sprpal(0, sp_palette);

	disp_on();

	sx = 0;
	map_x = 0;

	while(1){
		for(num=0; num<SPMAX; num++){
			spr_set(num);
			newx = x[num]+x1[num];
			newy = y[num]+y1[num];
			if((newx<=8)||(newx>=248))
				x1[num] *= -1;

			if((newy<=8)||(newy>=232))
				y1[num] *= -1;

			spr_x(newx);
			spr_y(newy);
			x[num]=newx;
			y[num]=newy;
		}

		if(wave_cnt==0){
			note = 60+(test % 12);
			if(test < 12){
				volume = 0xff;
			}else if(test < 24){
				volume = 0x0f;
			}else{
				volume = 0xf0;
			}
			put_hex(note  , 2, 12,26);
			put_hex(volume, 2, 12,27);
			wave_play(note, volume, 100);
			test++;
			if(test >= 12*3) test=0;
		}

		sx++;
		if((sx & 0xF) == 0) {
			map_x ++;
			if (map_x  >= MAP_WIDTH){
				map_x -= MAP_WIDTH;
			}
			load_map(
			  (sx + 256) >> 4,0, /* sx,sy */
			  map_x + (32/2), 0, /* map x,y */
			  (2/2), (24/2));    /* w,h */

			score += 1;
		}
		put_number(sx, 3, 8, 25);

		scroll(0, sx, 0, 0, ((24*8)-1), 0xC0);
		satb_update();

		wave_driver();
		vsync();
	}
}

