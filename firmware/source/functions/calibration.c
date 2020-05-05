/*
 * Copyright (C)2019 	Roger Clark, VK3KYY / G4KYF
 * 				and 	Kai Ludwig, DG4KLU
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

#include <calibration.h>
#include <trx.h>


static const uint32_t CALIBRATION_BASE 				= 0xF000;

static const uint32_t EXT_DACDATA_shift 			= 0x00005D;
static const uint32_t EXT_twopoint_mod  			= 0x000008;
static const uint32_t EXT_Q_MOD2_offset 			= 0x00000A;
static const uint32_t EXT_phase_reduce  			= 0x000055;

static const uint32_t EXT_pga_gain      			= 0x000065;
static const uint32_t EXT_voice_gain_tx 			= 0x000066;
static const uint32_t EXT_gain_tx       			= 0x000067;
static const uint32_t EXT_padrv_ibit    			= 0x000064;

static const uint32_t EXT_xmitter_dev_wideband		= 0x000068;
static const uint32_t EXT_xmitter_dev_narrowband	= 0x00006A;

static const uint32_t EXT_dac_vgain_analog 			= 0x00006C;
static const uint32_t EXT_volume_analog    			= 0x00006D;

static const uint32_t EXT_noise1_th_wideband   		= 0x000047;
static const uint32_t EXT_noise2_th_wideband   		= 0x000049;
static const uint32_t EXT_rssi3_th_wideband    		= 0x00004b;
static const uint32_t EXT_noise1_th_narrowband 		= 0x00004d;
static const uint32_t EXT_noise2_th_narrowband 		= 0x00004f;
static const uint32_t EXT_rssi3_th_narrowband  		= 0x000051;

static const uint32_t EXT_squelch_th 				= 0x00003f;

static const uint32_t EXT_uhf_dev_tone				= 0x00005E;
static const uint32_t EXT_vhf_dev_tone				= 0x0000CE;

static const uint32_t POWER_ADDRESS_UHF_400MHZ 		= 0x00000B; //UHF is in 5Mhz bands starting at 400Mhz.
static const uint32_t POWER_ADDRESS_VHF_135MHZ 		= 0x00007B; //VHF is in 5Mhz bands from 135Mhz (Only first 8 entries are used)

#define CALIBRATION_TABLE_LENGTH 0xE0 // Calibration table is 224 bytes long
static __attribute__((section(".data.$RAM4"))) uint8_t calibrationData[CALIBRATION_TABLE_LENGTH];

bool calibrationInit()
{
	return (SPI_Flash_read(CALIBRATION_BASE, &calibrationData[0], CALIBRATION_TABLE_LENGTH));
}

/*
 * Sent to HRC6000 register 0x37. This sets the DMR received Audio Gain. (And Beep Volume)
 * Effective for DMR Receive Only.
 * only the low 4 bits are used. 1.5dB change per increment 0-331
 */
void read_val_DACDATA_shift(int offset, uint8_t* val_shift)
{
	*val_shift = calibrationData[EXT_DACDATA_shift + offset] + 1;

	if (*val_shift > 31)
	{
		*val_shift = 31;
	}
	*val_shift |= 0x80;
}
/*
 * Sent to HRC6000 register 0x47 and 0x48 to set the DC level of the Mod1 output. This sets the frequency of the AT1846 Reference Oscillator.
 * Effective for DMR and FM Receive and Transmit. Calibrates the receive and transmit frequencies.
 * Only one setting should be required for all frequencies but the Cal table has separate entries for VHF and UHF.
 */
void read_val_twopoint_mod(int offset, uint8_t* val_0x47, uint8_t* val_0x48)
{
	*val_0x47 = calibrationData[EXT_twopoint_mod + offset + 0];
	*val_0x48 = calibrationData[EXT_twopoint_mod + offset + 1];
}

