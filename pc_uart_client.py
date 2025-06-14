#!/usr/bin/env python3
"""GUI application to send images and view detection stream over UART."""

import tkinter as tk
from tkinter import filedialog, messagebox
import threading
import queue
import time
import serial

import pc_uart_utils as utils


class App:
    def __init__(self, master):
        self.master = master
        master.title('UART Object Detection Client')

        tk.Label(master, text='Port').grid(row=0, column=0, sticky='e')
        self.port_entry = tk.Entry(master)
        self.port_entry.insert(0, 'COM3')
        self.port_entry.grid(row=0, column=1)

        tk.Label(master, text='Baud').grid(row=1, column=0, sticky='e')
        self.baud_entry = tk.Entry(master)
        self.baud_entry.insert(0, str(921600 * 8))
        self.baud_entry.grid(row=1, column=1)

        tk.Label(master, text='Width').grid(row=2, column=0, sticky='e')
        self.width_entry = tk.Entry(master)
        self.width_entry.insert(0, '224')
        self.width_entry.grid(row=2, column=1)

        tk.Label(master, text='Height').grid(row=3, column=0, sticky='e')
        self.height_entry = tk.Entry(master)
        self.height_entry.insert(0, '224')
        self.height_entry.grid(row=3, column=1)

        self.connect_btn = tk.Button(master, text='Connect', command=self.connect)
        self.connect_btn.grid(row=4, column=0, sticky='ew')
        self.disconnect_btn = tk.Button(master, text='Disconnect', command=self.disconnect, state='disabled')
        self.disconnect_btn.grid(row=4, column=1, sticky='ew')

        self.send_btn = tk.Button(master, text='Send Images', command=self.send_images, state='disabled')
        self.send_btn.grid(row=5, column=0, columnspan=2, sticky='ew')

        self.stream_btn = tk.Button(master, text='Start Stream', command=self.start_stream, state='disabled')
        self.stream_btn.grid(row=6, column=0, columnspan=2, sticky='ew')

        self.stop_btn = tk.Button(master, text='Stop Stream', command=self.stop_stream, state='disabled')
        self.stop_btn.grid(row=7, column=0, columnspan=2, sticky='ew')

        self.ser = None
        self.stop_event = threading.Event()
        self.frame_queue = queue.Queue(maxsize=2)
        self.stream_thread = None
        self.display_thread = None

        master.protocol('WM_DELETE_WINDOW', self.on_close)

    def connect(self):
        try:
            self.ser = serial.Serial(self.port_entry.get(), int(self.baud_entry.get()), timeout=1)
        except Exception as e:
            messagebox.showerror('Connection error', str(e))
            return
        self.connect_btn.config(state='disabled')
        self.disconnect_btn.config(state='normal')
        self.send_btn.config(state='normal')
        self.stream_btn.config(state='normal')

    def disconnect(self):
        self.stop_stream()
        if self.ser:
            self.ser.close()
            self.ser = None
        self.connect_btn.config(state='normal')
        self.disconnect_btn.config(state='disabled')
        self.send_btn.config(state='disabled')
        self.stream_btn.config(state='disabled')

    def send_images(self):
        if not self.ser:
            messagebox.showwarning('Not connected', 'Connect to the device first')
            return
        files = filedialog.askopenfilenames(title='Select image files')
        for img in files:
            utils.send_image(self.ser, img, (int(self.width_entry.get()), int(self.height_entry.get())))
            time.sleep(0.1)

    def stream_loop(self):
        while not self.stop_event.is_set():
            frame, w, h = utils.read_frame(self.ser)
            if frame is None:
                continue
            dets = utils.read_detections(self.ser)
            frame = utils.draw_detections(frame, dets)
            if not self.frame_queue.full():
                self.frame_queue.put(frame)
        self.frame_queue.put(None)

    def start_stream(self):
        if not self.ser:
            messagebox.showwarning('Not connected', 'Connect to the device first')
            return
        self.stop_event.clear()
        self.stream_thread = threading.Thread(target=self.stream_loop, daemon=True)
        self.display_thread = threading.Thread(target=utils.display_loop, args=(self.frame_queue, self.stop_event), daemon=True)
        self.stream_thread.start()
        self.display_thread.start()
        self.stream_btn.config(state='disabled')
        self.stop_btn.config(state='normal')

    def stop_stream(self):
        self.stop_event.set()
        if self.stream_thread:
            self.stream_thread.join()
            self.stream_thread = None
        if self.display_thread:
            self.display_thread.join()
            self.display_thread = None
        self.stop_btn.config(state='disabled')
        self.stream_btn.config(state='normal')

    def on_close(self):
        self.disconnect()
        self.master.destroy()


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == '__main__':
    main()
