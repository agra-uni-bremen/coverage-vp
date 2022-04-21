# coverage-vp

Modified version of [SymEx-VP][symex-vp github] with Concolic Line Coverage support.

## Installation

Refer to the original [installation][symex-vp install]
instructions. Contrary to vanilla SymEx-VP, this repository also
bundles a slightly modified version of [clover][clover github].

## Acknowledgements

This work was supported in part by the German Federal Ministry of
Education and Research (BMBF) within the project Scale4Edge under
contract no. 16ME0127 and within the project VerSys under contract
no. 01IW19001.

## License

The original riscv-vp code is licensed under MIT (see `LICENSE.MIT`).
All modifications made for the integration of symbolic execution with
riscv-vp are licensed under GPLv3+ (see `LICENSE.GPL`). Consult the
copyright headers of individual files for more information.

[symex-vp github]: https://github.com/agra-uni-bremen/symex-vp
[symex-vp install]: https://github.com/agra-uni-bremen/symex-vp/blob/7fd4dbaba2dac28b9c51fd1c3edfa78ac112c668/README.md#installation
[clover github]: https://github.com/agra-uni-bremen/clover
