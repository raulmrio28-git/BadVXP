/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vmtimer.h"
#include "ResID.h"
#include "vm4res.h"
/* ---------------------------------------------------------------------------
* global variables
* ------------------------------------------------------------------------ */


VMINT		layer_hdl[1];	/* layer handle array. */

/* ---------------------------------------------------------------------------
 * local variables
 * ------------------------------------------------------------------------ */

unsigned char temp_unpack[240 * 320 / 8];
unsigned char frame_data[240 * 320 * 2]; //76800+5bytes

/*
* system events 
*/
void handle_sysevt(VMINT message, VMINT param);

/*
* key events 
*/
void handle_keyevt(VMINT event, VMINT keycode);

/*
* pen events
*/
void handle_penevt(VMINT event, VMINT x, VMINT y);

/*
* decoding
*/
void animate();

/*
* entry
*/

#define ANI_MAGIC '!inA'

//If you wish to debug, uncomment this
//#define DEBUGGING_LOG

/////////////////////////////////////////////////////////////
// Utilities (taken from gtrxAC's peanut.vxp)
/////////////////////////////////////////////////////////////

VMWCHAR ucs2_str[128];

void input_exit(VMINT state, VMWSTR text) {
	vm_exit_app();
}

void show_error_and_exit(const char* text) {
	vm_ascii_to_ucs2(ucs2_str, 256, (VMSTR)text);
	vm_input_text2(ucs2_str, 0, input_exit);
}

/////////////////////////////////////////////////////////////
// Utilities (taken from UstadMobile MRE examples)
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// Decoding
/////////////////////////////////////////////////////////////

VMFILE f = 0;

#define THRESHOLD	2
#define N		 4096
#define F		   18

void lzss_unpack(unsigned char* input, unsigned int insize, unsigned char* output)
{
    unsigned char status = 0;
    unsigned char s_bits = 0;
    unsigned char* c_input = input;
    unsigned char* c_inputend = input + insize;
    unsigned int olen = 0;
    unsigned char c;
    unsigned int r;
    unsigned int i, j, k;
    unsigned char text_buf[N + F - 1];
    status = *c_input++;
    s_bits = 8;
    r = N - F;
    for (i = 0; i < N - F; i++)
        text_buf[i] = 0;
    while (c_input < c_inputend)
    {
        if (status & 1)
        {
            c = *c_input++;
            output[olen++] = c; 
            text_buf[r++] = c;
            r &= (N - 1);
        }
        else
        {
            i = *c_input++;
            j = *c_input++;
            i |= ((j & 0xf0) << 4);  j = (j & 0x0f) + THRESHOLD;
            for (k = 0; k <= j; k++)
            {
                c = text_buf[(i + k) & (N - 1)];
                output[olen++] = c;
                text_buf[r++] = c;
                r &= (N - 1);
            }
        }
        status >>= 1;
        s_bits--;
        if (s_bits == 0)
        {
            status = *c_input++;
            s_bits = 8;
		}
    }
}

void decode_img(unsigned char* in, unsigned int inlen, unsigned short* out, unsigned int outlen)
{
    unsigned char magic;
    unsigned char bits_per_run;
	unsigned char is_redundant;
    unsigned char starting_pixel;
    unsigned short len, len_red;
    unsigned char* what_to_unpack;
	magic = in[4];
	bits_per_run = magic>>2;
	is_redundant = magic>>1 & 1;
	starting_pixel = magic & 1;
	len = in[0] | (in[1] << 8);
    if (is_redundant)
		len_red = in[2] | (in[3] << 8);
	if (is_redundant)
    {
        what_to_unpack = temp_unpack;
        lzss_unpack(in + 5, len_red, temp_unpack);
    }
    else
        what_to_unpack = in+5;
    {
		unsigned int* cdata = (unsigned int*)what_to_unpack;
        unsigned int rbits = 32;
        unsigned int rdata = *cdata;
		unsigned int outpos = 0;
        unsigned int outbit = 8;
		unsigned int outbyte = 0;
        while (outpos < outlen)
        {
            int rle_count = 0;
			int i;
            if (rbits == 0)
            {
                cdata++;
                rdata = *cdata;
                rle_count = rdata >> (32 - bits_per_run);
                rdata <<= bits_per_run;
                rbits = 32 - bits_per_run;
            }
            else if (rbits < bits_per_run)
            {
                rle_count = rdata >> (32 - bits_per_run);
                cdata++;
                rdata = *cdata;
                rle_count |= rdata >> (32 - (bits_per_run - rbits));
                rdata <<= bits_per_run - rbits;
                rbits = 32 - (bits_per_run - rbits);
            }
            else
            {
                rle_count = rdata >> (32 - bits_per_run);
                rdata <<= bits_per_run;
                rbits -= bits_per_run;
            }

            for (i = 0; i < rle_count && outpos < outlen; ++i)
				out[outpos++] = (starting_pixel==1?0xffff:0x0000);
			if (outpos >= outlen)
				break;
            starting_pixel = !starting_pixel;
        }
    }
}
int drv;

