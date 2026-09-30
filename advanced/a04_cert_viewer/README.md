# A04 — Certificate Viewer

X.509 certificate chain viewer displaying Device, Intermediate CA, and Root CA certificates with parsed ASN.1 fields, validity indicators, and chain visualization.

## What it demonstrates

- Tabbed interface with interactive certificate selection
- Certificate chain visualization with connected status dots
- Parsed X.509 fields: CN, Issuer, Serial, Validity, Algorithm, Key Usage
- DER hex preview of raw certificate data
- Validity countdown with color-coded status indicators

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tap certificate tabs to view different certs in the chain
4. Chain dots highlight the currently selected certificate
