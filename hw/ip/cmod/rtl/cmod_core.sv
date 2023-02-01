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
    output logic    intr_tx_empty_o,
    output logic    intr_rx_overflow_o
);

    logic           tx, tx_prev;
    logic           txtrigger, txend; // TODO: `txend` not used yet.
    logic [1:0]     txilvl, rxilvl;

    logic [127:0]   wdata, rdata;
    logic [3:0]     wdata_qe, wdata_qe_prev, rdata_re, rdata_re_prev;
    logic           wdata_all_qe, wdata_all_qe_prev, rdata_all_re, rdata_all_re_prev;

    logic           tx_fifo_rst, tx_fifo_wready, tx_fifo_rvalid, tx_fifo_rvalid_prev, tx_fifo_rready; // TODO: check if tx_fifo_rvalid_prev is necessary
    logic [127:0]   tx_fifo_data_out;
    logic [2:0]     tx_fifo_depth;

    logic           rx_fifo_rst, rx_fifo_wvalid, rx_fifo_wready, rx_fifo_rvalid;
    logic [127:0]   rx_fifo_data_in;
    logic [2:0]     rx_fifo_depth;

    logic tx_watermark, tx_watermark_prev;
    logic rx_watermark, rx_watermark_prev;

    logic event_tx_watermark, event_tx_empty, event_rx_watermark, event_rx_overflow;

    assign tx           = tx_prev | txtrigger;
    assign txtrigger    = reg2hw.ctrl.txtrigger.q;
    assign txend        = reg2hw.ctrl.txend.q;
    assign txilvl       = reg2hw.ctrl.txilvl.q;
    assign rxilvl       = reg2hw.ctrl.rxilvl.q;

    assign wdata        = {reg2hw.wdata[3].q, reg2hw.wdata[2].q, reg2hw.wdata[1].q, reg2hw.wdata[0].q};
    assign wdata_qe     = { (reg2hw.wdata[3].qe | (wdata_qe_prev[3] & ~wdata_all_qe_prev)) & tx,
                            (reg2hw.wdata[2].qe | (wdata_qe_prev[2] & ~wdata_all_qe_prev)) & tx,
                            (reg2hw.wdata[1].qe | (wdata_qe_prev[1] & ~wdata_all_qe_prev)) & tx,
                            (reg2hw.wdata[0].qe | (wdata_qe_prev[0] & ~wdata_all_qe_prev)) & tx};
    assign wdata_all_qe = wdata_qe[3] & wdata_qe[2] & wdata_qe[1] & wdata_qe[0];

    assign hw2reg.rdata[3].d = rdata[127:96];
    assign hw2reg.rdata[2].d = rdata[95:64];
    assign hw2reg.rdata[1].d = rdata[63:32];
    assign hw2reg.rdata[0].d = rdata[31:0];
    assign rdata_re = { reg2hw.rdata[3].re | (rdata_re_prev[3] & ~rdata_all_re_prev),
                        reg2hw.rdata[2].re | (rdata_re_prev[2] & ~rdata_all_re_prev),
                        reg2hw.rdata[1].re | (rdata_re_prev[1] & ~rdata_all_re_prev),
                        reg2hw.rdata[0].re | (rdata_re_prev[0] & ~rdata_all_re_prev)};
    assign rdata_all_re = rdata_re[3] & rdata_re[2] & rdata_re[1] & rdata_re[0];

    assign hw2reg.status.tx.d       = tx;
    assign hw2reg.status.txfull.d   = ~tx_fifo_wready;
    assign hw2reg.status.rxfull.d   = ~rx_fifo_wready;
    assign hw2reg.status.txempty.d  = ~tx_fifo_rvalid;
    assign hw2reg.status.rxvalid.d  = rx_fifo_rvalid;
    assign hw2reg.status.txlvl.d    = tx_fifo_depth;
    assign hw2reg.status.rxlvl.d    = rx_fifo_depth;

    assign tx_fifo_rst      = 1'b0; // TODO: `tx_fifo_rst` is unused at the moment
    assign rx_fifo_rst      = 1'b0; // TODO: `rx_fifo_rst` is unused at the moment

    assign tx_o.data        = tx_fifo_data_out;
    assign tx_o.valid       = tx_fifo_rvalid;
    assign rx_fifo_data_in  = rx_i.data;
    assign rx_fifo_wvalid   = rx_i.valid;
    assign tx_fifo_rready   = tx_rready_i;
    assign rx_wready_o      = rx_fifo_wready;
    

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
        .clr_i    ( tx_fifo_rst     ),
        .wvalid_i ( wdata_all_qe    ),
        .wready_o ( tx_fifo_wready  ),
        .wdata_i  ( wdata           ),
        .rvalid_o ( tx_fifo_rvalid  ),
        .rready_i ( tx_fifo_rready  ),
        .rdata_o  ( tx_fifo_data_out),
        .full_o   (),
        .depth_o  ( tx_fifo_depth   ),
        .err_o    ()
    );

    prim_fifo_sync #(
        .Width  (128),
        .Pass   (1'b0),
        .Depth  (4)
    ) u_cmod_rx_fifo (
        .clk_i,
        .rst_ni,
        .clr_i    ( rx_fifo_rst        ),
        .wvalid_i ( rx_fifo_wvalid     ),
        .wready_o ( rx_fifo_wready     ),
        .wdata_i  ( rx_fifo_data_in    ),
        .rvalid_o ( rx_fifo_rvalid     ),
        .rready_i ( rdata_all_re       ),
        .rdata_o  ( rdata              ),
        .full_o   (),
        .depth_o  ( rx_fifo_depth      ),
        .err_o    ()
    );

    ////////////////////////
    // Interrupt & Status //
    ////////////////////////

    always_comb begin
        unique case(txilvl)
            2'b0:    tx_watermark = (tx_fifo_depth < 3'd2);
            2'b1:    tx_watermark = (tx_fifo_depth < 3'd3);
            default: tx_watermark = (tx_fifo_depth < 3'd4);
        endcase
    end

    assign event_tx_watermark = tx_watermark & ~tx_watermark_prev;
    assign event_tx_empty     = ~tx_fifo_rvalid & tx_fifo_rvalid_prev;

    always_comb begin
        unique case(rxilvl)
            2'd0:    rx_watermark = (rx_fifo_depth >= 3'd1);
            2'd1:    rx_watermark = (rx_fifo_depth >= 3'd2);
            2'd2:    rx_watermark = (rx_fifo_depth >= 3'd3);
            default: rx_watermark = 1'b0;
        endcase
    end

    assign event_rx_watermark = rx_watermark & ~rx_watermark_prev;
    assign event_rx_overflow  = rx_fifo_wvalid & ~rx_fifo_wready;

    always_ff @( posedge clk_i or negedge rst_ni ) begin
        if (!rst_ni) begin
            tx_watermark_prev   <= 1'b1;
            rx_watermark_prev   <= 1'b0;
            wdata_qe_prev       <= 4'b0;
            wdata_all_qe_prev   <= 1'b0;
            rdata_re_prev       <= 4'b0;
            rdata_all_re_prev   <= 1'b0;
            tx_fifo_rvalid_prev <= 1'b0;
            tx_prev             <= 1'b0;
        end else begin
            tx_watermark_prev   <= tx_watermark;
            rx_watermark_prev   <= rx_watermark;
            wdata_qe_prev       <= wdata_qe;
            wdata_all_qe_prev   <= wdata_all_qe;
            rdata_re_prev       <= rdata_re;
            rdata_all_re_prev   <= rdata_all_re;
            tx_fifo_rvalid_prev <= tx_fifo_rvalid;
            tx_prev             <= tx;
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

    prim_intr_hw #(.Width(1)) intr_hw_rx_empty (
        .clk_i,
        .rst_ni,
        .event_intr_i           ( event_rx_overflow                ),
        .reg2hw_intr_enable_q_i ( reg2hw.intr_enable.rx_overflow.q ),
        .reg2hw_intr_test_q_i   ( reg2hw.intr_test.rx_overflow.q   ),
        .reg2hw_intr_test_qe_i  ( reg2hw.intr_test.rx_overflow.qe  ),
        .reg2hw_intr_state_q_i  ( reg2hw.intr_state.rx_overflow.q  ),
        .hw2reg_intr_state_de_o ( hw2reg.intr_state.rx_overflow.de ),
        .hw2reg_intr_state_d_o  ( hw2reg.intr_state.rx_overflow.d  ),
        .intr_o                 ( intr_rx_overflow_o               )
    );
endmodule