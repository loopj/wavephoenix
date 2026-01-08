/***************************************************************************//**
 * @brief RAIL Configuration
 * @details
 *   WARNING: Auto-Generated Radio Config  -  DO NOT EDIT
 *   Radio Configurator Version: 2304.5.2
 *   RAIL Adapter Version: 2.4.33
 *   RAIL Compatibility: 2.x
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_device.h"
#include "rail_config.h"

uint32_t RAILCb_CalcSymbolRate(RAIL_Handle_t railHandle)
{
  (void) railHandle;
  return 0U;
}

uint32_t RAILCb_CalcBitRate(RAIL_Handle_t railHandle)
{
  (void) railHandle;
  return 0U;
}

void RAILCb_ConfigFrameTypeLength(RAIL_Handle_t railHandle,
                                  const RAIL_FrameType_t *frameType)
{
  (void) railHandle;
  (void) frameType;
}

static const uint8_t irCalConfig[] = {
  25, 63, 1, 6, 4, 16, 1, 0, 0, 1, 1, 6, 0, 16, 39, 0, 0, 5, 0, 1, 1, 0, 0, 0, 0, 0
};

static const int32_t timingConfig[] = {
  0, 0, 0
};

static const uint8_t hfxoRetimingConfigEntries[] = {
  2, 0, 0, 0, 0x00, 0xf0, 0x49, 0x02, 6, 20, 0, 0, 0x00, 0xe0, 0x93, 0x04, 5, 56, 0, 0, 0xa0, 0x08, 0, 0, 0, 0, 0x58, 0x09, 1, 4, 7, 6, 0x10, 0x0a, 1, 4, 7, 7, 0xc8, 0x0a, 0, 4, 8, 7, 0x80, 0x0b, 0, 4, 8, 8, 0x38, 0x0c, 0, 4, 9, 8, 0x61, 0x08, 0, 0, 0, 0, 0x68, 0x08, 0, 0, 0, 0, 0xc7, 0x09, 1, 4, 4, 3, 0x2c, 0x0b, 1, 4, 4, 4, 0x92, 0x0c, 1, 4, 5, 4
};

static RAIL_ChannelConfigEntryAttr_t channelConfigEntryAttr = {
#if RAIL_SUPPORTS_OFDM_PA
  {
    { 0xFFFFFFFFUL },
    { 0xFFFFFFFFUL, 0xFFFFFFFFUL }
  }
#else // RAIL_SUPPORTS_OFDM_PA
  { 0xFFFFFFFFUL },
#endif // RAIL_SUPPORTS_OFDM_PA
};

static const uint32_t phyInfo[] = {
  16UL,
  0x00888888UL, // 136.53333333333333
  (uint32_t) NULL,
  (uint32_t) irCalConfig,
  (uint32_t) timingConfig,
  0x00000000UL,
  10800000UL,
  34560000UL,
  1440000UL,
  0x00F50F01UL,
  0x02103A3DUL,
  (uint32_t) NULL,
  (uint32_t) hfxoRetimingConfigEntries,
  (uint32_t) NULL,
  0UL,
  0UL,
  1440010UL,
  (uint32_t) NULL,
  (uint32_t) NULL,
  (uint32_t) NULL,
};

const uint32_t WaveBird_modemConfigBase[] = {
  0x0002400CUL, 0x00000000UL,
  /*    4010 */ 0x00004000UL,
  0x00024020UL, 0x00000012UL,
  /*    4024 */ 0x00000000UL,
  0x00074030UL, 0x00000000UL,
  /*    4034 */ 0x00000000UL,
  /*    4038 */ 0x00000000UL,
  /*    403C */ 0x00000000UL,
  /*    4040 */ 0x00000000UL,
  /*    4044 */ 0x00004000UL,
  /*    4048 */ 0x00040704UL,
  0x00014050UL, 0x00000000UL,
  0x0002405CUL, 0x00000000UL,
  /*    4060 */ 0x00000000UL,
  0x000140A8UL, 0x00000000UL,
  0x000440BCUL, 0x00000000UL,
  /*    40C0 */ 0x00000000UL,
  /*    40C4 */ 0x00000000UL,
  /*    40C8 */ 0x00000000UL,
  0x00044104UL, 0x000040FFUL,
  /*    4108 */ 0x00000000UL,
  /*    410C */ 0x000041FFUL,
  /*    4110 */ 0x00000000UL,
  0x1001C020UL, 0x0007F800UL,
  0x3001C020UL, 0x000802F5UL,
  0x1001C024UL, 0x000000FFUL,
  0x3001C024UL, 0x00001300UL,
  0x0007C028UL, 0x83B380ECUL,
  /*    C02C */ 0x51407543UL,
  /*    C030 */ 0x48000FA0UL,
  /*    C034 */ 0x00004030UL,
  /*    C038 */ 0x00000000UL,
  /*    C03C */ 0x00000000UL,
  /*    C040 */ 0x0000022EUL,
  0x0004C050UL, 0x04301151UL,
  /*    C054 */ 0xE6092D0EUL,
  /*    C058 */ 0x08070654UL,
  /*    C05C */ 0x0002B6D1UL,
  0x000AC064UL, 0x1C003004UL,
  /*    C068 */ 0x09183040UL,
  /*    C06C */ 0x2079640DUL,
  /*    C070 */ 0x01FBFCEBUL,
  /*    C074 */ 0x03E8F67FUL,
  /*    C078 */ 0x15724BBDUL,
  /*    C07C */ 0x0518A311UL,
  /*    C080 */ 0x76543210UL,
  /*    C084 */ 0x00000A98UL,
  /*    C088 */ 0x00000000UL,
  0x01010008UL, 0x00000700UL,
  0x01010018UL, 0x00000000UL,
  0x01010020UL, 0x00000000UL,
  0x0108401CUL, 0x00000010UL,
  /*    4020 */ 0x00087020UL,
  /*    4024 */ 0x0001000BUL,
  /*    4028 */ 0x00002000UL,
  /*    402C */ 0x000A0000UL,
  /*    4030 */ 0x03000000UL,
  /*    4034 */ 0x00000000UL,
  /*    4038 */ 0x00000000UL,
  0x01064058UL, 0x00FF0352UL,
  /*    405C */ 0x00000840UL,
  /*    4060 */ 0x00000008UL,
  /*    4064 */ 0x00320411UL,
  /*    4068 */ 0x000002C4UL,
  /*    406C */ 0x00000000UL,
  0x01114080UL, 0x11E00106UL,
  /*    4084 */ 0x0000164FUL,
  /*    4088 */ 0x002B03D3UL,
  /*    408C */ 0x00000000UL,
  /*    4090 */ 0x00000000UL,
  /*    4094 */ 0x00000000UL,
  /*    4098 */ 0x00000000UL,
  /*    409C */ 0x00000000UL,
  /*    40A0 */ 0x00000000UL,
  /*    40A4 */ 0x00000000UL,
  /*    40A8 */ 0x00000000UL,
  /*    40AC */ 0x00000000UL,
  /*    40B0 */ 0x00000000UL,
  /*    40B4 */ 0x00000000UL,
  /*    40B8 */ 0x00000000UL,
  /*    40BC */ 0x00000000UL,
  /*    40C0 */ 0x00000000UL,
  0x010240E0UL, 0x00000033UL,
  /*    40E4 */ 0x00000000UL,
  0x010140ECUL, 0x8C74987DUL,
  0x010540F4UL, 0x07830464UL,
  /*    40F8 */ 0x3AC81388UL,
  /*    40FC */ 0x000A209CUL,
  /*    4100 */ 0x00206100UL,
  /*    4104 */ 0x123556B7UL,
  0x0103410CUL, 0x001164F0UL,
  /*    4110 */ 0x29043020UL,
  /*    4114 */ 0x4040BB88UL,
  0x01024124UL, 0x00000000UL,
  /*    4128 */ 0x00000000UL,
  0x010A4130UL, 0x0C660664UL,
  /*    4134 */ 0x0000010CUL,
  /*    4138 */ 0x00FA53E8UL,
  /*    413C */ 0x00000000UL,
  /*    4140 */ 0x00000000UL,
  /*    4144 */ 0x00000000UL,
  /*    4148 */ 0x00000000UL,
  /*    414C */ 0x00000000UL,
  /*    4150 */ 0x00000000UL,
  /*    4154 */ 0x00000101UL,
  0x01034168UL, 0x07830464UL,
  /*    416C */ 0x00821388UL,
  /*    4170 */ 0x00000000UL,
  0x01044230UL, 0x00000000UL,
  /*    4234 */ 0x0E000000UL,
  /*    4238 */ 0x00000000UL,
  /*    423C */ 0x00000000UL,
  0x01024244UL, 0x00000000UL,
  /*    4248 */ 0x001F81F4UL,
  0x010C4254UL, 0x00000000UL,
  /*    4258 */ 0x003C0000UL,
  /*    425C */ 0x00000000UL,
  /*    4260 */ 0x00000000UL,
  /*    4264 */ 0x55555555UL,
  /*    4268 */ 0x00000017UL,
  /*    426C */ 0x00000000UL,
  /*    4270 */ 0x00000000UL,
  /*    4274 */ 0x0006AAAAUL,
  /*    4278 */ 0x00000000UL,
  /*    427C */ 0x00000000UL,
  /*    4280 */ 0x00000000UL,
  0x01018010UL, 0x00000003UL,
  0x01028038UL, 0x00103A3DUL,
  /*    803C */ 0x00000001UL,
  0x0103809CUL, 0x00000000UL,
  /*    80A0 */ 0x0003B870UL,
  /*    80A4 */ 0x0002C0FFUL,
  0x110180A8UL, 0x000001F0UL,
  0x310180A8UL, 0x01014205UL,
  0x110180ACUL, 0x000001F0UL,
  0x310180ACUL, 0x000D0A05UL,
  0x010280B0UL, 0x02000300UL,
  /*    80B4 */ 0x01000037UL,
  0x02020098UL, 0x04000C00UL,
  /*    009C */ 0x0000004CUL,
  0x020100A4UL, 0x00000400UL,
  0x020200D0UL, 0xAA400005UL,
  /*    00D4 */ 0x00000188UL,
  0x020100E4UL, 0x11512C6CUL,
  0x020200F4UL, 0x00000000UL,
  /*    00F8 */ 0x1108213DUL,
  0x120100FCUL, 0x0000003FUL,
  0x320100FCUL, 0x00045400UL,
  0x02010130UL, 0x02510060UL,
  0x02010154UL, 0x00003FC4UL,
  0x02010168UL, 0x00000400UL,
  0x03014FFCUL, (uint32_t) &phyInfo,
  0xFFFFFFFFUL,
};

const RAIL_ChannelConfigEntry_t WaveBird_channels[] = {
  {
    .phyConfigDeltaAdd = NULL,
    .baseFrequency = 2404800000,
    .channelSpacing = 2400000,
    .physicalChannelOffset = 0,
    .channelNumberStart = 0,
    .channelNumberEnd = 31,
    .maxPower = RAIL_TX_POWER_MAX,
    .attr = &channelConfigEntryAttr,
#ifdef RADIO_CONFIG_ENABLE_CONC_PHY
    .entryType = 0,
#endif
#ifdef RADIO_CONFIG_ENABLE_STACK_INFO
    .stackInfo = NULL,
#endif
    .alternatePhy = NULL,
  },
};

const RAIL_ChannelConfig_t WaveBird_channelConfig = {
  .phyConfigBase = WaveBird_modemConfigBase,
  .phyConfigDeltaSubtract = NULL,
  .configs = WaveBird_channels,
  .length = 1U,
  .signature = 0UL,
  .xtalFrequencyHz = 38400000UL,
};

const RAIL_ChannelConfig_t *channelConfigs[] = {
  &WaveBird_channelConfig,
  NULL
};

uint32_t wavebirdAccelerationBuffer[305];
