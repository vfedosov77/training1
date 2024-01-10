import tkinter
import tkinter as tk
from tkinter import ttk

class Dialog(tk.Tk):
    def __init__(self):
        super().__init__()

        self.info_provider = None
        self.items2details = dict()

        self.title("Please ask the question")
        self.geometry("800x1000")

        self.bottom_frame = tk.Frame(self)
        self.bottom_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        self.entry = tk.Entry(self.bottom_frame)
        self.entry.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        self.submit_button = tk.Button(self.bottom_frame, text="Submit query", command=self.submit_query)
        self.submit_button.pack(side=tk.RIGHT, padx=5)

        self.tree = ttk.Treeview(self)
        self.tree.pack(expand=True, fill='both', side=tk.TOP)

        # Define columns
        self.tree['columns'] = ('summary')

        # Format columns
        self.tree.column('#0', width=0, stretch=tk.NO)
        self.tree.column('summary', anchor=tk.W, width=800)

        # Create headings
        self.tree.heading('#0', text='', anchor=tk.W)
        self.tree.heading('summary', text='Summary', anchor=tk.W)

        # Bind the item click event
        self.tree.bind('<Double-1>', self.show_details)

    def clean(self):
        for child in self.tree.get_children():
            self.tree.delete(child)

        self.items2details.clear()


    def set_provider(self, info_provider):
        self.info_provider = info_provider

    def submit_query(self):
        assert self.info_provider
        text = self.entry.get()

        if text:
            self.submit_button.config(state=tk.DISABLED)
            self.clean()
            self.info_provider.get_answer(text)

            self.submit_button.config(state=tk.NORMAL)

        self.entry.delete(0, tk.END)

    def add_log_entry(self, summary, details):
        # Add a parent item
        new_item = self.tree.insert('', 'end', text=summary, values=(summary, '[Click to expand]'))
        self.items2details[new_item] = details
        self.update()

    def show_details(self, event):
        item = self.tree.selection()[0]
        # Check if the item clicked is a parent item
        if not self.tree.parent(item):
            # Create a new Toplevel window
            detail_window = tk.Toplevel(self)
            detail_window.title("Log Details")
            detail_window.geometry("1000x1000")
            # Create a text widget and insert the details
            text = tk.Text(detail_window, wrap='word')
            text.insert(tk.END, self.items2details[item])
            text.pack(expand=True, fill='both', padx=5, pady=5)
            # Make the text widget read-only
            text.config(state=tk.DISABLED)
            # Add a close button
            close_button = tk.Button(detail_window, text="Close", command=detail_window.destroy)
            close_button.pack(side=tk.BOTTOM, pady=5)