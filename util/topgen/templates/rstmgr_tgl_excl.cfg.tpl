// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// rstmgr_tgl_excl.cfg generated by `topgen.py` tool
${gencmd}
<%
import topgen.lib as lib
unused_resets = lib.get_unused_resets(top)
%>\
//=========================================================
// This file contains resets that are not used at top level
//=========================================================
% for reset in unused_resets :
-node tb.dut*.u_rstmgr_aon.resets_o.${reset}
% endfor
