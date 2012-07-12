/* Arduino WaveHC Library
 * Copyright (C) 2009 by William Greiman
 *  
 * This file is part of the Arduino WaveHC Library
 *  
 * This Library is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino WaveHC Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
\mainpage Arduino WaveHC Library
<CENTER>Copyright &copy; 2009 by William Greiman
</CENTER>

\section Intro Introduction

WaveHC is an Arduino library for the Adafruit Wave Shield.  It can play
uncompressed mono Wave(.WAV) files at sample rate up to 44.1 K samples per
second. Only the high 12 bits of 16-bit files are used.  Audio files are read
from an SD flash memory card.

Standard SD and high capacity SDHC flash memory cards are supported with
FAT16 or FAT32 file systems.  The WaveHC only supports short FAT 8.3 names.

WaveHC does not support MMC flash cards.

\section comment Bugs and Comments

If you wish to report bugs or have comments, send email to
fat16lib@sbcglobal.net.

\section SDcard SD/SDHC Cards

Arduinos access SD cards using the cards SPI protocol.  PCs, Macs, and
most consumer devices use the 4-bit parallel SD protocol.  A card that
functions well on A PC or Mac may not work well on the Arduino.

Most cards have good SPI read performance but cards vary widely in
how there SPI hardware interface is implemented.  Newer card require
very fast rise times for SPI signals.  Version 1.0 of the Wave Shield
may not work well with these cards.  Ladyada's improved Version 1.1
works with almost all SD/SDHC cards.

The default SPI clock rate is 8 Mhz.  It may be helpful on Version 1.0
wave shields to reduce this to 4 Mhz. See SdReader::init() for details.

SanDisk cards generally have good performance in the Version 1.0 Wave Shield.


\section WaveHCClass WaveHC Usage

See Ladyada's excellent tutorial on using WaveHC:

http://www.ladyada.net/make/waveshield/libraryhc.html

Also see the readme.txt file for instructions on installing WaveHC.

Advanced users may need to edit the WavePinDefs.h file.

WaveHC uses a slightly restricted form of short file names.
Only printable ASCII characters are supported. No characters with code point
values greater than 127 are allowed.  Space is not allowed even though space
was allowed in the API of early versions of DOS.

Short names are limited to 8 characters followed by an optional period (.)
and extension of up to 3 characters.  The characters may be any combination
of letters and digits.  The following special characters are also allowed:

$ % ' - _ @ ~ ` ! ( ) { } ^ # &

Short names are always converted to upper case and their original case
value is lost.

\section HowTo How to Format and Prepare SD Cards for WaveHC

WaveHC is optimized for contiguous files.  It will only play 16-bit
44.1 K files if they are contiguous.  All files copied to a newly
formatted card will be contiguous.  It is only possible to create
a fragmented file if you delete a file from an SD and copy a larger
file to the SD.

You should use a freshly formatted SD card for best performance.  FAT
file systems become slower if many files have been created and deleted.
This is because the directory entry for a deleted file is marked as deleted,
but is not deleted.  When a file is opened, these entries must be scanned
to find the file to be opened, a flaw in the FAT design.  Also files can
become fragmented which causes reads to be slower.

Microsoft operating systems support removable media formatted with a
Master Boot Record, MBR, or formatted as a super floppy with a FAT Boot Sector
in block zero.

Microsoft operating systems expect MBR formatted removable media
to have only one partition. The first partition should be used.

Microsoft operating systems do not support partitioning SD flash cards.
If you erase an SD card with a program like KillDisk, Most versions of
Windows will format the card as a super floppy.

The best way to restore an SD card's format is to use SDFormatter
which can be downloaded from:

http://www.sdcard.org/consumers/formatter/

SDFormatter aligns flash erase boundaries with file
system structures which reduces write latency and file system overhead.

SDFormatter does not have an option for FAT type so it may format
small cards as FAT12.

After the MBR is restored by SDFormatter you may need to reformat small
cards that have been formatted FAT12 to force the volume type to be FAT16.

If you reformat the SD card with an OS utility, choose a cluster size that
will result in:

4084 < CountOfClusters && CountOfClusters < 65525

The volume will then be FAT16.

If you are formatting an SD card on OS X or Linux, be sure to use the first
partition. Format this partition with a cluster count in above range.

\section  References References

Adafruit Industries:

http://www.adafruit.com/

http://www.ladyada.net/make/waveshield/

The Arduino site:

http://www.arduino.cc/

For more information about FAT file systems see:

http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx

For information about using SD cards as SPI devices see:

http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf

The ATmega328 datasheet:

http://www.atmel.com/dyn/resources/prod_documents/doc8161.pdf
 

 */  