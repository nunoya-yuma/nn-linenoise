# README

## Setup

```shell
git clone git@github.com:nunoya-yuma/nn-linenoise.git
git submodule update --init
```

## Try sample code

```shell
# Build
NN_LINENOISE=$(git rev-parse --show-toplevel)
cd ${NN_LINENOISE}/examples
cmake -B build -S. -GNinja
cmake --build build

# Run
./build/nn_cli_sample
```

## Try unit test

```shell
# Build
NN_LINENOISE=$(git rev-parse --show-toplevel)
cd ${NN_LINENOISE}
cmake -B build -S. -GNinja
cmake --build build

# Run
./build/test/nn_cli_test
```
