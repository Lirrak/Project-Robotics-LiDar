import time
import math
import sys

# Try importing pyserial, fallback to mockup if not installed
try:
    import serial
except ImportError:
    serial = None

class RPiESP32Bridge:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        
        # Kinematic constants
        self.wheel_radius = 0.0325 # 65mm diameter
        self.wheel_base = 0.160    # 160mm wheel spacing
        
        self.connect()

    def connect(self):
        if serial is not None:
            try:
                self.ser = serial.Serial(self.port, self.baudrate, timeout=0.1)
                print(f"[Bridge] Successfully connected to ESP32 on port {self.port} at {self.baudrate} baud.")
            except Exception as e:
                print(f"[Bridge] Warning: Could not open serial port {self.port}: {e}")
                print("[Bridge] Running in Simulation Mode (No physical serial port).")
                self.ser = None
        else:
            print("[Bridge] pyserial not installed. Running in simulation mode.")

    def send_cmd_vel(self, v_linear, w_angular):
        """
        Send target linear velocity v (m/s) and angular velocity w (rad/s) to ESP32.
        Command string format sent via UART: 'CMD,v,w\n'
        """
        cmd_str = f"CMD,{v_linear:.3f},{w_angular:.3f}\n"
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(cmd_str.encode('ascii'))
            except Exception as e:
                print(f"[Bridge] Error sending command to ESP32: {e}")
        else:
            # Print debug for simulation
            pass

    def read_telemetry_and_update_odometry(self, dt=0.05):
        """
        Reads TELE lines from ESP32: 'TELE,velL,velR,ticksL,ticksR'
        Updates dead-reckoning odometry (x, y, theta).
        """
        if not self.ser or not self.ser.is_open:
            return None

        try:
            while self.ser.in_waiting > 0:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startsWith("TELE,"):
                    parts = line.split(',')
                    if len(parts) >= 5:
                        velL = float(parts[1])
                        velR = float(parts[2])
                        
                        # Forward kinematics
                        v = (velR + velL) / 2.0
                        w = (velR - velL) / self.wheel_base
                        
                        # Update odometry pose
                        self.theta += w * dt
                        # Normalize theta to [-pi, pi]
                        self.theta = math.atan2(math.sin(self.theta), math.cos(self.theta))
                        
                        self.x += v * math.cos(self.theta) * dt
                        self.y += v * math.sin(self.theta) * dt
                        
                        return {
                            'x': self.x,
                            'y': self.y,
                            'theta': self.theta,
                            'v': v,
                            'w': w,
                            'velL': velL,
                            'velR': velR
                        }
        except Exception as e:
            print(f"[Bridge] Serial read error: {e}")

        return None

if __name__ == "__main__":
    print("Raspberry Pi 3B+ <-> ESP32 Serial Communication Bridge")
    bridge = RPiESP32Bridge()
    
    # Test driving loop simulation
    print("Testing command sending (Forward 0.2 m/s for 2 seconds)...")
    start = time.time()
    while time.time() - start < 2.0:
        bridge.send_cmd_vel(0.2, 0.0)
        time.sleep(0.1)

    print("Stopping robot...")
    bridge.send_cmd_vel(0.0, 0.0)
    print("Bridge test complete cleanly.")
