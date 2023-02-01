// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/testing/cmod_testutils.h"

extern bool cmod_testutils_get_status(dif_cmod_t *cmod, dif_cmod_status_t flag);

void log_tx_status_regs(dif_cmod_t *cmod) {
  bool val;

  val = cmod_testutils_get_status(cmod, kDifCmodStatusTx);
  LOG_INFO("TX: %i", val);

  val = cmod_testutils_get_status(cmod, kDifCmodStatusTxfull);
  LOG_INFO("TXFULL: %i", val);

  val = cmod_testutils_get_status(cmod, kDifCmodStatusTxempty);
  LOG_INFO("TXEMPTY: %i", val);
}

void log_rx_status_regs(dif_cmod_t *cmod) {
  bool val;

  val = cmod_testutils_get_status(cmod, kDifCmodStatusRxvalid);
  LOG_INFO("RXVALID: %i", val);

  val = cmod_testutils_get_status(cmod, kDifCmodStatusRxfull);
  LOG_INFO("RXFULL: %i", val);
}

void log_status_regs(dif_cmod_t *cmod) {
  log_tx_status_regs(cmod);
  log_rx_status_regs(cmod);
}