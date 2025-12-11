---
title: "Contributing"
permalink: /contributing/
toc: true
---

# Contributing to TESAIoT Developer Hub

We welcome contributions from the community! This guide will help you get started.

## Ways to Contribute

1. **Report Bugs**: Found an issue? [Open a bug report](https://github.com/tesaiot/developer-hub/issues/new?template=bug_report.md)
2. **Suggest Features**: Have an idea? [Open a feature request](https://github.com/tesaiot/developer-hub/issues/new?template=feature_request.md)
3. **Add Examples**: Create new examples for the community
4. **Improve Docs**: Fix typos, clarify instructions, add tutorials
5. **Review PRs**: Help review pull requests from other contributors

## Code of Conduct

Please be respectful and constructive. We're all here to learn and build great things.

## Development Setup

### Prerequisites

- Git
- Node.js 18+ (for JavaScript/TypeScript examples)
- Python 3.10+ (for Python examples)
- Docker (for testing containers)

### Clone the Repository

```bash
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub
```

### Testing Examples

Each example can be tested independently:

```bash
cd examples/<example-name>

# Python examples
python -m pytest tests/

# JavaScript examples
npm test

# Docker
docker build -t test-example .
docker run --rm test-example
```

## Adding a New Example

### 1. Create Directory Structure

```bash
mkdir -p examples/my-example/{src,tests,docs}
```

### 2. Required Files

Every example must include:

| File | Description |
|------|-------------|
| `README.md` | Setup instructions and usage |
| `Dockerfile` | Container deployment |
| `.env.example` | Configuration template |
| `ARCHITECTURE.md` | Technical design (optional) |

### 3. README Template

```markdown
# Example Name

Brief description of what this example does.

## Features

- Feature 1
- Feature 2

## Quick Start

### Prerequisites

- Required software

### Installation

\`\`\`bash
# Installation commands
\`\`\`

### Configuration

\`\`\`bash
cp .env.example .env
# Edit .env with your values
\`\`\`

### Running

\`\`\`bash
# Run commands
\`\`\`

## API Reference

Document any APIs exposed by the example.

## License

Apache 2.0 - See [LICENSE](../../LICENSE)

## Credits

Built with [TESAIoT Platform Examples](https://github.com/tesaiot/developer-hub)
```

### 4. Code Style

- **Python**: Follow PEP 8, use type hints
- **JavaScript/TypeScript**: Use ESLint + Prettier
- **Comments**: Write clear, helpful comments
- **Tests**: Include basic tests where applicable

### 5. Docker Requirements

```dockerfile
# Multi-stage build preferred
FROM node:20-alpine AS builder
# ... build steps

FROM node:20-alpine AS production
# ... production setup
```

## Pull Request Process

### 1. Fork and Branch

```bash
# Fork on GitHub, then:
git clone https://github.com/YOUR-USERNAME/developer-hub.git
git checkout -b feature/my-new-example
```

### 2. Make Changes

- Write clear commit messages
- Keep changes focused and atomic
- Update documentation

### 3. Test Your Changes

```bash
# Test the example works
cd examples/my-example
./test.sh  # or equivalent

# Test Docker build
docker build -t test .
docker run --rm test
```

### 4. Submit PR

- Fill out the PR template
- Reference any related issues
- Request review from maintainers

### 5. Review Process

- Maintainers will review within 5 business days
- Address feedback promptly
- Once approved, PR will be merged

## Documentation Contributions

### Local Preview

```bash
cd docs
bundle install
bundle exec jekyll serve
# Open http://localhost:4000/developer-hub/
```

### Writing Guidelines

- Use clear, concise language
- Include code examples
- Add screenshots where helpful
- Keep technical jargon to a minimum

## Questions?

- **GitHub Discussions**: [Ask questions](https://github.com/tesaiot/developer-hub/discussions)
- **Issues**: [Report problems](https://github.com/tesaiot/developer-hub/issues)

Thank you for contributing to TESAIoT Developer Hub! 🚀
