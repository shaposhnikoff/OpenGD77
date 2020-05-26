/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _VOICE_PROMPTS_H_
#define _VOICE_PROMPTS_H_

enum voicePrompts {	PROMPT_0 = 0 ,PROMPT_1,PROMPT_2,PROMPT_3,PROMPT_4,PROMPT_5,PROMPT_6,PROMPT_7,PROMPT_8,PROMPT_9,
					PROMPT_A,PROMPT_B,PROMPT_C,PROMPT_D,PROMPT_E,PROMPT_F,PROMPT_G,PROMPT_H,PROMPT_I,PROMPT_J,PROMPT_K,PROMPT_L,PROMPT_M,
					PROMPT_N,PROMPT_O,PROMPT_P,PROMPT_Q,PROMPT_R,PROMPT_S,PROMPT_T,PROMPT_U,PROMPT_V,PROMPT_W,PROMPT_X,PROMPT_Y,PROMPT_Z,
					PROMPT_ZEE,
					PROMPT_BATTERY,PROMPT_CHANNEL,PROMPT_COLORCODE,PROMPT_CONTACT,PROMPT_DBM,PROMPT_DISPLAYOPTIONS,
					PROMPT_ENTRY,PROMPT_FIRMWAREINFO,PROMPT_LASTHEARD,PROMPT_MEGAHERTZ,PROMPT_OPTIONS,
					PROMPT_POINT,PROMPT_PRIVATE,PROMPT_PRIVATECALL,PROMPT_RSSI,PROMPT_TALKGROUP,PROMPT_TIMESLOT,PROMPT_VFO,
					PROMPT_SILENCE};


extern bool voicePromptIsActive;


void voicePromptsTick(void);// Called from HR-C6000.c

void voicePromptsInit(void);// Call before building the prompt sequence
void voicePromptsAppendPrompt(uint8_t prompt);// Append an individual prompt item. This can be a single letter number or a phrase
void voicePromptsAppendString(char *);// Append a text string e.g. "VK3KYY"

void voicePromptsPlay(void);// Starts prompt playback

#endif
