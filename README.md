# coverage-vp

Modified version of [SymEx-VP][symex-vp github] with Concolic Line Coverage support.

More information about coverage-vp is available in the IEEE Embedded System Letters (ESL) journal publication [*Towards Quantification and Visualization of the Effects of Concretization during Concolic Testing*](https://doi.org/10.1109/LES.2022.3171603).

## Installation

Refer to the original [installation][symex-vp install]
instructions. Contrary to vanilla SymEx-VP, this repository also
bundles a slightly modified version of [clover][clover github].

## Usage

coverage-vp is a modified version of SymEx-VP which will generate
coverage information in an enhanced version of the `gcov` JSON format
(see `--json-format` option in `gcov(1)`). The enhanced version of this
format includes information about concretizations performed during
concolic testing. This generated JSON files can be passed to
[jcovr][jcovr github] for visualization purposes. Refer to the
the aforementioned journal publication for more information.

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
[jcovr github]: https://github.com/agra-uni-bremen/jcovr
