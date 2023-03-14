// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_cmod.h"
#include "sw/device/lib/runtime/ibex.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "cmod_regs.h"  // Generated
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

#define TIMEOUT (1000 * 1000)
#define WAIT_FOR_STATUS(cmod_, flag_, value_, timeout_usec_) \
  IBEX_SPIN_FOR(cmod_get_status((cmod_), (flag_)) == (value_), (timeout_usec_))

static const uint32_t data[] = {
    0x01234567, 0x89abcdef, 0xfedcab98, 0x76543210, 0x00000000, 0x11111111,
    0x22222222, 0x33333333, 0x11111111, 0x00000000, 0x00000000, 0x11111111,
    0x12121212, 0x21212121, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x23232323, 0x32323232, 0x01234567, 0x89abcdef, 0xfedcab98, 0x76543210,
    0x00000000, 0x11111111, 0x22222222, 0x33333333, 0x11111111, 0x00000000,
    0x00000000, 0x11111111,
};
OTTF_DEFINE_TEST_CONFIG();

inline bool cmod_get_status(dif_cmod_t *cmod, dif_cmod_status_t flag) {
  bool status;

  CHECK_DIF_OK(dif_cmod_get_status(cmod, flag, &status));

  return status;
}

uint32_t get_status_register(dif_cmod_t *cmod) {
  return mmio_region_read32(cmod->base_addr, CMOD_STATUS_REG_OFFSET);
}

bool test_main(void) {
  // TODO: add comments about which edge case is tested
  dif_cmod_t sender, receiver;

  dif_cmod_data_t dataOut, dataIn;

  // Initialize CMOD IP cores
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD0_BASE_ADDR), &sender));
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD1_BASE_ADDR), &receiver));

  // Trigger the sending of a new message.
  CHECK_DIF_OK(dif_cmod_txtrigger(&sender));

  // Wait until CMOD IP core is ready.
  WAIT_FOR_STATUS(&sender, kDifCmodStatusTx, true, TIMEOUT);
  WAIT_FOR_STATUS(&sender, kDifCmodStatusTxinputReady, true, TIMEOUT);

  // Check if bits of status register are correctly set.
  CHECK(get_status_register(&sender) == 0b0000000011001);

  // Send 128 bits
  memcpy(dataOut.data, data, sizeof(dataOut.data));
  CHECK_DIF_OK(dif_cmod_load_data(&sender, dataOut));

  // Wait until data is received, then check status registers.
  WAIT_FOR_STATUS(&receiver, kDifCmodStatusRxvalid, true, TIMEOUT);
  CHECK(get_status_register(&sender) == 0b0000000011001);
  CHECK(get_status_register(&receiver) == 0b0001000101000);

  // Read data, check status register, and data afterward.
  CHECK_DIF_OK(dif_cmod_read_data(&receiver, &dataIn));
  CHECK(get_status_register(&receiver) == 0b0000000001000);
  CHECK_ARRAYS_EQ(dataIn.data, data, 4);

  for (int i = 0; i < 8; i++) {
    // Send 128 bits
    memcpy(dataOut.data, &data[i * 4], sizeof(dataOut.data));
    CHECK_DIF_OK(dif_cmod_load_data(&sender, dataOut));

    // Check status registers
    if (i < 4) {
      uint32_t value = 0b0000000101000;

      // Set RXLVL field and RXFULL bit dynamically
      value += (i + 1) << 9;
      value += (i == 3) << 2;

      CHECK(get_status_register(&receiver) == value);
    } else {
      uint32_t value = 0b0100000101100;

      CHECK(get_status_register(&receiver) == value);

      value = 0b1;
      // Set TXLVL field and TXFULL bit dynamically
      value += (i == 7) << 1;
      value += (i != 7) << 4;
      value += (i - 3) << 6;
      CHECK(get_status_register(&sender) == value);
    }
  }

  // Set TXEND and check status register
  CHECK_DIF_OK(dif_cmod_txend(&sender));
  CHECK(get_status_register(&sender) == 0b0000100000011);

  for (int i = 0; i < 5; i++) {
    // Read 128 bit and check the data.
    CHECK_DIF_OK(dif_cmod_read_data(&receiver, &dataIn));
    CHECK(dataIn.data[0] == data[i * 4 + 0] &&
          dataIn.data[1] == data[i * 4 + 1] &&
          dataIn.data[2] == data[i * 4 + 2] &&
          dataIn.data[3] == data[i * 4 + 3]);

    // Check status registers
    if (i == 4) {
      CHECK(get_status_register(&receiver) == 0b0011000101000);
      CHECK(get_status_register(&sender) == 0b0000000001000);
    } else {
      CHECK(get_status_register(&receiver) == 0b0100000101100);

      uint32_t value = 0b0;

      value += (i != 3);
      value += (i == 3) << 3;
      value += (3 - i) << 6;
      CHECK(get_status_register(&sender) == value);
    }
  }

  // Trigger the sending of a new message.
  CHECK_DIF_OK(dif_cmod_txtrigger(&sender));

  // Wait until CMOD IP core is ready.
  WAIT_FOR_STATUS(&sender, kDifCmodStatusTx, true, TIMEOUT);
  WAIT_FOR_STATUS(&sender, kDifCmodStatusTxinputReady, true, TIMEOUT);

  // Check if bits of status register are correctly set.
  CHECK(get_status_register(&sender) == 0b0000000011001);

  // Send 128 bits
  memcpy(dataOut.data, data, sizeof(dataOut.data));
  CHECK_DIF_OK(dif_cmod_load_data(&sender, dataOut));

  // Check if bits of status register are correctly set.
  CHECK(get_status_register(&sender) == 0b0000001010001);
  CHECK(get_status_register(&receiver) == 0b0011000101000);

  // Receive remaining data blocks of first message
  for (int i = 0; i < 3; i++) {
    // Read 128 bit and check the data.
    CHECK_DIF_OK(dif_cmod_read_data(&receiver, &dataIn));
    CHECK(dataIn.data[0] == data[i * 4 + 20] &&
          dataIn.data[1] == data[i * 4 + 21] &&
          dataIn.data[2] == data[i * 4 + 22] &&
          dataIn.data[3] == data[i * 4 + 23]);

    if (i == 2) {
      CHECK(get_status_register(&receiver) == 0b1000000001000);
    } else {
      uint32_t value = 0b0000000101000;

      value += (2 - i) << 9;
      CHECK(get_status_register(&receiver) == value);
    }
  }

  // Set RXCONFIRM and check register
  CHECK_DIF_OK(dif_cmod_rxconfirm(&receiver));
  CHECK(get_status_register(&receiver) == 0b0001000101000);
  CHECK(get_status_register(&sender) == 0b0000000011001);

  // Read data, check status register, and data afterward.
  CHECK_DIF_OK(dif_cmod_read_data(&receiver, &dataIn));
  CHECK(get_status_register(&receiver) == 0b0000000001000);
  CHECK_ARRAYS_EQ(dataIn.data, data, 4);

  // Set TXEND
  CHECK_DIF_OK(dif_cmod_txend(&sender));
  CHECK(get_status_register(&sender) == 0b0000000001000);
  CHECK(get_status_register(&receiver) == 0b1000000001000);

  // Set RXCONFIRM and check register
  CHECK_DIF_OK(dif_cmod_rxconfirm(&receiver));
  CHECK(get_status_register(&receiver) == 0b0000000001000);

  return true;
}
