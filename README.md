# rp2040Term
PR2040 VT100 Terminal with Parallel TFT LCD.

# Warning
This design has not been compliance tested and will not be. It may cause personal damage to you or others. I take no responsibility for this. I accept no responsibility for any damage to any equipment that results from the use of this design and its components. IT IS ENTIRELY AT YOUR OWN RISK!

# Overview
The main purpose of this design was to lern about the RP2040 scanvideo library components and utilise them with a Parallel TFT LCD. Parallel meaning the interfaces uses HYSNC, VSYNC, DEN (currently not used), PCLK and RGB[5] signals.

I am currently using this with a Sharp LQ042 LCD with a resolution of 480x272. This works perfectly with a 8x16 Font to give a total of 60 Columns by 17 Rows.
