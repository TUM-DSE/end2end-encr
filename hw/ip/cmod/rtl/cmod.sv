// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Description: CMOD top level wrapper file

module cmod
    import cmod_reg_pkg::*;
    import cmod_pkg::*;
#(
    parameter logic [NumAlerts-1:0] AlertAsyncOn = {NumAlerts{1'b1}}
) (
    input           clk_i,
    input           rst_ni,

    // Bus Interface
    input  tlul_pkg::tl_h2d_t tl_i,
    output tlul_pkg::tl_d2h_t tl_o,

    // Inter-module signals
    input  cmod_rcv_t   rx_i,
    input  logic        tx_rready_i,
    output cmod_req_t   tx_o,
    output logic        rx_wready_o,

    // Alerts
    input  prim_alert_pkg::alert_rx_t [NumAlerts-1:0] alert_rx_i,
    output prim_alert_pkg::alert_tx_t [NumAlerts-1:0] alert_tx_o,

    // Interrupts
    output logic    intr_tx_watermark_o,
    output logic    intr_rx_watermark_o,
    output logic    intr_tx_empty_o
);

    logic [NumAlerts-1:0] alert_test, alerts;
    cmod_reg2hw_t reg2hw;
    cmod_hw2reg_t hw2reg;

    ////////////
    // Inputs //
    ////////////

    // Register Interface
    cmod_reg_top u_reg (
        .clk_i,
        .rst_ni,
        .tl_i,
        .tl_o,
        .reg2hw,
        .hw2reg,
        // SEC_CM: BUS.INTEGRITY
        .intg_err_o (alerts[0]),
        .devmode_i (1'b1)
    );

    //////////
    // Core //
    //////////

    // CMOD core
    cmod_core cmod_core (
        .clk_i,
        .rst_ni,
        .reg2hw,
        .hw2reg,

        .rx_i,
        .tx_rready_i,
        .tx_o,
        .rx_wready_o,

        .intr_tx_watermark_o,
        .intr_rx_watermark_o,
        .intr_tx_empty_o
    );

    ////////////
    // Alerts //
    ////////////

    assign alert_test = {
        reg2hw.alert_test.q & 
        reg2hw.alert_test.qe
    };

    for (genvar i = 0; i < NumAlerts; i++) begin : gen_alert_tx
        prim_alert_sender #(
            .AsyncOn(AlertAsyncOn[i]),
            .IsFatal(1'b1)
        ) u_prim_alert_sender (
            .clk_i,
            .rst_ni,
            .alert_test_i  ( alert_test[i] ),
            .alert_req_i   ( alerts[i]      ),
            .alert_ack_o   (               ),
            .alert_state_o (               ),
            .alert_rx_i    ( alert_rx_i[i] ),
            .alert_tx_o    ( alert_tx_o[i] )
        );
    end
endmodule