#!/bin/bash

# Convertir los archivos .webp a .png
for i in *.webp; do
    convert "$i" "${i%.*}.png"
done