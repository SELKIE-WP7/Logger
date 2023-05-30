#!/usr/bin/env python3

# Copyright (C) 2023 Swansea University
#
# This file is part of the SELKIELogger suite of tools.
#
# SELKIELogger is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this SELKIELogger product.
# If not, see <http://www.gnu.org/licenses/>.

import os
from pandas import to_timedelta

from threading import Thread

import tkinter as tk
from tkinter import ttk
from tkinter import filedialog as tkfd
from tkinter import messagebox as tkmb
from tkinter.scrolledtext import ScrolledText

from SELKIELogger.scripts import log
from SELKIELogger.scripts.GUI import Button, TextHandler, logFuncExceptions

from SELKIELogger.Processes import N2KToTimeseries

# def N2KToTimeseries(n2kfile, output, fileFormat, dropna=False, pgns=None, epoch=False)


class N2KConvertOpts:
    def __init__(self):
        self.n2kFile = None
        self.dropNA = True
        self.outFile = None
        self.outFileAuto = True
        self.outFormat = "csv.gz"
        self.epoch = False
        self.pgns = None


allSticky = (tk.N, tk.W, tk.E, tk.S)
supportedFormatsLabels = [
    "CSV (compressed)",
    "CSV (uncompressed)",
    "MATLAB data file",
    "Parquet file",
    "Excel spreadsheet",
]
supportedFormats = ["csv.gz", "csv", "mat", "parquet", "xlsx"]