/*
 *
 * Sent to the HRC6000 register 0x04 which is the offset value for the Mod 2 output.
 * However according to the schematics the Mod 2 output is not connected.
 * Therefore the function of this setting is unclear. (possible datasheet error?)
 *
 */
void read_val_Q_MOD2_offset(int offset, uint8_t* val_0x04)
{
	//The Cal table has one entry for each band and this is almost identical to 0x47 above.
	*val_0x04 = calibrationData[EXT_Q_MOD2_offset + offset];
}
/*
 * Sent to the HRC6000 register 0x46. This adjusts the level of the DMR Modulation on the MOD1 output.
 * Mod 1 controls the AT1846 Reference oscillator so this is effectively the deviation setting for DMR.
 * Because it modulates the reference oscillator the deviation will change depending on the frequency being used.
 *
 * Only affects DMR Transmit. The Cal table has 8 calibration values for each band.
 */
void read_val_phase_reduce(int offset, uint8_t* val_0x46)
{
	*val_0x46 = calibrationData[EXT_phase_reduce + offset];
}
/*
 * Sent to AT1846S register 0x0a bits 6-10. Sets Voice Analogue Gain for FM Transmit.
 */
void read_val_pga_gain(int offset, uint8_t* value)
{

	*value = calibrationData[EXT_pga_gain + offset] & 0x1f;
}
/*
 * Sent to AT1846S register 0x41 bits 0-6. Sets Voice Digital Gain for FM Transmit.
 */
void read_val_voice_gain_tx(int offset, uint8_t* value)
{
	*value = calibrationData[EXT_voice_gain_tx + offset] & 0x7f;
}
/*
 * Sent to AT1846S register 0x44 bits 8-11. Sets Voice Digital Gain after ADC for FM Transmit.
 */
void read_val_gain_tx(int offset, uint8_t* value)
{
	*value = calibrationData[EXT_gain_tx + offset] & 0x0f;
}
/*
 * Sent to AT1846S register 0x0a bits 11-14. Sets PA Power Control for DMR and FM Transmit
 */
void read_val_padrv_ibit(int offset, uint8_t* value)
{
	*value = calibrationData[EXT_padrv_ibit + offset] & 0x0f;
}

/*
 * Sent to AT1846S register 0x59 bits 6-15. Sets Deviation for Wideband FM Transmit
 */
void read_val_xmitter_dev_wideband(int offset, uint16_t* value)
{
	*value = calibrationData[EXT_xmitter_dev_wideband + offset] + ((calibrationData[EXT_xmitter_dev_wideband + offset + 1] & 0x03) << 8);
}
/*
 * Sent to AT1846S register 0x59 bits 6-15. Sets Deviation for NarrowBand FM Transmit
 */
void read_val_xmitter_dev_narrowband(int offset, uint16_t* value)
{
	*value = calibrationData[EXT_xmitter_dev_narrowband + offset + 0] + ((calibrationData[EXT_xmitter_dev_narrowband + offset + 1] & 0x03) << 8);
}

/*
 * Sent to AT1846S register 0x59 bits 0-5. Sets Tones Deviation for FM Transmit
 */
