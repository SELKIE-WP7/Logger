#!/usr/bin/env python3

import os

from threading import Thread

import tkinter as tk
from tkinter import ttk
from tkinter import filedialog as tkfd
from tkinter import messagebox as tkmb
from tkinter.scrolledtext import ScrolledText

from SELKIELogger.scripts import log
from SELKIELogger.scripts.GUI import Button, TextHandler

from SELKIELogger import SLFiles as SLF
from SELKIELogger.SLMessages import IDs
from SELKIELogger.Processes import convertDataFile


class SLConvertOpts:
    def __init__(self):
        self.dataFile = None
        self.varFile = None
        self.checkVarFile = False
        self.sourceMap = None
        self.resample = "none"
        self.timesource = IDs.SLSOURCE_TIMER
        self.dropNA = True
        self.outFile = None
        self.outFileAuto = True
        self.outFormat = "csv.gz"


allSticky = (tk.N, tk.W, tk.E, tk.S)
resampleIntervals = [
    "None",
    "10min",
    "2min",
    "1min",
    "10s",
    "1s",
    "500ms",
    "200ms",
    "100ms",
]
supportedFormatsLabels = [
    "CSV (compressed)",
    "CSV (uncompressed)",
    "MATLAB data file",
    "Parquet file",
    "Excel spreadsheet",
]
supportedFormats = ["csv.gz", "csv", "mat", "parquet", "xlsx"]


