# Guide to use dockerfile

## LightFTP
In `docker_env`, build with
```shell
docker build -t aflnet_legion:LightFTP --no-cache --rm .
```

In `docker_env`, run with
```shell
docker run --security-opt seccomp=unconfined  -v=/<directory to AFLNetLegion>/AFLNet_Legion/:/home/ubuntu/AFLNet_Legion -ti donggeliu/aflnet_legion:LightFTP
```

In docker container:

Build with:
```bash
./Setup
```

Run & debug with:
```bash
./LightFTP_gdb
```
