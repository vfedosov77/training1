import tkinter
import tkinter as tk
from tkinter import ttk

class Dialog(tk.Tk):
    def __init__(self):
        super().__init__()

        self.info_provider = None

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
        self.tree['columns'] = ('summary', 'details')

        # Format columns
        self.tree.column('#0', width=0, stretch=tk.NO)
        self.tree.column('summary', anchor=tk.W, width=400)
        self.tree.column('details', anchor=tk.W, width=400)

        # Create headings
        self.tree.heading('#0', text='', anchor=tk.W)
        self.tree.heading('summary', text='Summary', anchor=tk.W)
        self.tree.heading('details', text='Details', anchor=tk.W)

        # Define a tag to hide details by default
        self.tree.tag_configure('hidden', image='')

        # Bind the item click event
        self.tree.bind('<Double-1>', self.toggle_details)

    def set_provider(self, info_provider):
        self.info_provider = info_provider

    def submit_query(self):
        assert self.info_provider
        text = self.entry.get()

        if text:
            self.submit_button.config(state=tk.DISABLED)

            for child in self.tree.get_children():
                self.tree.delete(child)

            self.info_provider.get_answer(text)

            self.submit_button.config(state=tk.NORMAL)

        self.entry.delete(0, tk.END)

    def add_log_entry(self, summary, details):
        # Add a parent item
        parent = self.tree.insert('', 'end', text='', values=(summary, '[Click to expand]'), tags=('hidden',))
        # Add a child item with details
        self.tree.insert(parent, 'end', text='', values=('', details), tags=('hidden',))
        self.update()

    def toggle_details(self, event):
        item = self.tree.selget_childrenection()[0]
        if self.tree.parent(item):  # Ignore clicks on child items
            return
        if self.tree.tag_has('hidden', item):
            # Show details
            self.tree.item(item, tags=())
            self.tree.set(item, 'details', '')
        else:
            # Hide details
            self.tree.item(item, tags=('hidden',))
            self.tree.set(item, 'details', '[Click to expand]')
            for child in self.tree.get_children(item):
                self.tree.detach(child)
            self.tree.reattach(item, '', 0)