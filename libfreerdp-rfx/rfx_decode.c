/**
 * FreeRDP: A Remote Desktop Protocol client.
 * RemoteFX Codec Library - Decode
 *
 * Copyright 2011 Vic Lee
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/utils/stream.h>
#include "rfx_types.h"
#include "rfx_rlgr.h"
#include "rfx_differential.h"
#include "rfx_quantization.h"
#include "rfx_dwt.h"

#include "rfx_decode.h"

static void rfx_decode_format_rgb(sint16* r_buf, sint16* g_buf, sint16* b_buf,
	RFX_PIXEL_FORMAT pixel_format, uint8* dst_buf)
{
	sint16* r = r_buf;
	sint16* g = g_buf;
	sint16* b = b_buf;
	uint8* dst = dst_buf;
	int i;
	
	switch (pixel_format)
	{
		case RFX_PIXEL_FORMAT_BGRA:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (uint8) (*b++);
				*dst++ = (uint8) (*g++);
				*dst++ = (uint8) (*r++);
				*dst++ = 0xFF;
			}
			break;
		case RFX_PIXEL_FORMAT_RGBA:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (uint8) (*r++);
				*dst++ = (uint8) (*g++);
				*dst++ = (uint8) (*b++);
				*dst++ = 0xFF;
			}
			break;
		case RFX_PIXEL_FORMAT_BGR:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (uint8) (*b++);
				*dst++ = (uint8) (*g++);
				*dst++ = (uint8) (*r++);
			}
			break;
		case RFX_PIXEL_FORMAT_RGB:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (uint8) (*r++);
				*dst++ = (uint8) (*g++);
				*dst++ = (uint8) (*b++);
			}
			break;
		default:
			break;
	}
}

#define MINMAX(_v,_l,_h) ((_v) < (_l) ? (_l) : ((_v) > (_h) ? (_h) : (_v)))

void rfx_decode_ycbcr_to_rgb(sint16* y_r_buf, sint16* cb_g_buf, sint16* cr_b_buf)
{
	sint16 y, cb, cr;
	sint16 r, g, b;

	int i;
	for (i = 0; i < 4096; i++)
	{
		y = y_r_buf[i] + 128;
		cb = cb_g_buf[i];
		cr = cr_b_buf[i];
		r = (y + cr + (cr >> 2) + (cr >> 3) + (cr >> 5));
		y_r_buf[i] = MINMAX(r, 0, 255);
		g = (y - ((cb >> 2) + (cb >> 4) + (cb >> 5)) - ((cr >> 1) + (cr >> 3) + (cr >> 4) + (cr >> 5)));
		cb_g_buf[i] = MINMAX(g, 0, 255);
		b = (y + cb + (cb >> 1) + (cb >> 2) + (cb >> 6));
		cr_b_buf[i] = MINMAX(b, 0, 255);
	}
}

static void rfx_decode_component(RFX_CONTEXT* context, const uint32* quantization_values,
	const uint8* data, int size, sint16* buffer)
{
	PROFILER_ENTER(context->priv->prof_rfx_decode_component);

	PROFILER_ENTER(context->priv->prof_rfx_rlgr_decode);
		rfx_rlgr_decode(context->mode, data, size, buffer, 4096);
	PROFILER_EXIT(context->priv->prof_rfx_rlgr_decode);

	PROFILER_ENTER(context->priv->prof_rfx_differential_decode);
		rfx_differential_decode(buffer + 4032, 64);
	PROFILER_EXIT(context->priv->prof_rfx_differential_decode);

	PROFILER_ENTER(context->priv->prof_rfx_quantization_decode);
		context->quantization_decode(buffer, quantization_values);
	PROFILER_EXIT(context->priv->prof_rfx_quantization_decode);

	PROFILER_ENTER(context->priv->prof_rfx_dwt_2d_decode);
		context->dwt_2d_decode(buffer, context->priv->dwt_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_dwt_2d_decode);

	PROFILER_EXIT(context->priv->prof_rfx_decode_component);
}

void rfx_decode_rgb(RFX_CONTEXT* context, STREAM* data_in,
	int y_size, const uint32 * y_quants,
	int cb_size, const uint32 * cb_quants,
	int cr_size, const uint32 * cr_quants, uint8* rgb_buffer)
{
	PROFILER_ENTER(context->priv->prof_rfx_decode_rgb);

	rfx_decode_component(context, y_quants, stream_get_tail(data_in), y_size, context->priv->y_r_buffer); /* YData */
	stream_seek(data_in, y_size);
	rfx_decode_component(context, cb_quants, stream_get_tail(data_in), cb_size, context->priv->cb_g_buffer); /* CbData */
	stream_seek(data_in, cb_size);
	rfx_decode_component(context, cr_quants, stream_get_tail(data_in), cr_size, context->priv->cr_b_buffer); /* CrData */
	stream_seek(data_in, cr_size);

	PROFILER_ENTER(context->priv->prof_rfx_decode_ycbcr_to_rgb);
		context->decode_ycbcr_to_rgb(context->priv->y_r_buffer, context->priv->cb_g_buffer, context->priv->cr_b_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_decode_ycbcr_to_rgb);

	PROFILER_ENTER(context->priv->prof_rfx_decode_format_rgb);
		rfx_decode_format_rgb(context->priv->y_r_buffer, context->priv->cb_g_buffer, context->priv->cr_b_buffer,
			context->pixel_format, rgb_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_decode_format_rgb);
	
	PROFILER_EXIT(context->priv->prof_rfx_decode_rgb);
}
