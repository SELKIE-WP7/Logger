#!/usr/bin/python3
import logging
import msgpack
from SELKIELogger.SLMessages import SLMessageSink

import tkinter as tk
from tkinter import ttk
from tkinter import filedialog as tkfd
from tkinter import messagebox as tkmb

import os
import threading
import queue
from time import sleep

class IOThread:
    def __init__(self, filename, function, queue, onFinish=None, progress=None, stop=None):
        self.filename = filename
        self.function = function
        self.queue = queue
        self.onFinish = onFinish
        self.progress = progress
        self.stop = stop

    def __call__(self):
        res = self.function(self.filename, self.progress, self.stop)
        if self.stop():
            return # We're being stopped, so just finish here
        if res and self.queue:
            self.queue.put(res)
        if self.onFinish:
            self.onFinish()


def getSourceMap(filename, progress=None, stop=None):
    file = open(filename, "rb")
    file.seek(0, os.SEEK_END)
    size = file.tell()
    file.seek(0, os.SEEK_SET)
    unpacker = msgpack.Unpacker(file, unicode_errors='ignore')
    out = SLMessageSink(msglogger=logging.getLogger("Messages"))

    stats = {}
    for msg in unpacker:
        if stop and stop():
                return None
        msg = out.Process(msg, output="raw")
        if msg is None:
            continue

        if not msg.SourceID in stats:
            stats[msg.SourceID] = {}
        if not msg.ChannelID in stats[msg.SourceID]:
            stats[msg.SourceID][msg.ChannelID] = 1
        else:
            stats[msg.SourceID][msg.ChannelID] += 1

        if progress:
            progress(file.tell() / size)
    return (out.SourceMap(), stats)

