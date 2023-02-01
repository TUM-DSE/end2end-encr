// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_cmod.h"

#include <assert.h>
#include <stddef.h>

#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/mmio.h"

#include "cmod_regs.h"  // Generated

const uint32_t kDifCmodFifoSizeBlocks = 4u;

static bool cmod_txfull(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_TXFULL_BIT);
}

static bool cmod_txempty(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_TXEMPTY_BIT);
}

static bool cmod_tx(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_TX_BIT);
}

static bool cmod_rxfull(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_RXFULL_BIT);
}

static bool cmod_rxvalid(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_RXVALID_BIT);
}

static void cmod_set_multireg(const dif_cmod_t *cmod, const uint32_t *data,
                              size_t num_regs, ptrdiff_t reg0_offset) {
  ptrdiff_t offset;

  for (int i = 0; i < num_regs; i++) {
    offset = reg0_offset + (i * sizeof(uint32_t));

    mmio_region_write32(cmod->base_addr, offset, data[i]);
  }
}

static void cmod_read_multireg(const dif_cmod_t *cmod, uint32_t *data,
                               size_t num_regs, ptrdiff_t reg0_offset) {
  ptrdiff_t offset;

  for (int i = 0; i < num_regs; i++) {
    offset = reg0_offset + (i * sizeof(uint32_t));

    data[i] = mmio_region_read32(cmod->base_addr, offset);
  }
}

static void cmod_reset() {
  // TODO:
}

dif_result_t dif_cmod_watermark_tx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t value;

  switch (watermark) {
    case kDifCmodWatermarkDataBlock2:
      value = CMOD_CTRL_TXILVL_VALUE_TXLVL2;
      break;
    case kDifCmodWatermarkDataBlock3:
      value = CMOD_CTRL_TXILVL_VALUE_TXLVL3;
      break;
    case kDifCmodWatermarkDataBlock4:
      value = CMOD_CTRL_TXILVL_VALUE_TXLVL4;
      break;
    default:
      return kDifError;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_field32_write(reg, CMOD_CTRL_TXILVL_FIELD, value);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

  return kDifOk;
}

dif_result_t dif_cmod_watermark_rx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t value;

  switch (watermark) {
    case kDifCmodWatermarkDataBlock1:
      value = CMOD_CTRL_RXILVL_VALUE_RXLVL1;
      break;
    case kDifCmodWatermarkDataBlock2:
      value = CMOD_CTRL_RXILVL_VALUE_RXLVL2;
      break;
    case kDifCmodWatermarkDataBlock3:
      value = CMOD_CTRL_RXILVL_VALUE_RXLVL3;
      break;
    default:
      return kDifError;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_field32_write(reg, CMOD_CTRL_RXILVL_FIELD, value);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

  return kDifOk;
}

dif_result_t dif_cmod_txtrigger(const dif_cmod_t *cmod) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_bit32_write(reg, CMOD_CTRL_TXTRIGGER_BIT, true);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

  return kDifOk;
}

dif_result_t dif_cmod_send_data(const dif_cmod_t *cmod, dif_cmod_data_t data) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  if (cmod_txfull(cmod)) {  // TODO: Decide if to use while instead
    return kDifUnavailable;
  }

  cmod_set_multireg(cmod, &data.data[0], CMOD_WDATA_MULTIREG_COUNT,
                    CMOD_WDATA_0_REG_OFFSET);

  return kDifOk;
}

dif_result_t dif_cmod_read_data(const dif_cmod_t *cmod, dif_cmod_data_t *data) {
  if (cmod == NULL || data == NULL) {
    return kDifBadArg;
  }

  if (!cmod_rxvalid(cmod)) {  // TODO: Decide if to use while instead
    return kDifUnavailable;
  }

  cmod_read_multireg(cmod, data->data, CMOD_RDATA_MULTIREG_COUNT,
                     CMOD_RDATA_0_REG_OFFSET);

  return kDifOk;
}

dif_result_t dif_cmod_tx_blocks_available(const dif_cmod_t *cmod,
                                          size_t *num_blocks) {
  if (cmod == NULL || num_blocks == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  uint32_t fill_blocks = bitfield_field32_read(reg, CMOD_STATUS_TXLVL_FIELD);
  *num_blocks = kDifCmodFifoSizeBlocks - fill_blocks;

  return kDifOk;
}

dif_result_t dif_cmod_rx_blocks_available(const dif_cmod_t *cmod,
                                          size_t *num_blocks) {
  if (cmod == NULL || num_blocks == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  uint32_t fill_blocks = bitfield_field32_read(reg, CMOD_STATUS_RXLVL_FIELD);
  *num_blocks = kDifCmodFifoSizeBlocks - fill_blocks;

  return kDifOk;
}

dif_result_t dif_cmod_get_tx_lvl(const dif_cmod_t *cmod, uint32_t *val) {
  if (cmod == NULL || val == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  *val = bitfield_field32_read(reg, CMOD_STATUS_TXLVL_FIELD);

  return kDifOk;
}

dif_result_t dif_cmod_get_rx_lvl(const dif_cmod_t *cmod, uint32_t *val) {
  if (cmod == NULL || val == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  *val = bitfield_field32_read(reg, CMOD_STATUS_RXLVL_FIELD);

  return kDifOk;
}

dif_result_t dif_cmod_get_status(const dif_cmod_t *cmod, dif_cmod_status_t flag,
                                 bool *set) {
  if (cmod == NULL || set == NULL) {
    return kDifBadArg;
  }

  switch (flag) {
    case kDifCmodStatusTxfull:
      *set = cmod_txfull(cmod);
      break;
    case kDifCmodStatusTxempty:
      *set = cmod_txempty(cmod);
      break;
    case kDifCmodStatusTx:
      *set = cmod_tx(cmod);
      break;
    case kDifCmodStatusRxfull:
      *set = cmod_rxfull(cmod);
      break;
    case kDifCmodStatusRxvalid:
      *set = cmod_rxvalid(cmod);
      break;
    default:
      return kDifError;
  }

  return kDifOk;
}
