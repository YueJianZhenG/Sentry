#!/bin/bash

tmux new-session -d

tmux split-window -h
tmux split-window -v

tmux split-window -h
tmux split-window -v
tmux select-layout even-vertical #垂直布局
#tmux select-layout even-horizontal #水平布局


# Run a process in each pane
tmux send-keys -t 0 './app ./config/config.json RegistryServer' C-m
tmux send-keys -t 1 './app ./config/config.json DataServer' C-m
tmux send-keys -t 2 './app ./config/config.json HttpServer' C-m
tmux send-keys -t 3 './app ./config/config.json ChatServer' C-m
tmux send-keys -t 4 './app ./config/config.json GateServer' C-m

# Attach to the tmux session to view the output
tmux attach-session