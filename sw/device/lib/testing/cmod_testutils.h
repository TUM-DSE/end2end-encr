// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_TESTING_CMOD_TESTUTILS_H_
#define OPENTITAN_SW_DEVICE_LIB_TESTING_CMOD_TESTUTILS_H_

#include "sw/device/lib/dif/dif_cmod.h"
#include "sw/device/lib/runtime/ibex.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"

/**
 * Returns the value of the CMOD status flag.
 *
 * @param cmod A CMOD handle.
 * @param flag Status flag to query.
 */
inline bool cmod_testutils_get_status(dif_cmod_t *cmod,
                                      dif_cmod_status_t flag) {
  bool status;
  CHECK_DIF_OK(dif_cmod_get_status(cmod, flag, &status));

  return status;
}

/**
 * Waits for the given CMOD status flag to be set to the given value.
 *
 * @param cmod A CMOD handle.
 * @param flag Status flag to query.
 * @param value The status flag value.
 * @param timeout_usec Timeout in microseconds.
 */
#define CMOD_TESTUTILS_WAIT_FOR_STATUS(cmod_, flag_, value_, timeout_usec_) \
  IBEX_SPIN_FOR(cmod_testutils_get_status((cmod_), (flag_)) == (value_),    \
                (timeout_usec_))

/**
 * Logs the RX status registers.
 */
void log_tx_status_regs(dif_cmod_t *cmod);

/**
 * Logs the TX status registers.
 */
void log_rx_status_regs(dif_cmod_t *cmod);

/**
 * Logs all the status registers.
 */
void log_status_regs(dif_cmod_t *cmod);

#endif  // OPENTITAN_SW_DEVICE_LIB_TESTING_CMOD_TESTUTILS_H_