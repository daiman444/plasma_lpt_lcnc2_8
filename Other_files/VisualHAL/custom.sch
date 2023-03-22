EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:HAL
LIBS:custom-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "noname.sch"
Date "22 sep 2014"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L FLIPFLOP flipflop.0
U 1 1 54202679
P 5100 1500
F 0 "flipflop.0" H 5100 1850 60  0000 C CNN
F 1 "FLIPFLOP" H 5100 1405 60  0000 C CNN
F 2 "~" H 5100 1500 60  0000 C CNN
F 3 "~" H 5100 1500 60  0000 C CNN
	1    5100 1500
	1    0    0    -1  
$EndComp
$Comp
L FLIPFLOP flipflop.1
U 1 1 54202688
P 5100 2600
F 0 "flipflop.1" H 5100 2950 60  0000 C CNN
F 1 "FLIPFLOP" H 5100 2505 60  0000 C CNN
F 2 "~" H 5100 2600 60  0000 C CNN
F 3 "~" H 5100 2600 60  0000 C CNN
	1    5100 2600
	1    0    0    -1  
$EndComp
$Comp
L FLIPFLOP flipflop.2
U 1 1 54202697
P 5100 3700
F 0 "flipflop.2" H 5100 4050 60  0000 C CNN
F 1 "FLIPFLOP" H 5100 3605 60  0000 C CNN
F 2 "~" H 5100 3700 60  0000 C CNN
F 3 "~" H 5100 3700 60  0000 C CNN
	1    5100 3700
	1    0    0    -1  
$EndComp
$Comp
L FLIPFLOP flipflop.3
U 1 1 542026DC
P 5100 4800
F 0 "flipflop.3" H 5100 5150 60  0000 C CNN
F 1 "FLIPFLOP" H 5100 4705 60  0000 C CNN
F 2 "~" H 5100 4800 60  0000 C CNN
F 3 "~" H 5100 4800 60  0000 C CNN
	1    5100 4800
	1    0    0    -1  
$EndComp
$Comp
L NOT not.0
U 1 1 5420272A
P 6250 1400
F 0 "not.0" H 6300 1550 59  0000 C CNN
F 1 "NOT" H 6300 1250 59  0000 C CNN
F 2 "~" H 6250 1400 60  0000 C CNN
F 3 "~" H 6250 1400 60  0000 C CNN
	1    6250 1400
	1    0    0    -1  
$EndComp
$Comp
L NOT not.1
U 1 1 54202739
P 6250 2500
F 0 "not.1" H 6300 2650 59  0000 C CNN
F 1 "NOT" H 6300 2350 59  0000 C CNN
F 2 "~" H 6250 2500 60  0000 C CNN
F 3 "~" H 6250 2500 60  0000 C CNN
	1    6250 2500
	1    0    0    -1  
$EndComp
$Comp
L NOT not.2
U 1 1 54202748
P 6250 3600
F 0 "not.2" H 6300 3750 59  0000 C CNN
F 1 "NOT" H 6300 3450 59  0000 C CNN
F 2 "~" H 6250 3600 60  0000 C CNN
F 3 "~" H 6250 3600 60  0000 C CNN
	1    6250 3600
	1    0    0    -1  
$EndComp
$Comp
L NOT not.3
U 1 1 54202757
P 6250 4700
F 0 "not.3" H 6300 4850 59  0000 C CNN
F 1 "NOT" H 6300 4550 59  0000 C CNN
F 2 "~" H 6250 4700 60  0000 C CNN
F 3 "~" H 6250 4700 60  0000 C CNN
	1    6250 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5600 1400 5800 1400
Wire Wire Line
	5600 2500 5800 2500
Wire Wire Line
	5600 4700 5800 4700
Wire Wire Line
	6800 2100 4500 2100
Wire Wire Line
	4500 2100 4500 2450
Wire Wire Line
	4500 2450 4600 2450
Wire Wire Line
	6800 1000 4500 1000
Wire Wire Line
	4500 1000 4500 1350
Wire Wire Line
	4500 1350 4600 1350
Wire Wire Line
	5600 3600 5800 3600
Wire Wire Line
	6800 3200 4500 3200
Wire Wire Line
	4500 3200 4500 3550
Wire Wire Line
	4500 3550 4600 3550
Wire Wire Line
	6800 4300 4500 4300
Wire Wire Line
	4500 4300 4500 4650
Wire Wire Line
	4500 4650 4600 4650
