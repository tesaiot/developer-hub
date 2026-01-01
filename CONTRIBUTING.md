# Contributing to TESAIoT Platform Examples

Thank you for your interest in contributing to TESAIoT Platform Examples! This document provides guidelines for contributing.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment for everyone.

## How to Contribute

### Reporting Issues

1. Check if the issue already exists in [GitHub Issues](https://github.com/tesaiot/developer-hub/issues)
2. Create a new issue with:
   - Clear, descriptive title
   - Steps to reproduce (if bug)
   - Expected vs actual behavior
   - Environment details (OS, language version, etc.)

### Submitting Pull Requests

1. **Fork** the repository
2. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes** following our coding standards
4. **Test** your changes thoroughly
5. **Commit** with clear messages:
   ```bash
   git commit -m "feat(example-name): add feature description"
   ```
6. **Push** to your fork
7. **Create a Pull Request** with:
   - Clear description of changes
   - Link to related issue (if any)
   - Screenshots (if UI changes)

### Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat` - New feature
- `fix` - Bug fix
- `docs` - Documentation only
- `style` - Formatting, no code change
- `refactor` - Code change, no feature/fix
- `test` - Adding tests
- `chore` - Maintenance tasks

**Examples:**
```
feat(python-cli): add device export command
fix(mqtt-streaming): handle reconnection timeout
docs(react-dashboard): update installation steps
```

## Coding Standards

### General

- Write clear, self-documenting code
- Add comments explaining "why", not "what"
- Follow existing patterns in the codebase
- Keep functions small and focused

### Python

- Follow [PEP 8](https://pep8.org/)
- Use type hints
- Write docstrings for public functions
- Run `black` for formatting
- Run `flake8` for linting

```python
def process_telemetry(device_id: str, data: dict) -> bool:
    """
    Process incoming telemetry data from device.

    Args:
        device_id: Unique device identifier
        data: Telemetry payload dictionary

    Returns:
        True if processed successfully, False otherwise

    Raises:
        ValueError: If device_id is empty
    """
    # Implementation...
```

### TypeScript/JavaScript

- Use TypeScript where possible
- Follow [Airbnb Style Guide](https://github.com/airbnb/javascript)
- Run `eslint` and `prettier`

```typescript
/**
 * Connect to MQTT broker via WebSocket Secure.
 *
 * @param token - MQTT API token for authentication
 * @returns Promise resolving when connected
 * @throws Error if token format is invalid
 */
async function connect(token: string): Promise<void> {
  // Implementation...
}
```

### C

- Follow [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- Use descriptive variable names
- Add header comments for functions

```c
/**
 * tesaiot_client_connect - Connect to MQTT broker
 * @client: Pointer to client instance
 *
 * Establishes TLS connection and performs MQTT handshake.
 * Blocks until connected or timeout.
 *
 * Return: 0 on success, negative error code on failure
 */
int tesaiot_client_connect(tesaiot_client_t *client);
```

## Documentation Standards

Every example MUST include:

### README.md

- Brief description
- Features list
- Prerequisites
- Quick start (copy-paste ready)
- Configuration table
- Troubleshooting section

### HOWTO.md

- Step-by-step tutorial
- Prerequisites checklist
- Expected output at each step
- Verification commands

### ARCHITECTURE.md

- System diagram (ASCII/Unicode)
- Data flow explanation
- Key files table
- Dependencies list

## Testing Requirements

### Before Submitting

1. **Install dependencies** successfully
2. **Run the example** end-to-end
3. **Test with mock/test credentials** if real ones unavailable
4. **Verify documentation** is accurate

### Test Commands

```bash
# Python
pytest --cov=. tests/

# Node.js
npm test

# C
make test
```

## Security Guidelines

### NEVER commit:

- API keys or tokens
- Certificates or private keys
- Passwords or secrets
- Real device IDs
- Internal URLs

### ALWAYS use:

- `.env.example` for configuration templates
- Placeholder values: `your_api_key_here`
- Generic device IDs: `device-001`
- Public URLs: `mqtt.tesaiot.com`

## Review Process

1. All PRs require at least one approval
2. CI checks must pass
3. Documentation must be updated
4. No secrets in code or history

## Getting Help

- **Questions**: Open a [Discussion](https://github.com/tesaiot/developer-hub/discussions)
- **Bugs**: Open an [Issue](https://github.com/tesaiot/developer-hub/issues)
- **Security**: Email security@tesaiot.com (do not open public issue)

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.

---

Thank you for contributing to TESAIoT Platform! 🙏
