// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "hw/ip/aes/model/aes_modes.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_aes.h"
#include "sw/device/lib/dif/dif_cmod.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/aes_testutils.h"
#include "sw/device/lib/testing/cmod_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

#define TIMEOUT (1000 * 1000)

// The mask share, used to mask kKey. Note that the masking should not be done
// manually. Software is expected to get the key in two shares right from the
// beginning.
static const uint8_t kKeyShare1[] = {
    0x0f, 0x1f, 0x2f, 0x3F, 0x4f, 0x5f, 0x6f, 0x7f, 0x8f, 0x9f, 0xaf,
    0xbf, 0xcf, 0xdf, 0xef, 0xff, 0x0a, 0x1a, 0x2a, 0x3a, 0x4a, 0x5a,
    0x6a, 0x7a, 0x8a, 0x9a, 0xaa, 0xba, 0xca, 0xda, 0xea, 0xfa,
};

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  dif_cmod_t cmod0, cmod1;
  dif_aes_t aes;

  // Initialise CMOD0, CMOD1, and AES.
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD0_BASE_ADDR), &cmod0));
  CHECK_DIF_OK(dif_cmod_init(
      mmio_region_from_addr(TOP_EARLGREY_CMOD1_BASE_ADDR), &cmod1));
  CHECK_DIF_OK(
      dif_aes_init(mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR), &aes));
  CHECK_DIF_OK(dif_aes_reset(&aes));

  // TODO: Only temporary, remove after `txend` is implemented.
  CHECK_DIF_OK(dif_cmod_txtrigger(&cmod0));

  // Mask the key.
  uint8_t key_share0[sizeof(kAesModesKey256)];
  for (int i = 0; i < sizeof(kAesModesKey256); ++i) {
    key_share0[i] = kAesModesKey256[i] ^ kKeyShare1[i];
  }

  // "Convert" key share byte arrays to `dif_aes_key_share_t`.
  dif_aes_key_share_t key;
  memcpy(key.share0, key_share0, sizeof(key.share0));
  memcpy(key.share1, kKeyShare1, sizeof(key.share1));

  // "Convert" iv byte arrays to `dif_aes_iv_t`.
  dif_aes_iv_t iv;
  memcpy(iv.iv, kAesModesIvCtr, sizeof(iv.iv));

  // Setup CTR encryption transaction.
  dif_aes_transaction_t transaction = {
      .operation = kDifAesOperationEncrypt,
      .mode = kDifAesModeCtr,
      .key_len = kDifAesKey256,
      .key_provider = kDifAesKeySoftwareProvided,
      .mask_reseeding = kDifAesReseedPerBlock,
      .manual_operation = kDifAesManualOperationAuto,
      .reseed_on_key_change = false,
      .ctrl_aux_lock = false,
  };
  CHECK_DIF_OK(dif_aes_start(&aes, &transaction, &key, &iv));

  // "Convert" plain data byte arrays to `dif_aes_data_t`.
  dif_aes_data_t in_data_plain;
  memcpy(in_data_plain.data, kAesModesPlainText, sizeof(in_data_plain.data));

  // Log encryption.
  LOG_INFO("Encrypt:\t\t\t0x%08x%08x%08x%08x", in_data_plain.data[0],
           in_data_plain.data[1], in_data_plain.data[2], in_data_plain.data[3]);

  // Load the plain text to trigger the encryption operation.
  AES_TESTUTILS_WAIT_FOR_STATUS(&aes, kDifAesStatusInputReady, true, TIMEOUT);
  CHECK_DIF_OK(dif_aes_load_data(&aes, in_data_plain));

  // Read out the produced cipher text.
  dif_aes_data_t out_data;
  AES_TESTUTILS_WAIT_FOR_STATUS(&aes, kDifAesStatusOutputValid, true, TIMEOUT);
  CHECK_DIF_OK(dif_aes_read_output(&aes, &out_data));

  // Finish the CTR encryption transaction.
  CHECK_DIF_OK(dif_aes_end(&aes));
  CHECK_ARRAYS_EQ((uint8_t *)out_data.data, kAesModesCipherTextCtr256,
                  sizeof(out_data.data));

  LOG_INFO("Encryption Result:\t0x%08x%08x%08x%08x", out_data.data[0],
           out_data.data[1], out_data.data[2], out_data.data[3]);

  // Sending data.
  dif_cmod_data_t wdata, rdata;
  memcpy(wdata.data, out_data.data, sizeof(wdata.data));
  LOG_INFO("Sending:\t\t0x%08x%08x%08x%08x", out_data.data[0], out_data.data[1],
           out_data.data[2], out_data.data[3]);

  CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod0, kDifCmodStatusTxfull, false, TIMEOUT);
  CHECK_DIF_OK(dif_cmod_send_data(&cmod0, wdata));

  CMOD_TESTUTILS_WAIT_FOR_STATUS(&cmod1, kDifCmodStatusRxvalid, true, TIMEOUT);
  CHECK_DIF_OK(dif_cmod_read_data(&cmod1, &rdata));

  LOG_INFO("Received:\t\t0x%08x%08x%08x%08x", rdata.data[0], rdata.data[1],
           rdata.data[2], rdata.data[3]);

  // Setup CTR decryption transaction.
  transaction.operation = kDifAesOperationDecrypt;
  CHECK_DIF_OK(dif_aes_start(&aes, &transaction, &key, &iv));

  // Load the received cipher text to start the decryption operation.
  memcpy(out_data.data, rdata.data, sizeof(out_data.data));
  AES_TESTUTILS_WAIT_FOR_STATUS(&aes, kDifAesStatusInputReady, true, TIMEOUT);
  CHECK_DIF_OK(dif_aes_load_data(&aes, out_data));

  // Read out the produced plain text.
  AES_TESTUTILS_WAIT_FOR_STATUS(&aes, kDifAesStatusOutputValid, true, TIMEOUT);
  CHECK_DIF_OK(dif_aes_read_output(&aes, &out_data));

  // Finish the CTR encryption transaction.
  CHECK_DIF_OK(dif_aes_end(&aes));

  LOG_INFO("Decryption Result:\t0x%08x%08x%08x%08x", out_data.data[0],
           out_data.data[1], out_data.data[2], out_data.data[3]);

  CHECK_ARRAYS_EQ((uint8_t *)out_data.data, kAesModesPlainText,
                  sizeof(out_data.data));

  return true;
}