void read_val_dev_tone(int index, uint8_t *value)
{
	int address;

	if (trxCurrentBand[TRX_TX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		address = EXT_uhf_dev_tone + index;
	}
	else
	{
		address = EXT_vhf_dev_tone + index;
	}

	*value = calibrationData[address];
}
/*
 * Sets Digital Audio Gain for FM and DMR Receive.
 * Sent to AT1846S register 0x44 bits 0-3.
 */
void read_val_dac_vgain_analog(int offset, uint8_t* value)
{
	*value = calibrationData[EXT_dac_vgain_analog + offset] & 0x0f;
}
/*
 * Sent to AT1846S register 0x44 bits 4-7.
 * Sets Analogue Audio Gain for FM and DMR Receive.
 */
void read_val_volume_analog(int offset, uint8_t* value)
{
	*value = calibrationData[EXT_volume_analog + offset] & 0x0f;
}
/*
 * Sent to AT1846S register 0x48. Sets Noise 1 Threshold  for FM Wideband Receive.
 */
void read_val_noise1_th_wideband(int offset, uint16_t* value)
{
	*value = ((calibrationData[EXT_noise1_th_wideband + offset] & 0x7f) << 7) + ((calibrationData[EXT_noise1_th_wideband + offset + 1] & 0x7f) << 0);
}
/*
 * Sent to AT1846S register 0x60. Sets Noise 2 Threshold  for FM  Wideband Receive.
 */
void read_val_noise2_th_wideband(int offset, uint16_t* value)
{
	*value = ((calibrationData[EXT_noise2_th_wideband + offset] & 0x7f) << 7) + ((calibrationData[EXT_noise2_th_wideband + offset + 1] & 0x7f) << 0);
}
/*
 * Sent to AT1846S register 0x3F. Sets RSSI3 Threshold  for  Wideband Receive.
 */
void read_val_rssi3_th_wideband(int offset, uint16_t* value)
{
	*value = ((calibrationData[EXT_rssi3_th_wideband + offset] & 0x7f) << 7) + ((calibrationData[EXT_rssi3_th_wideband + offset + 1] & 0x7f) << 0);
}

/*
 * Sent to AT1846S register 0x48. Sets Noise 1 Threshold  for FM Narrowband Receive.
 */
void read_val_noise1_th_narrowband(int offset, uint16_t* value)
{
	*value = ((calibrationData[EXT_noise1_th_narrowband + offset] & 0x7f) << 7) + ((calibrationData[EXT_noise1_th_narrowband + offset + 1] & 0x7f) << 0);
}

/*
 * Sent to AT1846S register 0x60. Sets Noise 2 Threshold  for FM  Narrowband  Receive.
 */
void read_val_noise2_th_narrowband(int offset, uint16_t* value)				//
{
	*value = ((calibrationData[EXT_noise2_th_narrowband + offset] & 0x7f) << 7) + ((calibrationData[EXT_noise2_th_narrowband + offset + 1] & 0x7f) << 0);
}

/*
 * Sent to AT1846S register 0x3F. Sets RSSI3 Threshold  for  Narrowband Receive.
 */
void read_val_rssi3_th_narrowband(int offset, uint16_t* value)				//
{
	*value = ((calibrationData[EXT_rssi3_th_narrowband + offset] & 0x7f) << 7) + ((calibrationData[EXT_rssi3_th_narrowband + offset + 1] & 0x7f) << 0);
}

/*
 * THE DESCRIPTION OF THIS FUNCTION IS WRONG
 *
 * Sent to At1846S register 0x49. sets squelch open threshold for FM Receive.
 * 8 Cal Table Entries for each band. Frequency Dependent.
 */
void read_val_squelch_th(int offset, int mod, uint16_t* value)
{
	uint8_t v1 = calibrationData[EXT_squelch_th + offset] - mod;
	uint8_t v2 = calibrationData[EXT_squelch_th + offset] - mod - 3;

	if ((v1 >= 127) || (v2 >= 127) || (v1 < v2))
	{
	  v1 = 24;
	  v2 = 21;
	}
	*value = (v1 << 7) + (v2 << 0);
}
/*
 * Power Settings for Transmit
 * 16 Entries of two bytes each.
 * First byte of each entry  is the low power and the second byte is the high power value
 * On the GD-77 and DM-1801, this value is 1/16 of the required DAC value.
 * On the RD-5R, its currently unclear how the power calibration or even the power control works.
 * So a lower multiplier has been used, to prevent the PA being overdriven.
 */
bool calibrationGetPowerForFrequency(int freq, calibrationPowerValues_t *powerSettings)
{
	int address;
	//uint8_t buffer[2];

	if (trxCurrentBand[TRX_TX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		address = (freq - RADIO_FREQUENCY_BANDS[RADIO_BAND_UHF].calTableMinFreq) / 500000;

		if (address < 0)
		{
			address = 0;
		}
		else
		{
			if (address > 15)
			{
				address = 15;
			}
		}
		address = POWER_ADDRESS_UHF_400MHZ + (address * 2);

	}
	else
	{
		address = (freq - RADIO_FREQUENCY_BANDS[RADIO_BAND_VHF].calTableMinFreq) / 500000;

		if (address < 0)
		{
			address = 0;
		}
		else
		{
			if (address > 7)
			{
				address = 7;
			}
		}
		address = POWER_ADDRESS_VHF_135MHZ + (address * 2);
	}


#if defined(PLATFORM_RD5R)
	const int POWER_CAL_MULTIPLIER = 8;
#else
	const int POWER_CAL_MULTIPLIER = 16;
#endif

	powerSettings->lowPower 	= calibrationData[address] 		* POWER_CAL_MULTIPLIER;
	powerSettings->highPower 	= calibrationData[address + 1] 	* POWER_CAL_MULTIPLIER;

	return true;
}

/*
 * Signal Level Meter setting.
 *
 * This is not currently used.
 *
 * Two bytes per band
 * Low Byte is low end of meter range
 * High Byte is High End of Meter Range
 */
bool calibrationGetRSSIMeterParams(calibrationRSSIMeter_t *rssiMeterValues)
{
	int address;

	if (trxCurrentBand[TRX_RX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		address = 0x0053;
	}
	else
	{
		address = 0x0C3;
	}

	memcpy((uint8_t *)rssiMeterValues, (&calibrationData[0] + address), 2);

	return true;
}

/*
 * This function is used to check if the calibration table has been copied to the common location
 * 0xF000 and if not, to copy it to that location in the external Flash memory
 */
bool checkAndCopyCalibrationToCommonLocation(void)
{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

const uint32_t VARIANT_CALIBRATION_BASE 				= 0x0008F000;
const int MARKER_BYTES_LENGTH = 8;
const uint8_t MARKER_BYTES[] = {0xA0 ,0x0F ,0xC0 ,0x12 ,0xA0 ,0x0F ,0xC0 ,0x12};

#elif defined(PLATFORM_DM1801)

const uint32_t VARIANT_CALIBRATION_BASE 				= 0x0006F000;
const int MARKER_BYTES_LENGTH = 2;
const uint8_t MARKER_BYTES[] = {0xA0 ,0x0F};// DM-1801 only seems to consistently have the first 2 bytes the same.

#elif defined(PLATFORM_RD5R)

const uint32_t VARIANT_CALIBRATION_BASE = 0x00017c00;	// Found by marker bytes matching
const int MARKER_BYTES_LENGTH = 8;
const uint8_t MARKER_BYTES[] = {0xA0 ,0x0F ,0xC0 ,0x12 ,0xA0 ,0x0F ,0xC0 ,0x12};

#endif

	uint8_t tmp[CALIBRATION_TABLE_LENGTH];

	if (SPI_Flash_read(CALIBRATION_BASE, (uint8_t *)tmp, MARKER_BYTES_LENGTH))
	{
		if (memcmp(MARKER_BYTES, tmp, MARKER_BYTES_LENGTH) == 0)
		{
			// found calibration table in common location.
			return true;
		}
		// Need to copy the calibration

		if (SPI_Flash_read(VARIANT_CALIBRATION_BASE, (uint8_t *)tmp, CALIBRATION_TABLE_LENGTH))
		{
			if (memcmp(MARKER_BYTES, tmp, MARKER_BYTES_LENGTH) == 0)
			{
				// found calibration table in variant location.
				if (SPI_Flash_write(CALIBRATION_BASE, tmp, CALIBRATION_TABLE_LENGTH))
				{
					// Update current calibration data with the calibration data that just been read
					memcpy(&calibrationData[0], tmp, CALIBRATION_TABLE_LENGTH);

					return true;
				}
			}
		}
	}

	return false; // Something went wrong!
}