Wire Wire Line
	4600 2550 4400 2550
Wire Wire Line
	4400 2550 4400 1900
Wire Wire Line
	4600 3650 4400 3650
Wire Wire Line
	4400 3650 4400 3000
Wire Wire Line
	4600 4750 4400 4750
Wire Wire Line
	4400 4750 4400 4100
Wire Wire Line
	4600 1550 4300 1550
Wire Wire Line
	4300 1550 4300 4850
Wire Wire Line
	4200 4850 4600 4850
Wire Wire Line
	4600 3750 4300 3750
Connection ~ 4300 3750
Wire Wire Line
	4600 2650 4300 2650
Connection ~ 4300 2650
$Comp
L OR2 or2.0
U 1 1 542028FC
P 3750 4850
F 0 "or2.0" H 3750 5000 60  0000 C CNN
F 1 "OR2" H 3750 4800 39  0000 C CNN
F 2 "~" H 3650 5050 60  0000 C CNN
F 3 "~" H 3650 5050 60  0000 C CNN
	1    3750 4850
	1    0    0    -1  
$EndComp
$Comp
L AND2 and2.0
U 1 1 5420290B
P 7450 5600
F 0 "and2.0" H 7450 5750 60  0000 C CNN
F 1 "AND2" H 7450 5550 39  0000 C CNN
F 2 "~" H 7350 5800 60  0000 C CNN
F 3 "~" H 7350 5800 60  0000 C CNN
	1    7450 5600
	1    0    0    -1  
$EndComp
Wire Wire Line
	6800 5650 7000 5650
Wire Wire Line
	6900 5550 7000 5550
Wire Wire Line
	7900 5600 8000 5600
Wire Wire Line
	8000 5600 8000 5850
Wire Wire Line
	3200 4900 3200 5850
Wire Wire Line
	3200 4900 3300 4900
Connection ~ 4300 4850
Wire Wire Line
	3300 4800 2700 4800
Wire Wire Line
	4600 1450 2700 1450
Text GLabel 2700 1450 0    60   Input ~ 0
counter.clk
Text GLabel 2700 4800 0    60   Input ~ 0
counter.reset
Text GLabel 8500 1900 2    60   Input ~ 0
counter.out1
Text GLabel 8500 3000 2    60   Input ~ 0
counter.out2
Text GLabel 8500 4100 2    60   Input ~ 0
counter.out4
Text GLabel 8500 5200 2    60   Input ~ 0
counter.out8
$Comp
L EDGE edge.0
U 1 1 54205D77
P 4000 5350
F 0 "edge.0" H 4000 5600 60  0000 C CNN
F 1 "EDGE" H 4000 5200 39  0000 C CNN
F 2 "~" H 4000 5600 60  0000 C CNN
F 3 "~" H 4000 5600 60  0000 C CNN
F 4 "1" H 4275 5475 39  0000 C CNN "both"
F 5 "1" H 4275 5375 39  0000 C CNN "in-edge"
F 6 "0" H 4275 5275 39  0000 C CNN "out-width-ns"
	1    4000 5350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 5350 3400 5350
Wire Wire Line
	6800 4300 6800 4700
Wire Wire Line
	6800 4700 6700 4700
Wire Wire Line
	6800 3200 6800 3600
Wire Wire Line
	6800 3600 6700 3600
Wire Wire Line
	6800 2100 6800 2500
Wire Wire Line
	6800 2500 6700 2500
Wire Wire Line
	6800 1000 6800 1400
Wire Wire Line
	6800 1400 6700 1400
Wire Wire Line
	4400 1900 8500 1900
Wire Wire Line
	4400 3000 8500 3000
Wire Wire Line
	4400 4100 8500 4100
Wire Wire Line
	5700 3600 5700 4100
Connection ~ 5700 4100
Connection ~ 5700 3600
Wire Wire Line
	5700 4700 5700 5200
Wire Wire Line
	5700 5200 8500 5200
Connection ~ 5700 4700
Wire Wire Line
	5700 2500 5700 3000
Connection ~ 5700 3000
Connection ~ 5700 2500
Wire Wire Line
	5700 1400 5700 1900
Connection ~ 5700 1900
Connection ~ 5700 1400
Wire Wire Line
	6900 5550 6900 3000
Connection ~ 6900 3000
Wire Wire Line
	6800 5650 6800 5200
Connection ~ 6800 5200
Wire Wire Line
	3200 5850 8000 5850
Connection ~ 3200 5350
$EndSCHEMATC
