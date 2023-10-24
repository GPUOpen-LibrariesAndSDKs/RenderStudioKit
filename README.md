# AMD RenderStudio Kit
This repo contains all required methods to integrate USD software into AMD RenderStudio ecosystem

## Building
### Dependencies
```bash
pip3 install -r Watchdog/requirements.txt
```
> Please note CMake would install pip requirements during build by default. Consider use WITH_PYTHON_DEPENDENCIES_INSTALL option to disable this.
#### CMake options
```bash
WITH_SHARED_WORKSPACE_SUPPORT [ON] - Enables shared workspace feature. Requires pyinstaller to be installed
WITH_PYTHON_DEPENDENCIES_INSTALL [ON] - Enables automatic install for all build requirements for Python
```
