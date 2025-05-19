FROM gitpod/workspace-full

RUN sudo apt update && \
    sudo apt install -y gcc g++ binutils cmake nasm && \
    sudo apt clean