class SLConvertGUI:
    def __init__(self, tkroot):
        self.options = SLConvertOpts()
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
                "label": ttk.Label(controlArea, text="Select data file: "),
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
                "label": ttk.Label(
                    controlArea, text="Select variable / channel mapping file: "
                ),
                "button": Button(
                    controlArea, config={"text": "Select"}, command=self.selectVarFile
                ),
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
                "label": ttk.Label(controlArea, text="Load channel information: "),
                "button": Button(
                    controlArea,
                    config={"text": "Load", "state": tk.DISABLED},
                    command=self.loadChannels,
                ),
            },
            4: {
                "status": ttk.Label(
                    controlArea,
                    width=7,
                    anchor=tk.CENTER,
                    text="X",
                    foreground="white",
                    background="red",
                ),
                "label": ttk.Label(controlArea, text="Select clock source: "),
                "button": ttk.Combobox(
                    controlArea, values=["<Unavailable>"], state=tk.DISABLED
                ),
            },
            5: {
                "status": ttk.Label(
                    controlArea,
                    width=7,
                    anchor=tk.CENTER,
                    text="X",
                    foreground="white",
                    background="red",
                ),
                "label": ttk.Label(controlArea, text="Set resample interval: "),
                "button": ttk.Combobox(
                    controlArea, values=resampleIntervals, text="None"
                ),
            },
            6: {
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
            7: {
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
            8: {
                "status": ttk.Label(
                    controlArea, width=7, anchor=tk.CENTER, text=""
                ),  # No indicator required
                "label": ttk.Label(controlArea, text="Drop missing values?"),
                "button": ttk.Checkbutton(controlArea, command=self.update),
            },
            9: {
                "status": ttk.Label(
                    controlArea, width=7, anchor=tk.CENTER, text=""
                ),  # No indicator required
                "label": ttk.Label(controlArea, text="Verbose output?"),
                "button": ttk.Checkbutton(controlArea, command=self.setLogLevel),
            },
        }

        self.steps[4]["button"].bind("<<ComboboxSelected>>", self.setTimesource)
        self.steps[4]["button"].bind("<Return>", self.setTimesource)
        self.steps[4]["button"].bind("<Tab>", self.setTimesource)
        self.steps[5]["button"].bind("<<ComboboxSelected>>", self.setResample)
        self.steps[5]["button"].bind("<Return>", self.setResample)
        self.steps[5]["button"].bind("<Tab>", self.setResample)
        self.steps[6]["button"].bind("<<ComboboxSelected>>", self.setFormat)
        self.steps[6]["button"].bind("<Return>", self.setFormat)
        self.steps[6]["button"].bind("<Tab>", self.setFormat)
        self.steps[6]["button"].set(supportedFormatsLabels[0])

        self.steps[8]["button"].state(["!alternate", "selected"])
        self.steps[9]["button"].state(["!alternate", "!selected"])

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
        self.runButton.grid(row=10, columnspan=3)

        self.setResample()
        self.setFormat()

    def setStatus(self, msg):
        self.statusText.set(msg)

    def setLogLevel(self):
        if "selected" in self.steps[9]["button"].state():
            log.setLevel(log.DEBUG)
            log.debug("Log level updated to DEBUG")
        else:
            log.debug("Log level updated to INFO")
            log.setLevel(log.INFO)

    def selectDataFile(self, event=None):
        oldname = self.options.dataFile
        name = tkfd.askopenfilename(
            parent=self.MFrame,
            title="Select data file:",
            filetypes=(
                ("Data files", "*.dat"),
                ("All files", "*.*"),
            ),
        )
        if name is None or len(name) == 0:
            return

        self.options.dataFile = name
        if oldname and name != oldname:
            self.options.checkVarFile = True
        self.autoSelectVarFile()
        self.update()

    def autoSelectVarFile(self):
        if not self.options.dataFile:
            self.options.varFile = None
            return

        oldname = self.options.varFile
        root, ext = os.path.splitext(self.options.dataFile)
        if (ext.lower() == ".dat") and os.path.exists(root + ".var"):
            self.options.varFile = root + ".var"
            self.options.checkVarFile = False

        if self.options.varFile != oldname:
            self.options.sourceMap = None

    def selectVarFile(self, event=None):
        oldname = self.options.varFile
        name = tkfd.askopenfilename(
            parent=self.MFrame,
            title="Select variable / channel mapping file:",
            filetypes=(
                ("Variable files", "*.var"),
                ("Data files", "*.dat"),
                ("All supported files", ("*.dat", "*.var")),
                ("All files", "*.*"),
            ),
        )
        if name is None or len(name) == 0:
            self.update()
            return

        self.options.varFile = name
        self.options.checkVarFile = False
        if oldname != name:
            self.options.sourceMap = None
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

    def loadChannels(self, event=None):
        vf = SLF.VarFile(self.options.varFile)
        self.options.sourceMap = vf.getSourceMap()
        self.update()

    def setTimesource(self, event=None):
        try:
            self.options.timesource = int(
                self.steps[4]["button"].get().split(":")[0], base=0
            )
        except:
            self.options.timesource = None
        self.update()

    def setResample(self, event=None):
        rs = self.steps[5]["button"].get().strip()
        if rs == "" or rs.lower() == "none":
            self.options.resample = -1
        else:
            self.options.resample = rs
        self.update()

    def setFormat(self, event=None):
        ix = self.steps[6]["button"].current()
        if ix < 0:
            try:
                ix2 = supportedFormats.index(
                    self.steps[6]["button"].get().strip().lower()
                )
                self.options.outFormat = supportedFormats[ix2]
                self.steps[6]["button"].set(supportedFormatsLabels[ix2])
            except ValueError:
                self.options.outFormat = None
        else:
            self.options.outFormat = supportedFormats[ix]

        self.update()

    def startConversion(self):
        if self.bgThread and self.bgThread.is_alive():
            return

        if self.options.resample == -1:
            self.options.resample = None

        self.bgThread = Thread(
            target=convertDataFile,
            args=[
                self.options.varFile,
                self.options.dataFile,
                self.options.outFile,
                self.options.outFormat,
                self.options.timesource,
                self.options.resample,
                self.options.dropNA,
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
            if self.options.dataFile:
                return os.path.exists(self.options.dataFile)
            return False

        def step2OK():
            if self.options.varFile:
                return os.path.exists(self.options.varFile)
            return False

        def step3OK():
            return self.options.sourceMap is not None

        def step4OK():
            if not step3OK():
                return False

            if (
                not self.options.timesource
                and IDs.SLSOURCE_TIMER in self.options.sourceMap
            ):
                self.options.timesource = IDs.SLSOURCE_TIMER
                return True

            return (
                self.options.timesource
                and self.options.timesource in self.options.sourceMap
            )

        def step5OK():
            # Better validation required!
            return self.options.resample is not None

        def step6OK():
            return self.options.outFormat in supportedFormats

        def step7OK():
            return self.options.outFileAuto or self.options.outFile

        # Steps 1 and 2 are always available

        def step3Available():
            return step1OK() and step2OK()

        def step4Available():
            return step3Available() and step3OK()

        # Step 5 and 6 (Resample interval, Format) always available.

        def step7Available():
            # To auto set an output name, need input, resample and format
            return step1OK() and step5OK() and step6OK()

        if step1OK():
            self.root.title(f"SLConvert: {os.path.basename(self.options.dataFile)}")
            self.steps[1]["status"].configure(
                text="OK", foreground="white", background="green"
            )
            self.steps[1]["label"].configure(
                text=f"{os.path.basename(self.options.dataFile)} selected"
            )
        else:
            self.root.title("SLConvert: (No file selected)")
            self.steps[1]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.steps[1]["label"].configure(text=f"Select a data file")

        if step2OK():
            if self.options.checkVarFile:
                self.steps[2]["status"].configure(
                    text="Check", foreground="black", background="yellow"
                )
            else:
                self.steps[2]["status"].configure(
                    text="OK", foreground="white", background="green"
                )
            self.steps[2]["label"].configure(
                text=f"{os.path.basename(self.options.varFile)} selected"
            )
        else:
            self.steps[2]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.steps[2]["label"].configure(
                text=f"Select a variable / channel mapping file"
            )

        if step3Available():
            self.steps[3]["button"].configure(state=tk.NORMAL)
        else:
            self.options.sourceMap = None
            self.steps[3]["button"].configure(state=tk.DISABLED)

        if step3OK():
            self.steps[3]["status"].configure(
                text="OK", foreground="white", background="green"
            )
        else:
            self.steps[3]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step4Available():
            self.steps[4]["button"].configure(
                values=[
                    f"0x{x:02x}: {self.options.sourceMap[x]}"
                    for x in self.options.sourceMap
                ],
                state=tk.NORMAL,
            )
        else:
            self.steps[4]["button"].configure(values="<Unavailable>", state=tk.DISABLED)
            self.steps[4]["button"].set("<Unavailable>")
            self.steps[4]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step4OK():
            self.steps[4]["status"].configure(
                text="OK", foreground="white", background="green"
            )
            self.steps[4]["button"].set(
                f"0x{self.options.timesource:02x}: {self.options.sourceMap[self.options.timesource]}"
            )
        else:
            self.steps[4]["button"].set("Select clock")
            self.steps[4]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.options.timesource = None

        if step5OK():
            self.steps[5]["status"].configure(
                text="OK", foreground="white", background="green"
            )
            if self.steps[5]["button"].get().strip() == "":
                self.steps[5]["button"].set("None")
                self.setResample()
        else:
            self.steps[5]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step6OK():
            self.steps[6]["status"].configure(
                text="OK", foreground="white", background="green"
            )
        else:
            self.steps[6]["status"].configure(
                text="X", foreground="white", background="red"
            )

        if step7Available():
            self.steps[7]["button"].configure(state=tk.NORMAL)
        else:
            self.options.sourceMap = None
            self.steps[7]["button"].configure(state=tk.DISABLED)

        if step7OK():
            if self.options.outFileAuto:
                self.steps[7]["status"].configure(
                    text="Auto", foreground="white", background="green"
                )
                self.steps[7]["label"].configure(
                    text="Output file name will be set automatically"
                )
            else:
                self.steps[7]["status"].configure(
                    text="OK", foreground="white", background="green"
                )
                self.steps[7]["label"].configure(
                    text=f"Output file name: {os.path.basename(self.options.outFile)}"
                )
        else:
            self.steps[7]["status"].configure(
                text="X", foreground="white", background="red"
            )
            self.steps[7]["label"].configure(text=f"Set output file name")

        if "selected" in self.steps[8]["button"].state():
            self.options.dropNA = True
        else:
            self.options.dropNA = False

        if (
            step1OK()
            and step2OK()
            and step3OK()
            and step4OK()
            and step5OK()
            and step6OK()
            and step7OK()
            and not self.bgThread
        ):
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

    main(SLConvertGUI)
