// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Description: CMOD core module

module cmod_core 
    import cmod_reg_pkg::*;
    import cmod_pkg::*;
(
    input           clk_i,
    input           rst_ni,

    input  cmod_reg_pkg::cmod_reg2hw_t reg2hw,
    output cmod_reg_pkg::cmod_hw2reg_t hw2reg,

    input  cmod_rcv_t   rx_i,
    input  logic        tx_rready_i,
    output cmod_req_t   tx_o,
    output logic        rx_wready_o,

    output logic    intr_tx_watermark_o,
    output logic    intr_rx_watermark_o,
    output logic    intr_tx_empty_o
);

    // CTRL register bits/fields
    logic       txtrigger_q, txend_q, txend_q_buf, rxconfirm_q;
    logic [1:0] txilvl_q, rxilvl_q;

    // STATUS register bits/fields
    logic       tx_d, tx_d_prev, txfull_d, rxfull_d, txempty_d, txinput_ready_d, rxvalid_d, rxlast_d;
    logic [2:0] txlvl_d, rxlvl_d;

    // WDATA/RDATA registers
    logic [127:0]           wdata_q, rdata_d;
    logic [NumRegsData-1:0] wdata_qe, wdata_qe_buf, rdata_re, rdata_re_buf;
    logic                   wdata_qe_all, wdata_qe_all_prev, rdata_re_all, rdata_re_all_prev;


    logic         tx_fifo_wready, tx_fifo_rvalid, tx_fifo_rvalid_prev, txlast;
    logic [127:0] tx_fifo_data_out;

    logic         rx_fifo_wvalid, rx_fifo_wready, rx_fifo_rvalid, rxlast, rxlast_buf;
    logic [127:0] rx_fifo_data_in;

    logic         tx_watermark, tx_watermark_prev;
    logic         rx_watermark, rx_watermark_prev;

    logic         event_tx_watermark, event_tx_empty, event_rx_watermark;


    // Get bits/fields from CTRL register
    assign txtrigger_q = reg2hw.ctrl.txtrigger.q;
    assign txend_q     = reg2hw.ctrl.txend.q;
    assign rxconfirm_q = reg2hw.ctrl.rxconfirm.q;
    assign txilvl_q    = reg2hw.ctrl.txilvl.q;
    assign rxilvl_q    = reg2hw.ctrl.rxilvl.q;

    // Set bits/fields of STATUS register
    assign hw2reg.status.tx.d            = tx_d;
    assign hw2reg.status.txfull.d        = txfull_d;
    assign hw2reg.status.rxfull.d        = rxfull_d;
    assign hw2reg.status.txempty.d       = txempty_d;
    assign hw2reg.status.txinput_ready.d = txinput_ready_d;
    assign hw2reg.status.rxvalid.d       = rxvalid_d;
    assign hw2reg.status.txlvl.d         = txlvl_d;
    assign hw2reg.status.rxlvl.d         = rxlvl_d;
    assign hw2reg.status.rxlast.d        = rxlast_d;

    // Get data from WDATA registers
    assign wdata_q      = {reg2hw.wdata[3].q, reg2hw.wdata[2].q, reg2hw.wdata[1].q, reg2hw.wdata[0].q};
    assign wdata_qe     = { (reg2hw.wdata[3].qe | (wdata_qe_buf[3] & ~wdata_qe_all_prev)) & tx_d & ~txend_q & ~txend_q_buf,
                            (reg2hw.wdata[2].qe | (wdata_qe_buf[2] & ~wdata_qe_all_prev)) & tx_d & ~txend_q & ~txend_q_buf,
                            (reg2hw.wdata[1].qe | (wdata_qe_buf[1] & ~wdata_qe_all_prev)) & tx_d & ~txend_q & ~txend_q_buf,
                            (reg2hw.wdata[0].qe | (wdata_qe_buf[0] & ~wdata_qe_all_prev)) & tx_d & ~txend_q & ~txend_q_buf};
    assign wdata_qe_all = wdata_qe[3] & wdata_qe[2] & wdata_qe[1] & wdata_qe[0];

    // Write data to RDATA registers
    assign hw2reg.rdata[3].d = rdata_d[127:96];
    assign hw2reg.rdata[2].d = rdata_d[95:64];
    assign hw2reg.rdata[1].d = rdata_d[63:32];
    assign hw2reg.rdata[0].d = rdata_d[31:0];
    assign rdata_re          = { reg2hw.rdata[3].re | (rdata_re_buf[3] & ~rdata_re_all_prev),
                                 reg2hw.rdata[2].re | (rdata_re_buf[2] & ~rdata_re_all_prev),
                                 reg2hw.rdata[1].re | (rdata_re_buf[1] & ~rdata_re_all_prev),
                                 reg2hw.rdata[0].re | (rdata_re_buf[0] & ~rdata_re_all_prev)};
    assign rdata_re_all      = rdata_re[3] & rdata_re[2] & rdata_re[1] & rdata_re[0];

    // Reset txtrigger, txend, and rxconfirm bits of CTRL register
    assign hw2reg.ctrl.txtrigger.d  = 1'b0;
    assign hw2reg.ctrl.txtrigger.de = 1'b1;
    assign hw2reg.ctrl.txend.d      = 1'b0;
    assign hw2reg.ctrl.txend.de     = 1'b1;
    assign hw2reg.ctrl.rxconfirm.d  = 1'b0;
    assign hw2reg.ctrl.rxconfirm.de = 1'b1;

    // Set values for inter module connection
    assign tx_o.data   = tx_fifo_data_out;
    assign tx_o.last   = txlast;
    assign tx_o.valid  = tx_fifo_rvalid;
    assign rx_wready_o = rx_fifo_wready & ~rxlast & ~rxlast_buf;

    // Get values from inter module connection
    assign rx_fifo_data_in = rx_i.data;
    assign rx_fifo_wvalid  = rx_i.valid & ~rxlast & ~rxlast_buf;
    assign rxlast          = rx_i.last;


    // Set internal values
    assign tx_d            = (tx_d_prev | txtrigger_q) & ~txlast;
    assign txfull_d        = ~tx_fifo_wready;
    assign rxfull_d        = ~rx_fifo_wready;
    assign txempty_d       = ~tx_fifo_rvalid;
    assign txinput_ready_d = tx_d & ~txend_q & ~txend_q_buf & tx_fifo_wready;
    

    always_comb begin
        unique case (txlvl_d)
            3'b0:    txlast = txend_q | txend_q_buf;
            default: txlast = 1'b0; 
        endcase
    end

    always_comb begin
        unique case (rxlvl_d)
            3'b0:    rxlast_d = rxlast | rxlast_buf;
            default: rxlast_d = 1'b0;
        endcase
    end
    

    ///////////
    // FIFOs //
    ///////////

    prim_fifo_sync #(
        .Width  (128),
        .Pass   (1'b0),
        .Depth  (4)
    ) u_cmod_tx_fifo (
        .clk_i,
        .rst_ni,
        .clr_i    (),
        .wvalid_i ( wdata_qe_all     ),
        .wready_o ( tx_fifo_wready   ),
        .wdata_i  ( wdata_q          ),
        .rvalid_o ( tx_fifo_rvalid   ),
        .rready_i ( tx_rready_i      ),
        .rdata_o  ( tx_fifo_data_out ),
        .full_o   (),
        .depth_o  ( txlvl_d          ),
        .err_o    ()
    );

    prim_fifo_sync #(
        .Width  (128),
        .Pass   (1'b0),
        .Depth  (4)
    ) u_cmod_rx_fifo (
        .clk_i,
        .rst_ni,
        .clr_i    (),
        .wvalid_i ( rx_fifo_wvalid  ),
        .wready_o ( rx_fifo_wready  ),
        .wdata_i  ( rx_fifo_data_in ),
        .rvalid_o ( rxvalid_d       ),
        .rready_i ( rdata_re_all    ),
        .rdata_o  ( rdata_d         ),
        .full_o   (),
        .depth_o  ( rxlvl_d         ),
        .err_o    ()
    );

    ////////////////////////
    // Interrupt & Status //
    ////////////////////////

    always_comb begin
        unique case(txilvl_q)
            2'b0:    tx_watermark = (txlvl_d < 3'd2);
            2'b1:    tx_watermark = (txlvl_d < 3'd3);
            default: tx_watermark = (txlvl_d < 3'd4);
        endcase
    end

    assign event_tx_watermark = tx_watermark & ~tx_watermark_prev;
    assign event_tx_empty     = ~tx_fifo_rvalid & tx_fifo_rvalid_prev;

    always_comb begin
        unique case(rxilvl_q)
            2'd0:    rx_watermark = (rxlvl_d >= 3'd1);
            2'd1:    rx_watermark = (rxlvl_d >= 3'd2);
            2'd2:    rx_watermark = (rxlvl_d >= 3'd3);
            default: rx_watermark = 1'b0;
        endcase
    end

    assign event_rx_watermark = rx_watermark & ~rx_watermark_prev;

    always_ff @( posedge clk_i or negedge rst_ni ) begin
        if (!rst_ni) begin
            tx_watermark_prev   <= 1'b1;
            rx_watermark_prev   <= 1'b0;
            wdata_qe_buf        <= 4'b0;
            wdata_qe_all_prev   <= 1'b0;
            rdata_re_buf        <= 4'b0;
            rdata_re_all_prev   <= 1'b0;
            tx_fifo_rvalid_prev <= 1'b0;
            tx_d_prev           <= 1'b0;
            txend_q_buf         <= 1'b0;
            rxlast_buf          <= 1'b0;
        end else begin
            tx_watermark_prev   <= tx_watermark;
            rx_watermark_prev   <= rx_watermark;
            wdata_qe_buf        <= wdata_qe;
            wdata_qe_all_prev   <= wdata_qe_all;
            rdata_re_buf        <= rdata_re;
            rdata_re_all_prev   <= rdata_re_all;
            tx_fifo_rvalid_prev <= tx_fifo_rvalid;
            tx_d_prev           <= tx_d;
            txend_q_buf         <= (txend_q | txend_q_buf) & ~txlast;
            rxlast_buf          <= (rxlast | rxlast_buf) & ~rxconfirm_q;
        end
    end

    // Instantiate interrupt hardware primitives
    prim_intr_hw #(.Width(1)) intr_hw_tx_watermark (
        .clk_i,
        .rst_ni,
        .event_intr_i           ( event_tx_watermark                ),
        .reg2hw_intr_enable_q_i ( reg2hw.intr_enable.tx_watermark.q ),
        .reg2hw_intr_test_q_i   ( reg2hw.intr_test.tx_watermark.q   ),
        .reg2hw_intr_test_qe_i  ( reg2hw.intr_test.tx_watermark.qe  ),
        .reg2hw_intr_state_q_i  ( reg2hw.intr_state.tx_watermark.q  ),
        .hw2reg_intr_state_de_o ( hw2reg.intr_state.tx_watermark.de ),
        .hw2reg_intr_state_d_o  ( hw2reg.intr_state.tx_watermark.d  ),
        .intr_o                 ( intr_tx_watermark_o               )
    );

    prim_intr_hw #(.Width(1)) intr_hw_rx_watermark (
        .clk_i,
        .rst_ni,
        .event_intr_i           ( event_rx_watermark                ),
        .reg2hw_intr_enable_q_i ( reg2hw.intr_enable.rx_watermark.q ),
        .reg2hw_intr_test_q_i   ( reg2hw.intr_test.rx_watermark.q   ),
        .reg2hw_intr_test_qe_i  ( reg2hw.intr_test.rx_watermark.qe  ),
        .reg2hw_intr_state_q_i  ( reg2hw.intr_state.rx_watermark.q  ),
        .hw2reg_intr_state_de_o ( hw2reg.intr_state.rx_watermark.de ),
        .hw2reg_intr_state_d_o  ( hw2reg.intr_state.rx_watermark.d  ),
        .intr_o                 ( intr_rx_watermark_o               )
    );

    prim_intr_hw #(.Width(1)) intr_hw_tx_empty (
        .clk_i,
        .rst_ni,
        .event_intr_i           ( event_tx_empty                    ),
        .reg2hw_intr_enable_q_i ( reg2hw.intr_enable.tx_empty.q     ),
        .reg2hw_intr_test_q_i   ( reg2hw.intr_test.tx_empty.q       ),
        .reg2hw_intr_test_qe_i  ( reg2hw.intr_test.tx_empty.qe      ),
        .reg2hw_intr_state_q_i  ( reg2hw.intr_state.tx_empty.q      ),
        .hw2reg_intr_state_de_o ( hw2reg.intr_state.tx_empty.de     ),
        .hw2reg_intr_state_d_o  ( hw2reg.intr_state.tx_empty.d      ),
        .intr_o                 ( intr_tx_empty_o                   )
    );
endmodule