class N2KConvertGUI:
    def __init__(self, tkroot):
        self.options = N2KConvertOpts()
        self.bgThread = None

        self.statusText = tk.StringVar()
        self.setStatus("Starting...")

        self.root = tkroot
        self.root.option_add("*tearOff", tk.FALSE)
        self.root.minsize(600, 600)
        # Single frame as child item, to be resized to fit
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        self.MFrame = ttk.Frame(self.root, width=600, height=600)
        self.MFrame.grid(column=0, row=0, sticky=allSticky)

        self.MFrame.columnconfigure(0, weight=1)
        self.MFrame.rowconfigure(0, weight=1)
        self.MFrame.rowconfigure(1, weight=2)
        self.MFrame.rowconfigure(2, weight=0)

        statusArea = ttk.Frame(self.MFrame)
        statusArea.grid(column=0, row=2, sticky=allSticky)
        statusArea.columnconfigure(0, weight=4)
        statusArea.columnconfigure(1, weight=0)

        self.status = ttk.Label(statusArea, textvariable=self.statusText)
        self.status.grid(column=0, row=0, sticky=allSticky)

        #        self.actionbutton = Button(
        #            statusArea,
        #            config={"text": "Cancel", "state": tk.DISABLED},
        #            # command=lambda e=None: self.processCancel(e),
        #            command=lambda e: print(e)
        #        )
        #        self.actionbutton.grid(column=1, row=0, sticky=allSticky)

        self.buildControls()

        self.logPane = ScrolledText(self.MFrame, height=8)
        self.logPane.grid(column=0, row=1, sticky=allSticky)
        log.addHandler(TextHandler(self.logPane))
        log.setLevel(log.INFO)

        # self.root.bind("<<ProcessingComplete>>", self.processComplete)
        # self.root.bind("<<ProcessingProgress>>", self.processProgress)

        self.setStatus("Ready")
        self.update()

    def buildControls(self):
        controlArea = ttk.Frame(self.MFrame, relief=tk.SUNKEN)
        controlArea.grid(column=0, row=0, sticky=allSticky, padx=3, pady=3)
        controlArea.columnconfigure(0, weight=0)
        controlArea.columnconfigure(1, weight=4)
        controlArea.columnconfigure(2, weight=0)

        head = ttk.Label(
            controlArea, text="Select data files and conversion options below"
        )
        head.grid(row=0, column=0, columnspan=3, padx=3, pady=6)

        self.steps = {
            1: {
                "status": ttk.Label(
                    controlArea,
                    width=7,
                    anchor=tk.CENTER,
                    text="X",
                    foreground="white",
                    background="red",
                ),
                "label": ttk.Label(controlArea, text="Select N2K file: "),
                "button": Button(
                    controlArea, config={"text": "Select"}, command=self.selectDataFile
                ),
            },
            2: {
                "status": ttk.Label(
                    controlArea,
                    width=7,
                    anchor=tk.CENTER,
                    text="X",
                    foreground="white",
                    background="red",
                ),
                "label": ttk.Label(controlArea, text="Set output format: "),
                "button": ttk.Combobox(controlArea, values=supportedFormatsLabels),
            },
            3: {
                "status": ttk.Label(
                    controlArea,
                    width=7,
                    anchor=tk.CENTER,
                    text="X",
                    foreground="white",
                    background="red",
                ),
                "label": ttk.Label(controlArea, text="Select output file: "),
                "button": Button(
                    controlArea,
                    config={"text": "Select"},
                    command=self.selectOutputFile,
                ),
            },
            4: {
                "status": ttk.Label(
                    controlArea, width=7, anchor=tk.CENTER, text=""
                ),  # No indicator required
                "label": ttk.Label(controlArea, text="Drop missing values?"),
                "button": ttk.Checkbutton(controlArea, command=self.update),
            },
            5: {
                "status": ttk.Label(
                    controlArea, width=7, anchor=tk.CENTER, text=""
                ),  # No indicator required
                "label": ttk.Label(
                    controlArea,
                    text="Convert timestamps to real-time values? [Very slow]",
                ),
                "button": ttk.Checkbutton(controlArea, command=self.update),
            },
            6: {
                "status": ttk.Label(
                    controlArea, width=7, anchor=tk.CENTER, text=""
                ),  # No indicator required
                "label": ttk.Label(controlArea, text="Verbose output?"),
                "button": ttk.Checkbutton(controlArea, command=self.setLogLevel),
            },
        }

        self.steps[2]["button"].bind("<<ComboboxSelected>>", self.setFormat)
        self.steps[2]["button"].bind("<Return>", self.setFormat)
        self.steps[2]["button"].bind("<Tab>", self.setFormat)
        self.steps[2]["button"].set(supportedFormatsLabels[0])

        self.steps[4]["button"].state(["!alternate", "selected"])
        self.steps[5]["button"].state(["!alternate", "!selected"])
        self.steps[6]["button"].state(["!alternate", "!selected"])

        for s in self.steps.keys():
            self.steps[s]["status"].grid(
                row=s, column=0, sticky=(tk.N + tk.S), padx=3, pady=5
            )
            self.steps[s]["label"].grid(
                row=s, column=1, sticky=allSticky, padx=3, pady=5
            )
            self.steps[s]["button"].grid(
                row=s, column=2, sticky=allSticky, padx=3, pady=5
            )

        self.runButton = Button(
            controlArea,
            config={"text": "Start Conversion", "width": 18, "state": tk.DISABLED},
            command=self.startConversion,
        )
        self.runButton.grid(row=7, columnspan=3)

        self.setFormat()

    def setStatus(self, msg):
        self.statusText.set(msg)

    def setLogLevel(self):
        if "selected" in self.steps[6]["button"].state():
            log.setLevel(log.DEBUG)
            log.debug("Log level updated to DEBUG")
        else:
            log.debug("Log level updated to INFO")
            log.setLevel(log.INFO)

    def selectDataFile(self, event=None):
        oldname = self.options.n2kFile
        name = tkfd.askopenfilename(
            parent=self.MFrame,
            title="Select data file:",
            filetypes=(
                ("Extracted files", ("*.eml", "*.raw.dat")),
                ("Data files", "*.dat"),
                ("All files", "*.*"),
            ),
        )
        if name is None or len(name) == 0:
            return

        self.options.n2kFile = name
        if oldname and name != oldname:
            self.options.checkVarFile = True
        self.update()

    def selectOutputFile(self, event=None):
        if not self.options.outFileAuto and tkmb.askyesno(
            message="Use automatic file naming?"
        ):
            self.options.outFileAuto = True
            self.options.outFile = None
            self.update()
            return

        de = self.options.outFormat
        if de is None:
            de = "csv.gz"
        ix = supportedFormats.index(de)

        name = tkfd.asksaveasfilename(
            parent=self.MFrame,
            title="Select output file",
            filetypes=[
                (supportedFormatsLabels[ix], f"*.{supportedFormats[ix]}"),
                ("All supported formats", [f"*.{x}" for x in supportedFormats]),
                ("All files", "*.*"),
            ],
            defaultextension=f".{de}",
        )
        if name is None or len(name) == 0:
            self.update()
            return

        self.options.outFileAuto = False
        self.options.outFile = name

        self.update()

    def setFormat(self, event=None):
        ix = self.steps[2]["button"].current()
        if ix < 0:
            try:
                ix2 = supportedFormats.index(
                    self.steps[2]["button"].get().strip().lower()
                )
                self.options.outFormat = supportedFormats[ix2]
                self.steps[2]["button"].set(supportedFormatsLabels[ix2])
            except ValueError:
                self.options.outFormat = None
        else:
            self.options.outFormat = supportedFormats[ix]

        self.update()

    def startConversion(self):
        if self.bgThread and self.bgThread.is_alive():
            return

        self.bgThread = Thread(
            target=logFuncExceptions,
            args=[
                N2KToTimeseries,
                self.options.n2kFile,
                self.options.outFile,
                self.options.outFormat,
                self.options.dropNA,
                self.options.pgns,
                self.options.epoch,
            ],
            name="DataConversion",
        )
        self.runButton.configure(state=tk.DISABLED)
        self.setStatus("Running...")
        self.bgThread.start()
        while self.bgThread.is_alive():
            self.root.update()
        self.bgThread.join()
        self.bgThread = None
        self.setStatus("Conversion complete")
        self.runButton.configure(state=tk.NORMAL)

    def update(self):
        def step1OK():
            if self.options.n2kFile:
                return os.path.exists(self.options.n2kFile)
            return False

        def step2OK():
            return self.options.outFormat in supportedFormats

        def step3OK():
            return self.options.outFileAuto or self.options.outFile

        # Steps 1 and 2 are always available

        def step3Available():
            # To auto set an output name, need input, resample and format
            return step1OK() and step2OK() and step3OK()

        if step1OK():
            self.root.title(f"N2KConvert: {os.path.basename(self.options.n2kFile)}")
            self.steps[1]["status"].configure(
                text="OK", foreground="white", background="green"
            )
            self.steps[1]["label"].configure(
                text=f"{os.path.basename(self.options.n2kFile)} selected"
            )
        else:
            self.root.title("N2KConvert: (No file selected)")
            self.steps[1]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.steps[1]["label"].configure(text=f"Select a data file")

        if step2OK():
            self.steps[2]["status"].configure(
                text="OK", foreground="white", background="green"
            )
        else:
            self.steps[2]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step3OK():
            self.steps[3]["status"].configure(
                text="OK", foreground="white", background="green"
            )
        else:
            self.steps[3]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step3OK():
            if self.options.outFileAuto:
                self.steps[3]["status"].configure(
                    text="Auto", foreground="white", background="green"
                )
                self.steps[3]["label"].configure(
                    text="Output file name will be set automatically"
                )
            else:
                self.steps[3]["status"].configure(
                    text="OK", foreground="white", background="green"
                )
                self.steps[3]["label"].configure(
                    text=f"Output file name: {os.path.basename(self.options.outFile)}"
                )
        else:
            self.steps[3]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.steps[3]["label"].configure(text=f"Set output file name")

        if "selected" in self.steps[4]["button"].state():
            self.options.dropNA = True
        else:
            self.options.dropNA = False

        if "selected" in self.steps[5]["button"].state():
            self.options.epoch = True
        else:
            self.options.epoch = False

        if step1OK() and step2OK() and step3OK() and not self.bgThread:
            self.runButton.configure(state=tk.NORMAL)
        else:
            self.runButton.configure(state=tk.DISABLED)

        return

    def exit(self, event=None, **kwargs):
        self.root.destroy()
        if self.bgThread:
            self.bgThread.join()


if __name__ == "__main__":
    from SELKIELogger.scripts.GUI import main

    main(N2KConvertGUI)
