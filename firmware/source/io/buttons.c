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
typedef enum
{
	MBUTTON_ORANGE,
	MBUTTON_SK1,
	MBUTTON_SK2,
	MBUTTON_MAX
} MBUTTON_t;

typedef enum
{
	MBUTTON_STATE_PRESSED,
	MBUTTON_STATE_RELEASED
} MBUTTON_STATE_t;

static bool mbuttons[MBUTTON_MAX][MBUTTON_STATE_RELEASED + 1];
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
    // Reset Orange/SK1/SK2 buttons state
    for (uint8_t i = 0; i < MBUTTON_MAX; i++)
    {
    	mbuttons[i][MBUTTON_STATE_PRESSED] = false;
    	mbuttons[i][MBUTTON_STATE_RELEASED] = true;
    }
#endif

    old_button_state = 0;
}

#if defined(PLATFORM_GD77S)
static void checkMButtonstate(MBUTTON_t mbutton)
{
	if (mbuttons[mbutton][MBUTTON_STATE_RELEASED] && (mbuttons[mbutton][MBUTTON_STATE_PRESSED] == false))
	{
		taskENTER_CRITICAL();
		timer_mbuttons[mbutton] = (nonVolatileSettings.keypadTimerLong * 1000);
		taskEXIT_CRITICAL();

		mbuttons[mbutton][MBUTTON_STATE_PRESSED] = true;
		mbuttons[mbutton][MBUTTON_STATE_RELEASED] = false;
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

	if ((*buttons & buttonID) && mbuttons[mbutton][MBUTTON_STATE_PRESSED] && (mbuttons[mbutton][MBUTTON_STATE_RELEASED] == false) && (tmp_timer_mbutton == 0))
	{
		// Long press
		mbuttons[mbutton][MBUTTON_STATE_RELEASED] = true;
		// Set LONG bit
		*buttons |= (buttonID | buttonLong);
	}
	else if (((*buttons & buttonID) == 0) && mbuttons[mbutton][MBUTTON_STATE_PRESSED] && (mbuttons[mbutton][MBUTTON_STATE_RELEASED] == false) && (tmp_timer_mbutton != 0))
	{
		// Short press/release cycle
		mbuttons[mbutton][MBUTTON_STATE_PRESSED] = false;
		mbuttons[mbutton][MBUTTON_STATE_RELEASED] = true;

		taskENTER_CRITICAL();
		timer_mbuttons[mbutton] = 0;
		taskEXIT_CRITICAL();

		// Set SHORT press
		*buttons |= buttonID;
		*buttons &= ~buttonLong;
	}
	else if (((*buttons & buttonID) == 0) && mbuttons[mbutton][MBUTTON_STATE_PRESSED] && mbuttons[mbutton][MBUTTON_STATE_RELEASED])
	{
		// Button was still down after a long press, now handle release
		mbuttons[mbutton][MBUTTON_STATE_PRESSED] = false;
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
