# Security Examples

Examples for implementing NCSA IoT Cybersecurity compliance.

## Target Audience

| Role                   | Responsibility              | NCSA Level Focus |
| ---------------------- | --------------------------- | ---------------- |
| **Security Engineer**  | Implement security controls | Level 2-3        |
| **DevSecOps Engineer** | CI/CD security integration  | Level 3-4        |
| **IT Admin**           | Certificate/key management  | Level 1-2        |
| **Compliance Officer** | Audit and reporting         | Level 4          |

## NCSA Compliance Levels

```
Level 1: Security Baseline
├── TLS 1.2+ encryption
├── API Key authentication
└── Password policies

Level 2: Foundational Security
├── Mutual TLS (mTLS)
├── Vault secret management
└── JWT authentication

Level 3: Advanced Security
├── HSM key provisioning
├── Protected Update workflow
└── SBOM generation

Level 4: Comprehensive Governance
├── Security audit automation
├── Zero Trust gateway
└── Quantum-ready crypto
```

---

## Examples

### Identity & SSO

| Example                                              | NCSA Level | Description              | Status |
| ---------------------------------------------------- | ---------- | ------------------------ | ------ |
| [identity-sso](./identity-sso/)                      | Level 2    | Keycloak SSO integration | NEW    |
| [keycloak-react](./identity-sso/keycloak-react/)     | Level 2    | React SSO with PKCE      | Ready  |
| [keycloak-python](./identity-sso/keycloak-python/)   | Level 2    | Python CLI & FastAPI SSO | Ready  |
| [keycloak-flutter](./identity-sso/keycloak-flutter/) | Level 2    | Flutter mobile SSO       | Ready  |

### NCSA Level 1-2

| Example                                              | NCSA Level | Description             | Status  |
| ---------------------------------------------------- | ---------- | ----------------------- | ------- |
| [device-mtls](./ncsa-level-1-2/device-mtls/)         | Level 2    | Mutual TLS reference    | Link    |
| [password-policy](./ncsa-level-1-2/password-policy/) | Level 1    | Password policy example | Planned |

### NCSA Level 3

| Example                                                                | NCSA Level | Description        | Status  |
| ---------------------------------------------------------------------- | ---------- | ------------------ | ------- |
| [protected-update-workflow](./ncsa-level-3/protected-update-workflow/) | Level 3    | Trust M OID update | Planned |
| [sbom-generator](./ncsa-level-3/sbom-generator/)                       | Level 3    | CycloneDX SBOM     | Planned |

### NCSA Level 4

| Example                                                          | NCSA Level | Description                 | Status  |
| ---------------------------------------------------------------- | ---------- | --------------------------- | ------- |
| [vault-secrets-manager](./ncsa-level-4/vault-secrets-manager/)   | Level 4    | HashiCorp Vault integration | NEW     |
| [security-audit-scripts](./ncsa-level-4/security-audit-scripts/) | Level 4    | Automated compliance checks | Planned |

---

## Key Skills

- PKI and certificate management
- Vault secret rotation
- NCSA compliance implementation
- Security audit automation
- HSM/Secure Element integration

---

## NCSA Compliance Checklist

### Level 1 - Security Baseline

- [ ] TLS 1.2+ for all connections
- [ ] API keys stored securely
- [ ] Password policy (min 8 chars, complexity)
- [ ] Rate limiting enabled

### Level 2 - Foundational Security

- [ ] Mutual TLS for device auth
- [ ] Vault for secret management
- [ ] JWT with RS256 signing
- [ ] RBAC implemented

### Level 3 - Advanced Security

- [ ] HSM for key storage
- [ ] Signed firmware updates
- [ ] SBOM for all components
- [ ] Vulnerability scanning

### Level 4 - Comprehensive Governance

- [ ] Automated security audits
- [ ] Zero Trust architecture
- [ ] Incident response plan
- [ ] Compliance reporting

---

## References

- [NCSA IoT Cybersecurity Guideline 2024](https://www.ncsa.or.th)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)

---

**Category:** Security
**Last Updated:** 2025-12-27
