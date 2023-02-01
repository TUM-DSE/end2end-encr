// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_cmod.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/cmod_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

#define TIMEOUT (1000 * 1000)

static const uint32_t data[] = {
    0x01234567, 0x89abcdef, 0xfedcab98, 0x76543210, 0x00000000, 0x11111111,
    0x22222222, 0x33333333, 0x11111111, 0x00000000, 0x00000000, 0x11111111,
    0x12121212, 0x21212121, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x23232323, 0x32323232, 0x45454545, 0x67676767, 0x54545454, 0x76767676,
    0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x00000000, 0x00000000,
    0x00000000, 0x00000000};

OTTF_DEFINE_TEST_CONFIG();

void log_sending(dif_cmod_data_t *kSend) {
  LOG_INFO("\tSending:\t0x%08x%08x%08x%08x", kSend->data[0], kSend->data[1],
           kSend->data[2], kSend->data[3]);
}

void log_received(dif_cmod_data_t *kReceive) {
  LOG_INFO("\tReceived:\t0x%08x%08x%08x%08x", kReceive->data[0],
           kReceive->data[1], kReceive->data[2], kReceive->data[3]);
}

bool test_main(void) {
  // TODO: Add tests for TX register
  dif_cmod_t cmod0, cmod1;
  dif_cmod_data_t kSend, kReceive;

  // Initialise CMOD0 and CMOD1.
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD0_BASE_ADDR), &cmod0));
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD1_BASE_ADDR), &cmod1));

  // TODO: Only temporary, remove after `txend` is implemented.
  CHECK_DIF_OK(dif_cmod_txtrigger(&cmod0));

  LOG_INFO("Running CMOD DIF test...");
  LOG_INFO("Test 1: Send 128 bits");

  memcpy(kSend.data, &data, sizeof(dif_cmod_data_t));

  // Send data.
  log_sending(&kSend);
  CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod0, kDifCmodStatusTxfull, false, TIMEOUT);
  CHECK_DIF_OK(dif_cmod_send_data(&cmod0, kSend));

  // Read received data.
  CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod1, kDifCmodStatusRxvalid, true, TIMEOUT);
  CHECK_DIF_OK(dif_cmod_read_data(&cmod1, &kReceive));
  log_received(&kReceive);
  CHECK(kReceive.data[0] == data[0] && kReceive.data[1] == data[1] &&
        kReceive.data[2] == data[2] && kReceive.data[3] == data[3]);

  LOG_INFO("Test 2: Send 8x 128 bits and check status regs.");
  bool val;
  uint32_t val2;

  for (int i = 0; i < 8; i++) {
    memcpy(kSend.data, &data[i * 4], sizeof(dif_cmod_data_t));

    // Send data.
    log_sending(&kSend);
    CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod0, kDifCmodStatusTxfull, false,
                                   TIMEOUT);
    CHECK_DIF_OK(dif_cmod_send_data(&cmod0, kSend));

    if (i < 4) {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxempty, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_tx_lvl(&cmod0, &val2));
      CHECK(val2 == 0);
    } else {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxempty, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_tx_lvl(&cmod0, &val2));
      CHECK(val2 == i - 3);
    }

    if (i >= 4) {
      if (i == 7) {
        CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxfull, &val));
        CHECK(val == true);
      } else {
        CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxfull, &val));
        CHECK(val == false);
      }
    }

    if (i < 3) {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxvalid, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_rx_lvl(&cmod1, &val2));
      CHECK(val2 == i + 1);
    } else {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxfull, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxvalid, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_rx_lvl(&cmod1, &val2));
      CHECK(val2 == 4);
    }
  }

  for (int i = 0; i < 8; i++) {
    // Read received data.
    CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod1, kDifCmodStatusRxvalid, true,
                                   TIMEOUT);
    CHECK_DIF_OK(dif_cmod_read_data(&cmod1, &kReceive));
    log_received(&kReceive);
    CHECK(kReceive.data[0] == data[i * 4] &&
          kReceive.data[1] == data[i * 4 + 1] &&
          kReceive.data[2] == data[i * 4 + 2] &&
          kReceive.data[3] == data[i * 4 + 3]);

    if (i < 3) {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxempty, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_tx_lvl(&cmod0, &val2));
      CHECK(val2 == 3 - i);
    } else {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxempty, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod0, kDifCmodStatusTxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_tx_lvl(&cmod0, &val2));
      CHECK(val2 == 0);
    }

    if (i < 4) {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxfull, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxvalid, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_rx_lvl(&cmod1, &val2));
      CHECK(val2 == 4);
    } else if (i < 7) {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxvalid, &val));
      CHECK(val == true);

      CHECK_DIF_OK(dif_cmod_get_rx_lvl(&cmod1, &val2));
      CHECK(val2 == 7 - i);
    } else {
      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxfull, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_status(&cmod1, kDifCmodStatusRxvalid, &val));
      CHECK(val == false);

      CHECK_DIF_OK(dif_cmod_get_rx_lvl(&cmod1, &val2));
      CHECK(val2 == 0);
    }
  }

  LOG_INFO("Finished CMOD DIF test");

  return true;
}
