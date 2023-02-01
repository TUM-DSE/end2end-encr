// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_cmod.h"
#include "sw/device/lib/runtime/ibex.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "cmod_regs.h"  // Generated
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

static const uint32_t data[] = {
    0x01234567, 0x89abcdef, 0xfedcab98, 0x76543210, 0x00000000, 0x11111111,
    0x22222222, 0x33333333, 0x11111111, 0x00000000, 0x00000000, 0x11111111,
    0x12121212, 0x21212121, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x23232323, 0x32323232, 0x45454545, 0x67676767, 0x54545454, 0x76767676,
    0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x00000000, 0x00000000,
    0x00000000, 0x00000000};

OT_NOINLINE bool cmod_tx_full(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_TXFULL_BIT);
}

OT_NOINLINE bool cmod_rx_valid(const dif_cmod_t *cmod) {
  return mmio_region_get_bit32(cmod->base_addr, CMOD_STATUS_REG_OFFSET,
                               CMOD_STATUS_RXVALID_BIT);
}

OT_NOINLINE void cmod_set_multireg(const dif_cmod_t *cmod, const uint32_t *data,
                                   size_t num_regs, ptrdiff_t reg0_offset) {
  ptrdiff_t offset;

  for (int i = 0; i < num_regs; i++) {
    offset = reg0_offset + (i * sizeof(uint32_t));

    mmio_region_write32(cmod->base_addr, offset, data[i]);
  }
}

OT_NOINLINE void cmod_read_multireg(const dif_cmod_t *cmod, uint32_t *data,
                                    size_t num_regs, ptrdiff_t reg0_offset) {
  ptrdiff_t offset;

  for (int i = 0; i < num_regs; i++) {
    offset = reg0_offset + (i * sizeof(uint32_t));

    data[i] = mmio_region_read32(cmod->base_addr, offset);
  }
}

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  dif_cmod_t cmod0, cmod1;
  dif_cmod_data_t kReceive;
  uint32_t total;
  uint64_t start_cycles, end_cycles;

  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD0_BASE_ADDR), &cmod0));
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD1_BASE_ADDR), &cmod1));

  // TODO: Only temporary, remove after `txend` is implemented.
  CHECK_DIF_OK(dif_cmod_txtrigger(&cmod0));

  LOG_INFO("Running CMOD Performance Test...");

  LOG_INFO("Performance Test 1: Read TXFULL register.");

  start_cycles = ibex_mcycle_read();
  cmod_tx_full(&cmod0);
  end_cycles = ibex_mcycle_read();

  total = end_cycles - start_cycles;
  LOG_INFO("Duration: %u cycles", total);

  LOG_INFO("Performance Test 2: Write 128bits to WDATA.");

  start_cycles = ibex_mcycle_read();
  cmod_set_multireg(&cmod0, &data[0], 4, CMOD_WDATA_0_REG_OFFSET);
  end_cycles = ibex_mcycle_read();

  total = end_cycles - start_cycles;
  LOG_INFO("Duration: %u cycles", total);

  LOG_INFO("Performance Test 3: Read RXVALID register.");

  start_cycles = ibex_mcycle_read();
  cmod_rx_valid(&cmod1);
  end_cycles = ibex_mcycle_read();

  total = end_cycles - start_cycles;
  LOG_INFO("Duration: %u cycles", total);

  while (!cmod_rx_valid(&cmod1)) {
  }

  LOG_INFO("Performance Test 4: Read 128bits from RDATA.");

  start_cycles = ibex_mcycle_read();
  cmod_read_multireg(&cmod1, &kReceive.data[0], 4, CMOD_RDATA_0_REG_OFFSET);
  end_cycles = ibex_mcycle_read();

  total = end_cycles - start_cycles;
  LOG_INFO("Duration: %u cycles", total);

  LOG_INFO("Performance Test 5: Send & receive 128bits.");

  start_cycles = ibex_mcycle_read();
  while (cmod_tx_full(&cmod0)) {
  }

  cmod_set_multireg(&cmod0, &data[0], 4, CMOD_WDATA_0_REG_OFFSET);

  while (!cmod_rx_valid(&cmod1)) {
  }

  cmod_read_multireg(&cmod1, &kReceive.data[0], 4, CMOD_RDATA_0_REG_OFFSET);
  end_cycles = ibex_mcycle_read();

  total = end_cycles - start_cycles;
  LOG_INFO("Duration: %u cycles", total);

  LOG_INFO("Finished CMOD Performance Test");

  return true;
}
