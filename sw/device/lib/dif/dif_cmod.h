// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_DIF_DIF_CMOD_H_
#define OPENTITAN_SW_DEVICE_LIB_DIF_DIF_CMOD_H_

/**
 * @file
 * @brief <a href="/hw/ip/cmod/doc/">CMOD</a> Device Interface Functions
 */

#include <stdint.h>

#include "sw/device/lib/dif/autogen/dif_cmod_autogen.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
 * The size of the CMOD WDATA and RDATA FIFOs, in multiple of 4 bytes.
 */
extern const uint32_t kDifCmodFifoSize;

/**
 * A CMOD FIFO watermark depth configuration.
 */
typedef enum dif_cmod_watermark {
  /**
   * Indicates a depth of one.
   */
  kDifCmodWatermarkDepth1 = 0,
  /**
   * Indicates a depth of two.
   */
  kDifCmodWatermarkDepth2,
  /**
   * Indicates a depth of three.
   */
  kDifCmodWatermarkDepth3,
  /**
   * Indicates a depth of four.
   */
  kDifCmodWatermarkDepth4,
} dif_cmod_watermark_t;

/**
 * Sets the RDATA FIFO watermark.
 *
 * This function is only useful when the corresponding interrupt is enabled.
 * When the queued RDATA FIFO number of data blocks rises to or above this
 * level, the RX watermark interrupt is raised.
 *
 * @param cmod A cmod handle.
 * @param watermark RDATA FIFO watermark.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_watermark_rx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark);

/**
 * Sets the WDATA FIFO watermark.
 *
 * This function is only useful when the corresponding interrupt is enabled.
 * When the queued WDATA FIFO number of data blocks sinks below this
 * level, the TX watermark interrupt is raised.
 *
 * @param cmod A cmod handle.
 * @param watermark WDATA FIFO watermark.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_watermark_tx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark);

/**
 * Sets the TXTRIGGER bit of the CTRL register.
 *
 * This marks the beginning of a new message.
 *
 * @param cmod A cmod handle.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_txtrigger(const dif_cmod_t *cmod);

/**
 * Sets the TXEND bit of the CTRL register.
 *
 * This marks the end of a message and is set after the last data block of the
 * message is loaded into the CMOD module.
 *
 * @param cmod A cmod handle.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_txend(const dif_cmod_t *cmod);

/**
 * Sets the RXCONFIRM bit of the CTRL register.
 *
 * This confirms that the processor knows, that the last data block of a
 * message was read from the CMOD module.
 *
 * @param cmod A cmod handle.
 * @return The result of the operation.
 */
dif_result_t dif_cmod_rxconfirm(const dif_cmod_t *cmod);

/**
 * CMOD Status flags.
 */
typedef enum dif_cmod_status {
  kDifCmodStatusTxfull = 0,
  kDifCmodStatusTxempty,
  kDifCmodStatusTx,
  kDifCmodStatusTxinputReady,
  kDifCmodStatusRxfull,
  kDifCmodStatusRxvalid,
  kDifCmodStatusRxlast,
} dif_cmod_status_t;

/**
 * Queries the CMOD status flags.
 *
 * @param cmod A CMOD handle.
 * @param flag Status flag to query.
 * @param[out] set Flag state (set/unset).
 * @return The result of the operation.
 */
dif_result_t dif_cmod_get_status(const dif_cmod_t *cmod, dif_cmod_status_t flag,
                                 bool *set);

/**
 * Gets the lvl of the WDATA FIFO.
 *
 * @param cmod A cmod handle.
 * @param[out] lvl The current lvl of the WDATA FIFO.
 * @return The result of the operation.
 */
dif_result_t dif_cmod_get_txlvl(const dif_cmod_t *cmod, size_t *lvl);

/**
 * Gets the lvl of the RDATA FIFO.
 *
 * @param cmod A cmod handle.
 * @param[out] lvl The current lvl of the RDATA FIFO.
 * @return The result of the operation.
 */
dif_result_t dif_cmod_get_rxlvl(const dif_cmod_t *cmod, size_t *lvl);

/*
 * A typed representation of the CMOD data.
 */
typedef struct dif_cmod_data {
  uint32_t data[4];
} dif_cmod_data_t;

/**
 * Loads the CMOD Input Data (WDATA).
 *
 * The CMOD module must be able to accept input (TXINPUT_READY is set).
 * `kDifUnavailable` will be returned if this condition is not met.
 *
 * @param cmod A cmod handle.
 * @param data CMOD Input Data.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_load_data(const dif_cmod_t *cmod,
                                const dif_cmod_data_t data);

/**
 * Reads the CMOD Output Data (RDATA).
 *
 * The CMOD module must have available data (RXVALID is set).
 * `kDifUnavailable` will be returned if this condition is not met.
 *
 * If the last data block of a message was received and not confirmed yet
 * (RXLAST is set), `kDifError` will be returned.
 *
 * @param cmod A cmod handle.
 * @param[out] data CMOD Output Data.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_read_data(const dif_cmod_t *cmod, dif_cmod_data_t *data);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_CMOD_H_
