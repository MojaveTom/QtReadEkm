# QtReadEkm
This is a Qt Project on Mac OS X Yosemite to read an EKM v.4 meter connected via EKM's "EKM Blink - RS-485 to USB Converter".
The EKM Omnimeter Pulse UL v.4 - Pulse has two Basic kWh Meter 14mm 120 volt meters connected to pulse 1 and pulse 2 inputs and
a water meter connected to pulse 3 input.  One output is used to power a EKM 485Bee and the other controls a relay that 
switches the rain sensor on a sprinkler timer.

The program reads the v.4 meter "A" and "B" data periodically and stores the responses basically un-intrepeted into a MySql 
database.
The Notes.txt file has example SQL for pulling interesting (to me) information out of the database tables.
