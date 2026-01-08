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
  0x00666666UL, // 102.4
  (uint32_t) NULL,
  (uint32_t) irCalConfig,
  (uint32_t) timingConfig,
  0x00000000UL,
  9760000UL,
  38400000UL,
  1440000UL,
  0x00000F01UL,
  0x02004924UL,
  (uint32_t) NULL,
  (uint32_t) NULL,
  (uint32_t) NULL,
  0UL,
  0UL,
  1440000UL,
  (uint32_t) NULL,
  (uint32_t) NULL,
  (uint32_t) NULL,
};

const uint32_t WaveBird_modemConfigBase[] = {
  0x01040FF0UL, (uint32_t) &phyInfo,
  /*    0FF4 */ 0x00000000UL,
  /*    0FF8 */ 0x0003C000UL,
  /*    0FFC */ 0x0003C00FUL,
  0x00020004UL, 0x00000000UL,
  /*    0008 */ 0x00000000UL,
  0x00020018UL, 0x00000012UL,
  /*    001C */ 0x00000000UL,
  0x00070028UL, 0x00000000UL,
  /*    002C */ 0x00000000UL,
  /*    0030 */ 0x00000000UL,
  /*    0034 */ 0x00000000UL,
  /*    0038 */ 0x00000000UL,
  /*    003C */ 0x00000000UL,
  /*    0040 */ 0x00000704UL,
  0x00010048UL, 0x00000000UL,
  0x00020054UL, 0x00000000UL,
  /*    0058 */ 0x00000000UL,
  0x000400A0UL, 0x000040FFUL,
  /*    00A4 */ 0x00000000UL,
  /*    00A8 */ 0x000041FFUL,
  /*    00AC */ 0x00000000UL,
  0x00012000UL, 0x00000700UL,
  0x00012010UL, 0x00000000UL,
  0x00012018UL, 0x00000000UL,
  0x00013008UL, 0x0000AC3FUL,
  0x00023030UL, 0x00104924UL,
  /*    3034 */ 0x00000001UL,
  0x00013040UL, 0x00000000UL,
  0x000140A0UL, 0x0F00277AUL,
  0x000140F4UL, 0x00001020UL,
  0x00024134UL, 0x00000880UL,
  /*    4138 */ 0x000087E6UL,
  0x00024140UL, 0x0088006DUL,
  /*    4144 */ 0x1153E6C0UL,
  0x00156014UL, 0x00000010UL,
  /*    6018 */ 0x00087020UL,
  /*    601C */ 0x0000000BUL,
  /*    6020 */ 0x00003000UL,
  /*    6024 */ 0x000A0000UL,
  /*    6028 */ 0x03000000UL,
  /*    602C */ 0x00000000UL,
  /*    6030 */ 0x00FF0352UL,
  /*    6034 */ 0x00000C61UL,
  /*    6038 */ 0x00000001UL,
  /*    603C */ 0x00320411UL,
  /*    6040 */ 0x000002C4UL,
  /*    6044 */ 0x00000000UL,
  /*    6048 */ 0x13C0011DUL,
  /*    604C */ 0x0000164FUL,
  /*    6050 */ 0x002A03D0UL,
  /*    6054 */ 0x0094905AUL,
  /*    6058 */ 0x00000001UL,
  /*    605C */ 0x00000000UL,
  /*    6060 */ 0x00000000UL,
  /*    6064 */ 0x00000000UL,
  0x10017014UL, 0x0007F800UL,
  0x30017014UL, 0x000000F8UL,
  0x10017018UL, 0x000000FFUL,
  0x30017018UL, 0x00000300UL,
  0x0001701CUL, 0x82730060UL,
  0x00017028UL, 0x01800000UL,
  0x00027048UL, 0x00003D3CUL,
  /*    704C */ 0x000019BCUL,
  0x00037070UL, 0x00020105UL,
  /*    7074 */ 0x00000433UL,
  /*    7078 */ 0x00552300UL,
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

uint32_t wavebirdAccelerationBuffer[135];
