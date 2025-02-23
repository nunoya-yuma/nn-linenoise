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

## Try integration test

### Preparation

```shell
sudo apt update
sudo apt install -y python3 python3-venv python3-pip
python3 -m venv venv
source venv/bin/activate
pip install -r test/integration/requirements.txt
```

### Run tests

```shell
cd test/
pytest -vs integration
```
