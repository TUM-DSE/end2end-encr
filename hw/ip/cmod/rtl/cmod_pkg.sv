// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

package cmod_pkg;
  typedef struct packed {
    logic         valid;
    logic         last;
    logic [127:0] data;
  } cmod_req_t;

  typedef struct packed {
    logic         valid;
    logic         last;
    logic [127:0] data;
  } cmod_rcv_t;
endpackage : cmod_pkg