void vm_main(void) {
	/* initialize layer handle */
	layer_hdl[0] = -1;	
	
	/* register system events handler */
	vm_reg_sysevt_callback(handle_sysevt);

	/* Init MRE resource */
	vm_res_init();

	if ((drv = vm_get_removable_driver()) < 0)	
	{	
		/* if no removable drive then get system drive*/ 
		drv = vm_get_system_driver ();
	}

	animate();
}

void handle_sysevt(VMINT message, VMINT param) {
	switch (message) {
	case VM_MSG_CREATE:	/* the message of creation of application */
	case VM_MSG_ACTIVE: /* the message of application state from inactive to active */
		/*cerate base layer that has same size as the screen*/
		layer_hdl[0] = vm_graphic_create_layer(0, 0, 
			vm_graphic_get_screen_width(),		/* get screen width */
			vm_graphic_get_screen_height(),		/* get screen height */
			-1);		/* -1 means layer or canvas have no transparent color */
		
		/* set clip area*/
		vm_graphic_set_clip(0, 0, 
			vm_graphic_get_screen_width(), 
			vm_graphic_get_screen_height());
		break;
		
	case VM_MSG_PAINT:	/* the message of asking for application to repaint screen */
		break;
		
	case VM_MSG_INACTIVE:	/* the message of application state from active to inactive */
		if( layer_hdl[0] != -1 )
			vm_graphic_delete_layer(layer_hdl[0]);
		
		break;	
	case VM_MSG_QUIT:		/* the message of quit application */
		if( layer_hdl[0] != -1 )
			vm_graphic_delete_layer(layer_hdl[0]);
		
		/* Release all resource */
		vm_res_deinit();
		break;	
	}
}

int frames = 0;
int curr_frame = 0;
int curr_parsed = 20;
VMINT timer = 0;

#ifdef DEBUGGING_LOG
VMFILE log_file = 0;
#endif

void decode_and_draw_frame(VMINT tid)
{
	unsigned char magic;
	unsigned int frame_len = 0;
	VMUINT red = 0;
	if (curr_frame >= frames)
	{
		vm_delete_timer_ex(timer);
		vm_file_close(f);
		show_error_and_exit("Thank you for enjoying!");
		return;
	}
	memset(frame_data,0,240*320*2);
	vm_file_read(f, frame_data, 5, &red);
	magic = frame_data[4];
    frame_len = (frame_data[0] | (frame_data[1] << 8) | (frame_data[2] << 16) | (frame_data[3] << 24));
	vm_file_read(f, frame_data+5, frame_len, &red);
	#ifdef DEBUGGING_LOG
	{
		char a[128];
		VMUINT w;
		sprintf(a, "Decoding frame %d located in file at offset %d\n", curr_frame+1, curr_parsed);
		vm_file_write(log_file, a, strlen(a), &w);
	}
	#endif
	curr_parsed += 5+frame_len;
    decode_img(frame_data, frame_len+5, (unsigned short*)vm_graphic_get_layer_buffer(layer_hdl[0]), 240*320);
    vm_graphic_flush_layer(layer_hdl, 1);
	curr_frame++;
	//Due to timer issues, we need to artificially slowdown
	{
		int a;
		for (a=0;a<100;a++) {0;}
	}
}

void animate()
{
	//unsigned char* image = (unsigned char *)_acani;
	char path[128] = " :\\BadApple\\ani.ani";
	char header[20];
	VMUINT red = 0;
	path[0] = drv;
	vm_ascii_to_ucs2(ucs2_str, 256, " :\\log.txt");
	ucs2_str[0] = drv;
	#ifdef DEBUGGING_LOG
	log_file = vm_file_open(ucs2_str, MODE_CREATE_ALWAYS_WRITE, 0);
	#endif
	vm_ascii_to_ucs2(ucs2_str, 256, (VMSTR)path);
	f = vm_file_open(ucs2_str, MODE_READ, 1);
	if (f < 0) {
		show_error_and_exit("Frames not found");
		return;
	}
	vm_file_read(f, header, 20, &red);
    if (*((unsigned int*)header) != ANI_MAGIC)
    {
		show_error_and_exit("Header not correct");
		return;
	}
	frames = *((unsigned int*)&header[16]);
	timer = vm_create_timer(33, decode_and_draw_frame);
}
