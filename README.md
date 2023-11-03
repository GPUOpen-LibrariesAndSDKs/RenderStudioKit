# AMD RenderStudio Kit
This repo contains all required methods to integrate USD software into AMD RenderStudio ecosystem

## Building
### Dependencies
```bash
pip3 install -r Workspace/requirements.txt
```
> Please note CMake would install pip requirements during build by default. Consider use WITH_PYTHON_DEPENDENCIES_INSTALL option to disable this.
### CMake options
```bash
WITH_SHARED_WORKSPACE_SUPPORT [ON] - Enables shared workspace feature. Requires pyinstaller to be installed
WITH_PYTHON_DEPENDENCIES_INSTALL [ON] - Enables automatic install for all build requirements for Python
USD_LOCATION [""] - Here CMAKE_INSTALL_PREFIX of USD build should be passed
```
### Dockerized build for linux (with main RenderStudio repo downloaded)
```bash
make -C Kit/Tools/ build
```

## Deployment
### Prepare directories on remote
```bash
sudo mkdir -p /opt/rs/ && sudo chown $(whoami) /opt/rs
sudo mkdir -p "/var/lib/AMD/AMD RenderStudio/" && sudo chown $(whoami) "/var/lib/AMD/AMD RenderStudio/"
```
### Deploy from builder
- Set **studio** remote in ~/.ssh/config
```bash
make -C Kit/Tools/ deploy
```
### Nginx config sample
```bash
server {
    # Redirect workspace requests to RenderStudioWorkspace
    location /workspace/ {
        proxy_pass http://127.0.0.1:52700;
    }

    # Redirect live requests to RenderStudioServer
    location /workspace/live/ {
        proxy_pass http://127.0.0.1:52702/;
        proxy_http_version  1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

## Running
### Launch server
```bash
/opt/rs/RenderStudioServer/Run.sh
```