class SLViewGUI:

    def Button(self, parent, command, config=None, **kwargs):
        """ Wrapper around tk.Button()

        Applies a default configuration and binds commands to both the default
        "command" option and additionally to the return/enter key to match
        expected behaviour.

        Default configuration can be overriden by providing a dictionary of
        options that will be expanded and passed to .configure()
        """
        button_defconfig = {'width': 8}
        #b = tk.Button(parent, command=command, **kwargs)
        b = ttk.Button(parent, command=command)
        b.bind("<Return>", command)
        b.configure(**button_defconfig)
        if config:
            b.configure(**config)
        return b

    def __init__(self, tkroot):
        self.curFileName = tk.StringVar()
        self.curFile = None
        self.curSMap = None
        self.EQ = queue.Queue()
        self.stopEvent = threading.Event()
        self.threads = []

        self.root = tkroot
        self.root.option_add("*tearOff", tk.FALSE)
        # Single frame as child item, to be resized to fit
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        self.MFrame = ttk.Frame(self.root, width=800, height=600, relief=tk.SUNKEN)
        self.MFrame.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.MFrame.columnconfigure(0, weight=1)
        self.MFrame.rowconfigure(0, weight=0)
        self.MFrame.rowconfigure(1, weight=0)
        self.MFrame.rowconfigure(2, weight=1)

        self.file_label = ttk.Label(self.MFrame, anchor=tk.N, justify=tk.CENTER, textvariable=self.curFileName)
        self.file_label.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.progress_bar = ttk.Progressbar(self.MFrame, orient=tk.HORIZONTAL, mode='determinate', maximum=100)
        self.progress_bar.grid(column=0, row=1, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.pane = ttk.PanedWindow(self.MFrame, orient=tk.HORIZONTAL)
        self.sourceViewPane = ttk.LabelFrame(self.pane, text="Sources", width=150)
        self.sourceViewPane.columnconfigure(0, weight=1)
        self.sourceViewPane.rowconfigure(0, weight=1)

        self.channelViewPane = ttk.LabelFrame(self.pane, text="Channels")
        self.channelViewPane.columnconfigure(0, weight=1)
        self.channelViewPane.rowconfigure(0, weight=1)

        self.pane.add(self.sourceViewPane, weight=1)
        self.pane.add(self.channelViewPane, weight=3)
        self.pane.grid(column=0, row=2, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.sourceViewItems = tk.StringVar()
        self.sourceView = tk.Listbox(self.sourceViewPane, listvariable=self.sourceViewItems)
        self.sourceView.bind("<<ListboxSelect>>", lambda e: self.updateChannelView(e))
        self.sourceView.grid(column=0,row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.channelViewItems = tk.StringVar()
        self.channelView = tk.Listbox(self.channelViewPane, listvariable=self.channelViewItems)
        self.channelView.grid(column=0,row=0, sticky=(tk.N, tk.W, tk.E, tk.S))
        self.channelView.bind("<<ListboxSelect>>", lambda e: self.showChannelStats(e))

        ttk.Separator(orient="horizontal").grid(column=0, row=3, sticky=(tk.N, tk.W, tk.E, tk.S))

        statusArea = ttk.Frame(self.MFrame)
        statusArea.grid(column=0, row=4, sticky=(tk.N, tk.W, tk.E, tk.S))
        statusArea.columnconfigure(0, weight=4)
        statusArea.columnconfigure(1, weight=0)

        self.status = ttk.Label(statusArea, text="Ready")
        self.status.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

        self.actionbutton = self.Button(statusArea, config={"text": "Cancel", "state": tk.DISABLED}, command=lambda e=None: self.processCancel(e))
        self.actionbutton.grid(column=1, row=0,  sticky=(tk.N, tk.W, tk.E, tk.S))

        self.menu = tk.Menu(self.MFrame)

        menu_file = tk.Menu(self.menu, name='file')
        menu_file.add_command(label="Load...", underline=0, command=self.load, accelerator="Control + O")
        # Also bind to the accelerator
        self.root.bind("<Control-o>", self.load)

        menu_file.add_command(label="Process", underline=1, command=self.process, state=tk.DISABLED)
        self.root.bind("<<ProcessingComplete>>", self.processComplete)
        self.root.bind("<<ProcessingProgress>>", self.processProgress)

        menu_file.add_command(label="Close", underline=0, command=self.closeFile, state=tk.DISABLED)
        menu_file.add_command(label="Exit", underline=1, command=self.exit)

        self.menu.add_cascade(label="File", menu=menu_file, underline=0)
        self.root.config(menu=self.menu)

    def load(self, event=None):
        name = tkfd.askopenfilename(parent=self.MFrame, title="Select data file:", filetypes=(("All Supported Files", ("*.dat","*.var")), ("Data files", "*.dat"), ("Variable files", "*.var")))
        if name is None or len(name) == 0:
            return
        root, ext = os.path.splitext(name)
        if not (ext.lower() in [".dat",".var"]):
            tkmb.showwarning(title="Unexpected file extension", message=f"{os.path.basename(root)}{ext} has an unexpected extension.\nThis may not be a supported file type")
        if (ext.lower() == ".dat") and os.path.exists(root + ".var"):
            if tkmb.askyesno(title="Variable file", message="A variable information (.var) file with a name matching the data file selected is available. Process the variable file instead?"):
                name = root + ".var"

        self.closeFile(event)

        self.curFile = name
        self.curFileName.set(name)
        self.menu.children["file"].entryconfigure("Close", state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Process", state=tk.NORMAL)

        self.root.title(f"SLView: {os.path.basename(name)}")
        self.process(event)

    def process(self, event=None):
        if self.curFile is None:
            return

        self.stopEvent.clear()
        newIO = IOThread(self.curFile, getSourceMap, queue=self.EQ,
                onFinish=lambda: self.root.event_generate("<<ProcessingComplete>>"),
                progress=lambda x: self.root.event_generate("<<ProcessingProgress>>", state=int(10000*x)),
                stop=self.stopEvent.is_set
                )
        self.progress_bar.configure(value=0)
        self.progress_bar.start()
        newIOThread = threading.Thread(target=newIO, name="getSourceMap")
        self.threads.append(newIOThread)
        self.actionbutton.configure(state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Load...", state=tk.DISABLED)
        self.menu.children["file"].entryconfigure("Process", state=tk.DISABLED)
        self.menu.children["file"].entryconfigure("Close", state=tk.DISABLED)
        newIOThread.start()
    
    def processCancel(self, event=None):
        self.endThreads()

        self.progress_bar.stop()
        self.progress_bar.configure(value=100)
        self.menu.children["file"].entryconfigure("Load...", state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Process", state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Close", state=tk.NORMAL)
        self.actionbutton.configure(state=tk.DISABLED)
        self.status.configure(text="Processing cancelled")

        for i in self.threads:
            if not i.is_alive():
                self.threads.remove(i)

    def processComplete(self, event=None):
        self.curSMap, self.curStats = self.EQ.get()

        self.progress_bar.stop()
        self.progress_bar.configure(value=100)
        self.menu.children["file"].entryconfigure("Load...", state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Process", state=tk.NORMAL)
        self.menu.children["file"].entryconfigure("Close", state=tk.NORMAL)
        self.actionbutton.configure(state=tk.DISABLED)

        for i in self.threads:
            if not i.is_alive():
                self.threads.remove(i)
        self.updateSourceView()
        self.updateChannelView()

    def processProgress(self, event=None):
        progress = event.state/100
        self.progress_bar.configure(value=progress)
        self.status.configure(text=f"Processing file: {progress:.1f}% loaded")

    def updateSourceView(self, event=None):
        if self.curSMap is None:
            return
        csmd = self.curSMap.to_dict()
        selectedSource = self.sourceView.curselection()
        items = [f"0x{x:02x}: {self.curSMap[x].name}" for x in csmd.keys()]
        if str(items)[0] != self.sourceViewItems.get():
            self.sourceViewItems.set(items)

    def updateChannelView(self, event=None):
        if self.curSMap is None:
            return
        csmd = self.curSMap.to_dict()
        selectedSource = self.sourceView.curselection()
        if len(selectedSource) < 1:
           return 

        self.sourceID = list(csmd.keys())[selectedSource[0]]
        self.channelViewItems.set([f"0x{x[0]:02x}: {x[1]}" for x in list(enumerate(csmd[self.sourceID]))])
        self.showChannelStats()

    def showChannelStats(self, event=None):
        if self.curStats is None or self.curSMap is None:
            return

        csmd = self.curSMap.to_dict()

        selectedChannel = self.channelView.curselection()

        if len(selectedChannel) != 1:
            self.status.configure(text="")
            return

        channelID = selectedChannel[0]

        try:
            cstat = self.curStats[self.sourceID][channelID]
            self.status.configure(text=f"{self.curStats[self.sourceID][channelID]:d} messages seen for source 0x{self.sourceID:02x} and channel 0x{channelID:02x}")
        except Exception as e:
            self.status.configure(text=f"No messages seen for source 0x{self.sourceID:02x} and channel 0x{channelID:02x} - {e}")

    def closeFile(self, event=None):
        if self.stopEvent:
            self.stopEvent.set()
        if self.curFile:
            self.curFile = None

        self.curFileName.set(None)
        self.channelViewItems.set("")
        self.sourceViewItems.set("")
        self.curSMap = None
        self.curStats = None

        self.progress_bar.configure(value=0)

        self.menu.children["file"].entryconfigure("Process", state=tk.DISABLED)
        self.menu.children["file"].entryconfigure("Close", state=tk.DISABLED)
        self.root.title("SLView")

    def endThreads(self):
        self.stopEvent.set()
        # Even with the stopEvent set, we deadlock on a simple join, so issue
        # short timeouts with additional calls to self.root.update() to allow
        # TK events to be processed while this function is blocking the main
        # loop
        for i in self.threads:
            while i.is_alive():
                i.join(0.1)
                self.root.update()

    def exit(self, event=None):
        self.closeFile()
        self.endThreads()
        self.root.destroy()

def SLView():
    root = tk.Tk()
    root.geometry("800x600")
    try:
        ttk.Style().theme_use("alt")
    except:
        pass
    app = SLViewGUI(root)
    app.closeFile() # Make initial GUI state consistent with having no file loaded
    root.protocol("WM_DELETE_WINDOW", app.exit)
    root.mainloop()

if __name__ == "__main__":
    SLView()
