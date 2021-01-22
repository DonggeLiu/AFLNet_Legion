# Guide to use dockerfile

## LightFTP
Build with
```shell
docker build -t aflnet_mcts:LightFTP --no-cache --rm .
```
Run with
```shell
rm tmp.cid AFLNet_MCTS/log.ansi; docker run --security-opt seccomp=unconfined  -v=/path/to/AFLNet_MCTS:/home/ubuntu/AFLNet_MCTS -ti --cidfile tmp.cid  aflnet_mcts:LightFTP
```

