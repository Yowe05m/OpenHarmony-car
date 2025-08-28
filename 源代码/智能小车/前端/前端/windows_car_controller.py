import tkinter as tk
from tkinter import ttk, messagebox
import socket
import threading
import time
from tkinter import font

class CarController:
    def __init__(self, root):
        self.root = root
        self.root.title("智能小车TCP控制器")
        self.root.geometry("400x600") 
        self.root.configure(bg="#1E293B")
        
        self.default_font = font.nametofont("TkDefaultFont")
        self.default_font.configure(family="SimHei", size=10)
        self.root.option_add("*Font", self.default_font)
        
        self.is_connected = False
        self.client_socket = None
        
        self.create_widgets()
        
    def create_widgets(self):
        # ... (标题和连接配置区域代码保持不变) ...
        title_frame = tk.Frame(self.root, bg="#165DFF", height=50)
        title_frame.pack(fill=tk.X)
        title_label = tk.Label(title_frame, text="智能小车TCP控制器", bg="#165DFF", fg="white", font=("SimHei", 16, "bold"))
        title_label.pack(pady=10)

        config_frame = tk.Frame(self.root, bg="#334155", padx=15, pady=15)
        config_frame.pack(fill=tk.X)
        input_frame = tk.Frame(config_frame, bg="#334155")
        input_frame.pack(fill=tk.X)
        tk.Label(input_frame, text="IP地址:", bg="#334155", fg="white").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.ip_entry = tk.Entry(input_frame, width=15, bg="#1E293B", fg="white", insertbackground="white", relief=tk.FLAT, bd=0, highlightthickness=1, highlightbackground="#165DFF")
        self.ip_entry.insert(0, "192.168.155.112")
        self.ip_entry.grid(row=0, column=1, padx=5, pady=5, sticky=tk.W)
        tk.Label(input_frame, text="端口号:", bg="#334155", fg="white").grid(row=0, column=2, padx=5, pady=5, sticky=tk.W)
        self.port_entry = tk.Entry(input_frame, width=8, bg="#1E293B", fg="white", insertbackground="white", relief=tk.FLAT, bd=0, highlightthickness=1, highlightbackground="#165DFF")
        self.port_entry.insert(0, "5678")
        self.port_entry.grid(row=0, column=3, padx=5, pady=5, sticky=tk.W)
        self.connect_btn = tk.Button(config_frame, text="连接", command=self.toggle_connection, bg="#165DFF", fg="white", relief=tk.FLAT, font=("SimHei", 10, "bold"), padx=10, pady=5)
        self.connect_btn.pack(pady=5, anchor=tk.E)
        status_frame = tk.Frame(config_frame, bg="#334155")
        status_frame.pack(fill=tk.X, pady=5)
        self.status_indicator = tk.Canvas(status_frame, width=10, height=10, bg="#334155", highlightthickness=0)
        self.status_indicator.grid(row=0, column=0, padx=5)
        self.status_dot = self.status_indicator.create_oval(2, 2, 10, 10, fill="red")
        self.status_var = tk.StringVar()
        self.status_var.set("未连接")
        status_label = tk.Label(status_frame, textvariable=self.status_var, bg="#334155", fg="white")
        status_label.grid(row=0, column=1, sticky=tk.W)
        
        # === 新增：速度控制区域 ===
        speed_control_frame = tk.Frame(self.root, bg="#1E293B", padx=15, pady=10)
        speed_control_frame.pack(fill=tk.X)

        tk.Label(speed_control_frame, text="速度调节:", bg="#1E293B", fg="white", font=("SimHei", 12)).pack(side=tk.LEFT, padx=(0, 10))

        self.speed_var = tk.IntVar(value=80)
        self.speed_slider = tk.Scale(speed_control_frame, from_=0, to=100, orient=tk.HORIZONTAL,
                                     variable=self.speed_var,
                                     bg="#334155", fg="white", troughcolor="#1E293B",
                                     highlightthickness=0, bd=0,
                                     command=self.update_speed_label)
        self.speed_slider.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.speed_label = tk.Label(speed_control_frame, text="80%", bg="#1E293B", fg="#36D399", font=("SimHei", 12, "bold"))
        self.speed_label.pack(side=tk.LEFT, padx=(10, 0))
        # ==========================
        
        control_frame = tk.Frame(self.root, bg="#1E293B", padx=15, pady=15)
        control_frame.pack(fill=tk.BOTH, expand=True)
        
        btn_config = {'width': 8, 'height': 3, 'font': ('SimHei', 12, 'bold'), 'relief': tk.FLAT, 'bd': 0, 'highlightthickness': 0, 'activebackground': '#36D399', 'activeforeground': 'white'}
        direction_style = {'bg': '#36D399', 'fg': 'white'}
        stop_style = {'bg': '#F87272', 'fg': 'white'}
        
        # === 更新按钮指令 ===
        self.forward_btn = tk.Button(control_frame, text="前进", **btn_config, **direction_style)
        self.forward_btn.grid(row=0, column=1, padx=5, pady=5, sticky=tk.NSEW)
        self.forward_btn.bind("<ButtonPress-1>", lambda e: self.send_move_command("F"))
        self.forward_btn.bind("<ButtonRelease-1>", lambda e: self.send_command("S"))
        
        self.backward_btn = tk.Button(control_frame, text="后退", **btn_config, **direction_style)
        self.backward_btn.grid(row=2, column=1, padx=5, pady=5, sticky=tk.NSEW)
        self.backward_btn.bind("<ButtonPress-1>", lambda e: self.send_move_command("B"))
        self.backward_btn.bind("<ButtonRelease-1>", lambda e: self.send_command("S"))
        
        self.left_btn = tk.Button(control_frame, text="左转", **btn_config, **direction_style)
        self.left_btn.grid(row=1, column=0, padx=5, pady=5, sticky=tk.NSEW)
        self.left_btn.bind("<ButtonPress-1>", lambda e: self.send_move_command("L"))
        self.left_btn.bind("<ButtonRelease-1>", lambda e: self.send_command("S"))
        
        self.right_btn = tk.Button(control_frame, text="右转", **btn_config, **direction_style)
        self.right_btn.grid(row=1, column=2, padx=5, pady=5, sticky=tk.NSEW)
        self.right_btn.bind("<ButtonPress-1>", lambda e: self.send_move_command("R"))
        self.right_btn.bind("<ButtonRelease-1>", lambda e: self.send_command("S"))
        
        self.stop_btn = tk.Button(control_frame, text="停止", **btn_config, **stop_style)
        self.stop_btn.grid(row=1, column=1, padx=5, pady=5, sticky=tk.NSEW)
        self.stop_btn.bind("<ButtonPress-1>", lambda e: self.send_command("S"))
        # ==========================

        for i in range(3): control_frame.columnconfigure(i, weight=1)
        for i in range(3): control_frame.rowconfigure(i, weight=1)

        log_frame = tk.Frame(self.root, bg="#334155", padx=15, pady=15)
        log_frame.pack(fill=tk.BOTH, expand=True)
        tk.Label(log_frame, text="操作日志:", bg="#334155", fg="white").pack(anchor=tk.W, pady=(0, 5))
        self.log_text = tk.Text(log_frame, height=5, width=50, bg="#1E293B", fg="white", bd=0, highlightthickness=0, wrap=tk.WORD)
        self.log_text.pack(fill=tk.BOTH, expand=True)
        scrollbar = tk.Scrollbar(self.log_text, command=self.log_text.yview, bg="#334155")
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.config(yscrollcommand=scrollbar.set)
        
        self.disable_control_buttons()

    def update_speed_label(self, value):
        self.speed_label.config(text=f"{value}%")
    
    # 发送带速度的移动指令
    def send_move_command(self, direction):
        speed = self.speed_var.get()
        command = f"{direction}{speed}"
        self.send_command(command)

    def toggle_connection(self):
        if not self.is_connected:
            try:
                ip = self.ip_entry.get()
                port = int(self.port_entry.get())
                self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                #self.client_socket.settimeout(5)
                self.client_socket.connect((ip, port))
                self.is_connected = True
                self.connect_btn.config(text="断开", bg="#F87272")
                self.status_var.set(f"已连接到 {ip}:{port}")
                self.status_indicator.itemconfig(self.status_dot, fill="green")
                self.log(f"已连接到 {ip}:{port}")
                self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
                self.receive_thread.start()
                self.enable_control_buttons()
            except Exception as e:
                messagebox.showerror("连接错误", f"无法连接到 {ip}:{port}\n错误: {str(e)}")
                self.log(f"连接失败: {str(e)}")
        else:
            try:
                if self.client_socket:
                    self.client_socket.close()
                self.is_connected = False
                self.connect_btn.config(text="连接", bg="#165DFF")
                self.status_var.set("未连接")
                self.status_indicator.itemconfig(self.status_dot, fill="red")
                self.log("已断开连接")
                self.disable_control_buttons()
            except Exception as e:
                self.log(f"断开连接时出错: {str(e)}")

    def send_command(self, command):
        if not self.is_connected:
            self.log("未连接，无法发送命令")
            return
            
        try:
            self.client_socket.send(command.encode())
            
            # === 修改：更新日志信息 ===
            action_map = {'F': '前进', 'B': '后退', 'L': '左转', 'R': '右转', 'S': '停止'}
            action_char = command[0]
            action_name = action_map.get(action_char, '未知')

            if action_char in 'FBLR':
                speed = command[1:]
                log_message = f"发送命令: {action_name}, 速度: {speed}%"
            else:
                log_message = f"发送命令: {action_name}"
            self.log(log_message)
            # ==========================
            
        except Exception as e:
            self.log(f"发送命令失败: {str(e)}")
            self.toggle_connection()

    # ... (其余方法 receive_data, log, enable/disable_control_buttons, animate_button 保持不变) ...
    def receive_data(self):
        while self.is_connected:
            try:
                data = self.client_socket.recv(1024)
                if not data: break
                response = data.decode()
                self.log(f"收到回应: {response}")
            except Exception as e:
                if self.is_connected:
                    self.log(f"接收数据失败: {str(e)}")
                    self.root.after(0, self.toggle_connection)
                break
    
    def log(self, message):
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def enable_control_buttons(self):
        for btn in [self.forward_btn, self.backward_btn, self.left_btn, self.right_btn, self.stop_btn]:
            btn.config(state=tk.NORMAL)
        self.speed_slider.config(state=tk.NORMAL) # 启用滑块
    
    def disable_control_buttons(self):
        for btn in [self.forward_btn, self.backward_btn, self.left_btn, self.right_btn, self.stop_btn]:
            btn.config(state=tk.DISABLED)
        self.speed_slider.config(state=tk.DISABLED) # 禁用滑块

if __name__ == "__main__":
    root = tk.Tk()
    app = CarController(root)
    root.mainloop()