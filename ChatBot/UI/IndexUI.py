import tkinter as tk
from ChatBot.Common.Constants import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher

from functools import partial


class IndexUI(tk.Tk):
    def __init__(self, dispatcher: NotificationDispatcher):
        super().__init__()

        self.lines2details = dict()
        self.cur_log_line = 0

        self.history = []

        self.title("Create index")
        self.geometry("800x800")

        self.l2 = tk.Label(text="Log:")
        self.l2.grid(row=0, column=2)

        self.clean_button = tk.Button(self, text="Clean", command=self.clean, width=5)
        self.clean_button.grid(row=2, column=2)

        self.log = tk.Text(self, width=60)
        self.log.grid(row=1, column=2, rowspan=1, sticky="NSEW")
        self.log.tag_configure("color_tag", foreground="green")

        self.scrollbar = tk.Scrollbar(self)
        self.scrollbar.grid(row=1, column=3, rowspan=2, sticky="NSW")
        self.log.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.log.yview)
        # Bind the item click event
        self.log.bind('<Double-1>', self.show_details)
        self.item_to_highlight = None

        self.columnconfigure(2, weight=1)
        self.rowconfigure(1, weight=1)

        dispatcher.add_events_observer(partial(IndexUI.add_log_entry, self))
        dispatcher.add_progress_observer(partial(IndexUI.on_progress, self))
        #self.add_log_entry("Response: The code contains the closely related part to the question. The method related to processing the camera focus is the `autoFocus()` method in the `RasterCapturer` class.", None)

    def clean_log(self):
        self.log.delete("1.0", tk.END)
        self.lines2details.clear()
        self.cur_log_line = 0

    def clean(self):
        self.lines2details.clear()
        self.history.clear()
        self.clean_log()

    def set_tag(self, widget, tag, line, beg, end):
        widget.tag_add(tag, str(line + 1) + "." + str(beg), str(line + 1) + "." + str(end))

    def on_progress(self, module: str, progress: float):
        pass

    def add_log_entry(self, summary, details, kind):
        if kind == ITEM_TO_HIGHLIGHT:
            self.item_to_highlight = summary
            return

        summary = summary.replace("\n", " ")

        self.log.insert('end', summary + "\n")
        self.lines2details[self.cur_log_line] = details
        self.cur_log_line += 1

        self.update()

    def highlight_text(self, widget, text):
        lines = widget.get("1.0", tk.END).split("\n")

        for i, line in enumerate(lines):
            if line.find(text) != -1:
                widget.tag_add("color_tag", str(i + 1) + ".0", str(i + 1) + "." + str(len(line)))

        self.update()

    def show_details(self, event):
        if event.widget == self.log:
            index = self.log.index(tk.CURRENT)
        else:
            assert False

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