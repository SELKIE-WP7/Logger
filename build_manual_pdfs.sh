#!/bin/bash

mkdir "pdfs/"
for i in docs/manual/*.md; do
	o="pdfs/$(basename ${i}|sed 's/.md/.pdf/')"
	pandoc -s --resource-path docs/images/ -V papersize=a4 -V institute="Swansea University" -V fontfamily=mathptmx -V geometry="left=3cm, right=2cm, top=2.5cm, bottom=3cm" -t latex -o "${o}" "${i}"
 done
