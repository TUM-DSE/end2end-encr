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

/**
 * A CMOD FIFO watermark depth configuration.
 */
typedef enum dif_cmod_watermark {
  /**
   * Indicates a one data block watermark.
   */
  kDifCmodWatermarkDataBlock1 = 0,
  /**
   * Indicates a two data blocks watermark.
   */
  kDifCmodWatermarkDataBlock2,
  /**
   * Indicates a three data blocks watermark.
   */
  kDifCmodWatermarkDataBlock3,
  /**
   * Indicates a four data blocks watermark.
   */
  kDifCmodWatermarkDataBlock4,
} dif_cmod_watermark_t;

typedef struct dif_cmod_data {
  uint32_t data[4];
} dif_cmod_data_t;

// TODO: update
/**
 * The size of the CMOD TX and RX FIFOs, in blocks of 4 bytes.
 */
extern const uint32_t kDifCmodFifoSizeBlocks;

/**
 * Sets the RX FIFO watermark.
 *
 * This function is only useful when the corresponding interrupt is enabled.
 * When the queued RX FIFO number of data blocks rises to or above this
 * level, the RX watermark interrupt is raised.
 *
 * @param cmod A cmod handle.
 * @param watermark RX FIFO watermark.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_watermark_rx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark);

/**
 * Sets the TX FIFO watermark.
 *
 * This function is only useful when the corresponding interrupt is enabled.
 * When the queued TX FIFO number of data blocks sinks below this
 * level, the TX watermark interrupt is raised.
 *
 * @param cmod A cmod handle.
 * @param watermark TX FIFO watermark.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_watermark_tx_set(const dif_cmod_t *cmod,
                                       dif_cmod_watermark_t watermark);

// TODO: remove or update
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_txtrigger(const dif_cmod_t *cmod);

/**
 * Transmits 4 bytes of data.
 *
 * The peripheral must be able to accept the input (TXFULL unset), and
 * will return `kDifUnavailable` if this condition is not met.
 *
 * @param cmod A CMOD handle.
 * @param data Data to be transmitted.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_send_data(const dif_cmod_t *cmod, dif_cmod_data_t data);

/**
 * Read 4 bytes of received data.
 *
 * The peripheral must have valid data (RXVALID set), and will
 * return `kDifUnavailable` if this condition is not met.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_read_data(const dif_cmod_t *cmod, dif_cmod_data_t *data);

// TODO: update
/**
 * Gets the number of blocks available to be read from the CMOD RX FIFO.
 *
 * This function can be used to check FIFO full and empty conditions.
 *
 * @param cmod A CMOD handle.
 * @param[out] num_blocks Number of blocks available to be read.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_rx_blocks_available(const dif_cmod_t *cmod,
                                          size_t *num_blocks);

// TODO: update
/**
 * Gets the number of blocks available to be written to the CMOD TX FIFO.
 *
 * This function can be used to check FIFO full and empty conditions.
 *
 * @param cmod A CMOD handle.
 * @param[out] num_blocks Number of blocks available to be written.
 * @return The result of the operation.
 */
OT_WARN_UNUSED_RESULT
dif_result_t dif_cmod_tx_blocks_available(const dif_cmod_t *cmod,
                                          size_t *num_blocks);

/**
 * CMOD Status flags.
 */
typedef enum dif_cmod_status {
  kDifCmodStatusTxfull = 0,
  kDifCmodStatusTxempty,
  kDifCmodStatusTx,
  kDifCmodStatusRxfull,
  kDifCmodStatusRxvalid,
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

// TODO: Remove or update
dif_result_t dif_cmod_get_tx_lvl(const dif_cmod_t *cmod, uint32_t *val);

dif_result_t dif_cmod_get_rx_lvl(const dif_cmod_t *cmod, uint32_t *val);

// TODO: Add transaction methods

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_CMOD_H_
