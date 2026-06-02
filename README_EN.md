# sonic-cpp Patch Repository

## Latest Updates

- 2026-03-30: Released patch repository version v1.0.0 based on the master branch commit `69deb02`, supporting SVE2-256 optimization for the Kunpeng 950 processor.

## Project Introduction

sonic-cpp is an efficient JSON library for C++ developed by ByteDance. It leverages current CPU hardware characteristics and vectorized programming to significantly enhance the performance of JSON serialization and deserialization.

This project is a patch repository contributed by Huawei Kunpeng to the sonic-cpp open-source community. It optimizes the serialization and deserialization functions in sonic-cpp using vectorized instruction sets specific to Huawei's Kunpeng processors, making it suitable for the Kunpeng 950 processor. The function interfaces remain consistent with the open-source version of sonic-cpp.

## Directory Structure

```text
sonic-cpp/
├── docs/                           # Documentation directory
│   ├── en/                         # English documentation
│   │   ├── public_sys-resources/   # Public system resources
│   │   ├── api_reference.md        # API Reference
│   │   ├── quick_start.md          # Quick Start
│   │   └── release_notes.md        # Release Notes
│   └── LICENSE
├── 950-sve2-256.patch              # Kunpeng 950 SVE2-256 optimization patch
├── LICENSE
└── README_EN.md
```

## Release Notes

For details, see [Release Notes](docs/en/release_notes.md).

## Quick Start

For details, see [Quick Start](docs/en/quick_start.md).

## Documents

| Name| Description|
|---------|---------|
| [Quick Start](docs/en/quick_start.md)| Provides guidance on how to install and enable the sonic-cpp patch.|
| [Release Notes](docs/en/release_notes.md)| Provides basic information and feature updates of the released version of the sonic-cpp patch repository.|
| [API Reference](docs/en/api_reference.md)| Provides API descriptions and API calling examples.|

## Disclaimer

This code repository contributes to the sonic-cpp open-source project solely for performance optimization of certain sonic-cpp functions on Kunpeng processors. It strictly adheres to the coding style and methods, as well as security design of the native open-source software. Any vulnerability and security issues of the software shall be resolved by the corresponding upstream communities according to their response mechanisms. Please pay attention to the notifications and version updates released by the upstream communities. The Kunpeng computing community does not assume any responsibility for software vulnerabilities and security issues.

## License

The sonic-cpp patch repository is licensed under Apache-2.0. For details, see [LICENSE](https://gitcode.com/boostkit/sonic-cpp/blob/master/LICENSE).

The documents of this project are licensed under CC-BY 4.0. For details, see [LICENSE](docs/LICENSE).

## Contributions

If you have any questions or want to provide feedback on feature requirements and bug reports, you can submit issues. For details, see the [contribution guideline](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md).

## Suggestions and Feedback

You are welcome to contribute to the community. If you have any questions or suggestions, submit [issues](https://gitcode.com/boostkit/community/blob/master/docs/contributor/issue-submit.md). We will reply to you as soon as possible. Thank you for your support.

## Acknowledgments

The sonic-cpp patch repository is jointly developed by the following Huawei department:

 - Kunpeng Computing BoostKit Development Dept

Thank you to everyone in the community for your PRs. We warmly welcome contributions to the sonic-cpp patch repository!
