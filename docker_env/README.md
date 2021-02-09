# Guide to use dockerfile

## LightFTP
Build with
```shell
docker build -t aflnet_legion:LightFTP --no-cache --rm .
```
In `docker_env`, run with
```shell
rm tmp.cid ../log.ansi; docker run --security-opt seccomp=unconfined  -v=/path/to/AFLNet_Legion:/home/ubuntu/AFLNet_Legion -ti --cidfile tmp.cid  aflnet_legion:LightFTP
```

