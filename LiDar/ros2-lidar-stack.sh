#!/usr/bin/env bash
set -Eeuo pipefail

pid_file=/tmp/ros2-lidar-stack.pid
script_path=/workspace/Project-Robotics-LiDar/LiDar/ros2-lidar-stack.sh

stop_stack() {
    if [[ ! -r "$pid_file" ]]; then
        return 0
    fi

    local supervisor_pid command_line
    read -r supervisor_pid < "$pid_file" || return 0
    if [[ -r "/proc/$supervisor_pid/cmdline" ]]; then
        command_line=$(tr '\0' ' ' < "/proc/$supervisor_pid/cmdline" || true)
        if [[ "$command_line" == *"$script_path run"* ]]; then
            kill -TERM "$supervisor_pid" 2>/dev/null || true
        fi
    fi
}

if [[ "${1:-run}" == stop ]]; then
    stop_stack
    exit 0
fi

if [[ "${1:-run}" != run ]]; then
    printf 'Usage: %s [run|stop]\n' "$0" >&2
    exit 2
fi

if [[ -r "$pid_file" ]]; then
    old_pid=$(<"$pid_file")
    if [[ -r "/proc/$old_pid/cmdline" ]] &&
       tr '\0' ' ' < "/proc/$old_pid/cmdline" | grep -Fq "$script_path run"; then
        echo "ROS2 LiDAR stack is already running (PID $old_pid)." >&2
        exit 1
    fi
    rm -f "$pid_file"
fi

printf '%s\n' "$$" > "$pid_file"
driver_pid=
bridge_pid=

cleanup() {
    local exit_code=$?
    trap - EXIT INT TERM
    for child_pid in "$driver_pid" "$bridge_pid"; do
        if [[ -n "$child_pid" ]]; then
            kill -TERM "$child_pid" 2>/dev/null || true
        fi
    done
    for child_pid in "$driver_pid" "$bridge_pid"; do
        if [[ -n "$child_pid" ]]; then
            wait "$child_pid" 2>/dev/null || true
        fi
    done
    rm -f "$pid_file"
    exit "$exit_code"
}

trap cleanup EXIT
trap 'exit 0' INT TERM

source /opt/ros/jazzy/setup.bash
ros2 launch rplidar_ros rplidar.launch.py &
driver_pid=$!
ros2 launch rosbridge_server rosbridge_websocket_launch.xml &
bridge_pid=$!

wait -n "$driver_pid" "$bridge_pid"