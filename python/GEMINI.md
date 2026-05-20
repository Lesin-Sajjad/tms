# TMS (Transportation Management System)

## Project Overview
TMS is a new software project, currently in its initial setup phase. The workspace is initialized with a Python 3.12 virtual environment, suggesting a Python-based development stack.

### Technologies
- **Language:** Python 3.12
- **Environment Management:** Python `venv` (located in `.env/`)

## Building and Running
Since the project is in its early stages and no source code is present yet, the primary commands are related to environment management.

### Environment Setup
To activate the virtual environment and start development:

```bash
# On Linux/macOS
source .env/bin/activate

# On Windows
.env\Scripts\activate
```

### Installing Dependencies
Once the environment is active, use `pip` to install packages:

```bash
pip install <package-name>
```

## Development Conventions
- **Virtual Environment:** Always use the provided `.env` directory for local development to ensure dependency isolation.
- **Dependency Tracking:** (Recommendation) Maintain a `requirements.txt` or `pyproject.toml` file in the root directory as the project grows.
- **Source Code:** Place all application logic in a structured directory (e.g., `src/` or `tms/`).
