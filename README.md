# End-to-End On-Chip Encryption

This repository is a fork of the [OpenTitan project](https://github.com/lowRISC/opentitan) at commit [b7c7b3f](https://github.com/lowRISC/opentitan/commit/b7c7b3f237fcf58441c7335f1d3f8bab22561a0e), with the additions presented in the corresponding bachelor's thesis.

For more information about OpenTitan, see the [OpenTitan site](https://opentitan.org) and [OpenTitan docs](https://docs.opentitan.org).

## Communication Module (CMOD)

The implementation of the CMOD IP can be found at: [/hw/ip/cmod](/hw/ip/cmod).

The project also includes the following:

- the integration of the CMOD IP core into [EarlGrey](/hw/top_earlgrey),
- the [CMOD Functionality Test](/sw/device/tests/cmod_functionality_test.c), and
- the [CMOD Performance Test](/sw/device/tests/cmod_perftest.c).

## Reproducibility

To reproduce the results of the performance test presented in the bachelor's thesis, run the following command after OpenTitan is up and running:

```
python3 reproduce.py
```
