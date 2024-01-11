import tkinter
import tkinter as tk
from tkinter import ttk
from ChatBot.Constants import *
import re

class Dialog(tk.Tk):
    def __init__(self):
        super().__init__()

        self.info_provider = None
        self.lines2details = dict()
        self.cur_line = 0

        self.title("Please ask the question")
        self.geometry("800x1000")

        self.bottom_frame = tk.Frame(self)
        self.bottom_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        self.entry = tk.Entry(self.bottom_frame)
        self.entry.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        self.entry.insert(0, "How the camera focus is processed?")
        self.submit_button = tk.Button(self.bottom_frame, text="Submit query", command=self.submit_query)
        self.submit_button.pack(side=tk.RIGHT, padx=5)

        self.log = tk.Text(self)
        self.log.pack(expand=True, fill='both', side=tk.LEFT)
        self.log.tag_configure("color_tag", foreground="green")
        self.scrollbar = tk.Scrollbar(self)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.log.yview)
        # Bind the item click event
        self.log.bind('<Double-1>', self.show_details)
        self.item_to_highlight = None

        #self.add_log_entry("Response: The code contains the closely related part to the question. The method related to processing the camera focus is the `autoFocus()` method in the `RasterCapturer` class.", None)

    def clean(self):
        self.log.delete("1.0", tk.END)

        self.lines2details.clear()
        self.cur_line = 0


    def set_provider(self, info_provider):
        self.info_provider = info_provider

    def submit_query(self):
        assert self.info_provider
        text = self.entry.get()

        if text:
            self.submit_button.config(state=tk.DISABLED)
            self.clean()

            try:
                self.info_provider.get_answer(text)
            finally:
                self.submit_button.config(state=tk.NORMAL)

            self.submit_button.config(state=tk.NORMAL)

        self.entry.delete(0, tk.END)

    def add_log_entry(self, summary, details, kind):
        if kind == ITEM_TO_HIGHLIGHT:
            self.item_to_highlight = summary

        self.log.insert('end', summary + "\n")
        self.lines2details[self.cur_line] = details
        self.cur_line += 1

        if kind != NORMAL_TEXT:
            self.log.tag_add("color_tag", str(self.cur_line) + ".0", str(self.cur_line) + "." + str(len(summary)))

        self.update()

    def highlight_text(self, widget, text):
        lines = widget.get("1.0", tk.END).split("\n")

        for i, line in enumerate(lines):
            if line.find(text) != -1:
                widget.tag_add("color_tag", str(i + 1) + ".0", str(i + 1) + "." + str(len(line)))

        self.update()

    def show_details(self, event):
        index = self.log.index(tk.CURRENT)
        line_number = int(index.split('.')[0]) - 1

        # Check if the item clicked is a parent item
        if line_number in self.lines2details:
            # Create a new Toplevel window
            detail_window = tk.Toplevel(self)
            detail_window.title("Log Details")
            detail_window.geometry("1000x1000")
            # Create a text widget and insert the details
            text = tk.Text(detail_window, wrap='word')
            text.insert(tk.END, self.lines2details[line_number])
            text.pack(expand=True, side=tk.LEFT, fill='both', padx=5, pady=5)
            text.tag_configure("color_tag", foreground="green")
            scrollbar = tk.Scrollbar(detail_window)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            text.config(yscrollcommand=scrollbar.set)
            scrollbar.config(command=text.yview)

            # Make the text widget read-only
            text.config(state=tk.DISABLED)
            # Add a close button
            #close_button = tk.Button(detail_window, text="Close", command=detail_window.destroy)
            #close_button.pack(side=tk.BOTTOM, pady=5)

            if self.item_to_highlight:
                self.highlight_text(text, self.item_to_highlight)