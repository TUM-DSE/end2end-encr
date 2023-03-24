// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_cmod.h"

#include <stddef.h>

#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/mmio.h"

#include "cmod_regs.h"  // Generated

static bool cmod_txfull(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_TXFULL_BIT);
}

static bool cmod_txempty(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_TXEMPTY_BIT);
}

static bool cmod_tx(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_TX_BIT);
}

static bool cmod_txinput_ready(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_TXINPUT_READY_BIT);
}

static bool cmod_rxfull(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_RXFULL_BIT);
}

static bool cmod_rxvalid(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_RXVALID_BIT);
}

static bool cmod_rxlast(const dif_cmod_t *cmod) {
  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  return bitfield_bit32_read(reg, CMOD_STATUS_RXLAST_BIT);
}

dif_result_t dif_cmod_watermark_rx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t value;

  switch (watermark) {
    case kDifCmodWatermarkDepth1:
      value = CMOD_CTRL_RXILVL_VALUE_RXLVL1;
      break;
    case kDifCmodWatermarkDepth2:
      value = CMOD_CTRL_RXILVL_VALUE_RXLVL2;
      break;
    case kDifCmodWatermarkDepth3:
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

dif_result_t dif_cmod_watermark_tx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t value;

  switch (watermark) {
    case kDifCmodWatermarkDepth2:
      value = CMOD_CTRL_TXILVL_VALUE_TXLVL2;
      break;
    case kDifCmodWatermarkDepth3:
      value = CMOD_CTRL_TXILVL_VALUE_TXLVL3;
      break;
    case kDifCmodWatermarkDepth4:
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

dif_result_t dif_cmod_txtrigger(const dif_cmod_t *cmod) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_bit32_write(reg, CMOD_CTRL_TXTRIGGER_BIT, true);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

  return kDifOk;
}

dif_result_t dif_cmod_txend(const dif_cmod_t *cmod) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_bit32_write(reg, CMOD_CTRL_TXEND_BIT, true);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

  return kDifOk;
}

dif_result_t dif_cmod_rxconfirm(const dif_cmod_t *cmod) {
  if (cmod == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_CTRL_REG_OFFSET);
  reg = bitfield_bit32_write(reg, CMOD_CTRL_RXCONFIRM_BIT, true);
  mmio_region_write32(cmod->base_addr, CMOD_CTRL_REG_OFFSET, reg);

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
    case kDifCmodStatusTxinputReady:
      *set = cmod_txinput_ready(cmod);
      break;
    case kDifCmodStatusRxfull:
      *set = cmod_rxfull(cmod);
      break;
    case kDifCmodStatusRxvalid:
      *set = cmod_rxvalid(cmod);
      break;
    case kDifCmodStatusRxlast:
      *set = cmod_rxlast(cmod);
      break;
    default:
      return kDifError;
  }

  return kDifOk;
}

dif_result_t dif_cmod_get_txlvl(const dif_cmod_t *cmod, size_t *lvl) {
  if (cmod == NULL || lvl == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  *lvl = bitfield_field32_read(reg, CMOD_STATUS_TXLVL_FIELD);

  return kDifOk;
}

dif_result_t dif_cmod_get_rxlvl(const dif_cmod_t *cmod, size_t *lvl) {
  if (cmod == NULL || lvl == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
  *lvl = bitfield_field32_read(reg, CMOD_STATUS_RXLVL_FIELD);

  return kDifOk;
}

dif_result_t dif_cmod_load_data(const dif_cmod_t *cmod,
                                const dif_cmod_data_t data) {
  if (cmod == NULL) {
    return kDifBadArg;
  } else if (!cmod_txinput_ready(cmod)) {
    return kDifUnavailable;
  }

  ptrdiff_t offset;

  for (int i = 0; i < CMOD_WDATA_MULTIREG_COUNT; i++) {
    offset = CMOD_WDATA_0_REG_OFFSET + (i * sizeof(uint32_t));

    mmio_region_write32(cmod->base_addr, offset, data.data[i]);
  }

  return kDifOk;
}

dif_result_t dif_cmod_read_data(const dif_cmod_t *cmod, dif_cmod_data_t *data) {
  if (cmod == NULL || data == NULL) {
    return kDifBadArg;
  }

  uint32_t reg = mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);

  if (bitfield_bit32_read(reg, CMOD_STATUS_RXLAST_BIT)) {
    return kDifError;
  } else if (!bitfield_bit32_read(reg, CMOD_STATUS_RXVALID_BIT)) {
    return kDifUnavailable;
  }

  ptrdiff_t offset;

  for (int i = 0; i < CMOD_RDATA_MULTIREG_COUNT; i++) {
    offset = CMOD_RDATA_0_REG_OFFSET + (i * sizeof(uint32_t));

    data->data[i] = mmio_region_read32(cmod->base_addr, offset);
  }

  return kDifOk;
}
