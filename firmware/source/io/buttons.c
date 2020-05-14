/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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

#include <buttons.h>
#include <pit.h>
#include <settings.h>
#include <usb_com.h>


static uint32_t old_button_state;
volatile bool PTTLocked = false;

#if defined(PLATFORM_GD77S)
#define MBUTTON_PRESSED  0x01
#define MBUTTON_RELEASED 0x02
#define MBUTTONS_RESET   0x2A

typedef enum
{
	MBUTTON_ORANGE,
	MBUTTON_SK1,
	MBUTTON_SK2,
	MBUTTON_MAX
} MBUTTON_t;

static uint8_t mbuttons;
#endif

void buttonsInit(void)
{
    PORT_SetPinMux(Port_PTT, Pin_PTT, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_SK1, Pin_SK1, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_SK2, Pin_SK2, kPORT_MuxAsGpio);
#if ! defined(PLATFORM_RD5R)
    PORT_SetPinMux(Port_Orange, Pin_Orange, kPORT_MuxAsGpio);
#endif

    GPIO_PinInit(GPIO_PTT, Pin_PTT, &pin_config_input);
    GPIO_PinInit(GPIO_SK1, Pin_SK1, &pin_config_input);
    GPIO_PinInit(GPIO_SK2, Pin_SK2, &pin_config_input);
#if ! defined(PLATFORM_RD5R)
    GPIO_PinInit(GPIO_Orange, Pin_Orange, &pin_config_input);
#endif

#if defined(PLATFORM_GD77S)
    mbuttons = MBUTTONS_RESET;
#endif

    old_button_state = 0;
}

#if defined(PLATFORM_GD77S)
static bool isMButtonPressed(MBUTTON_t mbutton)
{
     return (((mbuttons >> (mbutton * 2)) & MBUTTON_PRESSED) & MBUTTON_PRESSED);
}

static bool isMButtonReleased(MBUTTON_t mbutton)
{
     return (((mbuttons >> (mbutton * 2)) & MBUTTON_RELEASED) & MBUTTON_RELEASED);
}

static void checkMButtonstate(MBUTTON_t mbutton)
{
	if (isMButtonReleased(mbutton) && (isMButtonPressed(mbutton) == false))
	{
		taskENTER_CRITICAL();
		timer_mbuttons[mbutton] = (nonVolatileSettings.keypadTimerLong * 1000);
		taskEXIT_CRITICAL();

		mbuttons |= (MBUTTON_PRESSED << (mbutton * 2));
		mbuttons &= ~(MBUTTON_RELEASED << (mbutton * 2));
	}
}
#endif

uint32_t buttonsRead(void)
{
	uint32_t result = BUTTON_NONE;

#if ! defined(PLATFORM_RD5R)
	if (GPIO_PinRead(GPIO_Orange, Pin_Orange) == 0)
	{
		result |= BUTTON_ORANGE;

#if defined(PLATFORM_GD77S)
		checkMButtonstate(MBUTTON_ORANGE);
#endif
	}
#endif // ! PLATFORM_RD5R

	if (GPIO_PinRead(GPIO_PTT, Pin_PTT) == 0)
	{
		result |= BUTTON_PTT;
	}

	if (GPIO_PinRead(GPIO_SK1, Pin_SK1) == 0)
	{
		result |= BUTTON_SK1;

#if defined(PLATFORM_GD77S)
		checkMButtonstate(MBUTTON_SK1);
#endif
	}

	if (GPIO_PinRead(GPIO_SK2, Pin_SK2) == 0)
	{
		result |= BUTTON_SK2;

#if defined(PLATFORM_GD77S)
		checkMButtonstate(MBUTTON_SK2);
#endif
	}

	return result;
}

#if defined(PLATFORM_GD77S)
static void checkMButtons(uint32_t *buttons, MBUTTON_t mbutton, uint32_t buttonID, uint32_t buttonLong)
{
	taskENTER_CRITICAL();
	uint32_t tmp_timer_mbutton = timer_mbuttons[mbutton];
	taskEXIT_CRITICAL();

	if ((*buttons & buttonID) && isMButtonPressed(mbutton) && (isMButtonReleased(mbutton) == false) && (tmp_timer_mbutton == 0))
	{
		// Long press
		mbuttons |= (MBUTTON_RELEASED << (mbutton * 2));
		// Set LONG bit
		*buttons |= (buttonID | buttonLong);
	}
	else if (((*buttons & buttonID) == 0) && isMButtonPressed(mbutton) && (isMButtonReleased(mbutton) == false) && (tmp_timer_mbutton != 0))
	{
		// Short press/release cycle
		mbuttons &= ~(MBUTTON_PRESSED << (mbutton * 2));
		mbuttons |= (MBUTTON_RELEASED << (mbutton * 2));

		taskENTER_CRITICAL();
		timer_mbuttons[mbutton] = 0;
		taskEXIT_CRITICAL();

		// Set SHORT press
		*buttons |= buttonID;
		*buttons &= ~buttonLong;
	}
	else if (((*buttons & buttonID) == 0) && isMButtonPressed(mbutton) && isMButtonReleased(mbutton))
	{
		// Button was still down after a long press, now handle release
		mbuttons &= ~(MBUTTON_PRESSED << (mbutton * 2));
	}
	else
	{
		// Hide Orange state, as short will happen on press/release cycle
		*buttons &= ~(buttonID | buttonLong);
	}
}
#endif

void buttonsCheckButtonsEvent(uint32_t *buttons, int *event)
{
	*buttons = buttonsRead();

#if defined(PLATFORM_GD77S)
	checkMButtons(buttons, MBUTTON_ORANGE, BUTTON_ORANGE, BUTTON_ORANGE_LONG);
	checkMButtons(buttons, MBUTTON_SK1, BUTTON_SK1, BUTTON_SK1_LONG);
	checkMButtons(buttons, MBUTTON_SK2, BUTTON_SK2, BUTTON_SK2_LONG);
#endif

	if (old_button_state != *buttons)
	{
		old_button_state = *buttons;
		*event = EVENT_BUTTON_CHANGE;
	}
	else
	{
		*event = EVENT_BUTTON_NONE;
	}
}
