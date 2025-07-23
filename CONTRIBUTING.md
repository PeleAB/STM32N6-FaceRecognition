# Contributing to STM32N6 Face Recognition System

We welcome contributions from the embedded AI community! This guide will help you get started with contributing to the project.

## 🤝 How to Contribute

### Types of Contributions

- **🐛 Bug Reports**: Help us identify and fix issues
- **✨ Feature Requests**: Suggest new functionality
- **📝 Documentation**: Improve guides and API docs
- **🔧 Code Contributions**: Implement features and fixes
- **🧪 Testing**: Add test cases and improve coverage
- **🎨 Examples**: Create tutorials and demo applications

## 🚀 Getting Started

### Prerequisites

1. **Hardware**: STM32N6570-DK development board
2. **Software**: 
   - STM32CubeIDE or ARM GCC toolchain
   - Git
   - Python 3.8+ (for tools)
3. **Knowledge**: 
   - C programming
   - Embedded systems
   - Basic neural networks understanding

### Development Setup

1. **Fork and Clone**:
   ```bash
   git clone https://github.com/yourusername/EdgeAI_Workshop.git
   cd EdgeAI_Workshop
   ```

2. **Install Dependencies**:
   ```bash
   # Python tools (optional)
   cd python_tools
   pip install -r requirements.txt
   ```

3. **Build and Test**:
   ```bash
   make clean && make -j4
   make flash  # Test on hardware
   ```

## 📋 Development Guidelines

### Code Style

#### C Code Style
- **Indentation**: 4 spaces (no tabs)
- **Naming**: 
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_CASE`
  - Types: `snake_case_t`
- **Comments**: Use `/** */` for functions, `//` for inline
- **Line Length**: Max 100 characters

#### Example:
```c
/**
 * @brief Calculate cosine similarity between embeddings
 * @param emb1 First embedding vector
 * @param emb2 Second embedding vector
 * @param len Vector length
 * @return Similarity score (-1.0 to 1.0)
 */
float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len)
{
    if (!emb1 || !emb2 || len == 0) {
        return 0.0f;
    }
    
    // Implementation here...
}
```

#### Python Code Style
- Follow **PEP 8**
- Use **type hints** where appropriate
- **Docstrings** for all public functions
- **Black** formatter recommended

### Commit Messages

Use [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

#### Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code restructuring
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

#### Examples:
```
feat(face_detection): add multi-scale detection support

Implement pyramid-based detection for better small face detection.
Improves detection accuracy by ~15% on challenging datasets.

Closes #123
```

```
fix(memory): resolve buffer overflow in image processing

The RGB conversion function was writing beyond buffer bounds
when processing images with non-standard strides.

Fixes #456
```

## 🐛 Reporting Issues

### Bug Reports

Use the [Bug Report Template](.github/ISSUE_TEMPLATE/bug_report.md):

**Required Information:**
- STM32N6 board revision
- Firmware version
- Steps to reproduce
- Expected vs actual behavior
- Debug output/logs
- Photos/videos if applicable

### Feature Requests

Use the [Feature Request Template](.github/ISSUE_TEMPLATE/feature_request.md):

**Include:**
- Clear use case description
- Proposed solution
- Alternative solutions considered
- Implementation complexity estimate

## 🔧 Making Changes

### Workflow

1. **Create Branch**:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/issue-description
   ```

2. **Make Changes**:
   - Follow coding standards
   - Add tests where appropriate
   - Update documentation
   - Test on hardware

3. **Commit Changes**:
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

4. **Push and Create PR**:
   ```bash
   git push origin feature/your-feature-name
   # Then create Pull Request on GitHub
   ```

### Pull Request Guidelines

#### Before Submitting
- [ ] Code compiles without warnings
- [ ] All tests pass (hardware and software)
- [ ] Documentation updated
- [ ] Commit messages follow convention
- [ ] No merge conflicts

#### PR Description Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature  
- [ ] Documentation update
- [ ] Performance improvement

## Testing
- [ ] Tested on STM32N6570-DK hardware
- [ ] Unit tests added/updated
- [ ] Performance regression tested

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No breaking changes (or clearly documented)
```

## 🧪 Testing

### Hardware Testing

Always test changes on actual hardware:

```bash
# Build and flash
make clean && make -j4
make flash

# Test basic functionality
# - Camera input working
# - Face detection functioning  
# - Recognition accuracy maintained
# - No memory leaks or crashes
```

### Software Testing

```bash
# Python tools testing
cd python_tools
python -m pytest tests/ -v

# Static analysis (if available)
cppcheck --enable=all Src/ Inc/
```

### Performance Testing

Monitor key metrics:
- **Memory usage**: RAM and Flash consumption
- **Processing time**: Face detection and recognition latency
- **Power consumption**: Current draw measurements
- **Accuracy**: Detection and recognition rates

## 📖 Documentation

### Code Documentation
- All public functions must have Doxygen comments
- Complex algorithms need detailed explanations
- Configuration options documented in headers

### User Documentation
- Update README.md for user-facing changes
- Add examples to `Exercises/` directory
- Update technical documentation in `Doc/`

### API Documentation
Generate documentation:
```bash
doxygen Doxyfile  # If Doxyfile exists
```

## 🔍 Code Review Process

### Review Criteria
- **Functionality**: Does it work as intended?
- **Code Quality**: Readable, maintainable, efficient
- **Testing**: Adequate test coverage
- **Documentation**: Clear and complete
- **Performance**: No significant regressions
- **Security**: No security vulnerabilities

### Review Timeline
- Initial review: Within 3-5 business days
- Follow-up reviews: Within 2 business days
- Maintainer approval required for merge

## 🏷️ Release Process

### Versioning
We use [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

### Release Checklist
- [ ] All tests passing
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Version numbers updated
- [ ] Release notes prepared

## 🆘 Getting Help

### Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and ideas
- **Pull Request Comments**: Code-specific discussions

### Maintainer Contact
For urgent issues or private concerns, contact the maintainers directly.

## 📜 License

By contributing, you agree that your contributions will be licensed under the same license as the project. See [LICENSE](LICENSE) for details.

## 🙏 Recognition

Contributors will be:
- Listed in project documentation
- Mentioned in release notes
- Added to CONTRIBUTORS.md file

Thank you for helping make this project better! 🚀

---

**Questions?** Feel free to ask in [GitHub Discussions](../../discussions) or open an issue.