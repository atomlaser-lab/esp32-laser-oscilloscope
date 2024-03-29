EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector:Conn_Coaxial J?
U 1 1 63A3A077
P 1300 2200
F 0 "J?" H 1400 2175 50  0000 L CNN
F 1 "Conn_Coaxial" H 1400 2084 50  0000 L CNN
F 2 "" H 1300 2200 50  0001 C CNN
F 3 " ~" H 1300 2200 50  0001 C CNN
	1    1300 2200
	-1   0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 63A3E2DF
P 2050 2200
F 0 "R?" V 1843 2200 50  0000 C CNN
F 1 "10k" V 1934 2200 50  0000 C CNN
F 2 "" V 1980 2200 50  0001 C CNN
F 3 "~" H 2050 2200 50  0001 C CNN
	1    2050 2200
	0    1    1    0   
$EndComp
Wire Wire Line
	1500 2200 1900 2200
$Comp
L power:GND #PWR?
U 1 1 63A4A9AA
P 1300 2400
F 0 "#PWR?" H 1300 2150 50  0001 C CNN
F 1 "GND" H 1305 2227 50  0000 C CNN
F 2 "" H 1300 2400 50  0001 C CNN
F 3 "" H 1300 2400 50  0001 C CNN
	1    1300 2400
	1    0    0    -1  
$EndComp
$Comp
L Device:D D?
U 1 1 63A580DE
P 3850 2450
F 0 "D?" V 3804 2530 50  0000 L CNN
F 1 "D" V 3895 2530 50  0000 L CNN
F 2 "" H 3850 2450 50  0001 C CNN
F 3 "~" H 3850 2450 50  0001 C CNN
	1    3850 2450
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR?
U 1 1 63A58F0A
P 3850 2650
F 0 "#PWR?" H 3850 2400 50  0001 C CNN
F 1 "GND" H 3855 2477 50  0000 C CNN
F 2 "" H 3850 2650 50  0001 C CNN
F 3 "" H 3850 2650 50  0001 C CNN
	1    3850 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 2650 3850 2600
$Comp
L Device:D D?
U 1 1 63A59704
P 3850 2150
F 0 "D?" V 3804 2230 50  0000 L CNN
F 1 "D" V 3895 2230 50  0000 L CNN
F 2 "" H 3850 2150 50  0001 C CNN
F 3 "~" H 3850 2150 50  0001 C CNN
	1    3850 2150
	0    1    1    0   
$EndComp
Wire Wire Line
	3850 1950 3850 2000
Connection ~ 3850 2300
$Comp
L Connector:Conn_Coaxial J?
U 1 1 63A60B14
P 1300 3800
F 0 "J?" H 1400 3775 50  0000 L CNN
F 1 "Conn_Coaxial" H 1400 3684 50  0000 L CNN
F 2 "" H 1300 3800 50  0001 C CNN
F 3 " ~" H 1300 3800 50  0001 C CNN
	1    1300 3800
	-1   0    0    -1  
$EndComp
$Comp
L Device:D D?
U 1 1 63A6433B
P 1700 3950
F 0 "D?" V 1654 4030 50  0000 L CNN
F 1 "D" V 1745 4030 50  0000 L CNN
F 2 "" H 1700 3950 50  0001 C CNN
F 3 "~" H 1700 3950 50  0001 C CNN
	1    1700 3950
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR?
U 1 1 63A644B0
P 1700 4150
F 0 "#PWR?" H 1700 3900 50  0001 C CNN
F 1 "GND" H 1705 3977 50  0000 C CNN
F 2 "" H 1700 4150 50  0001 C CNN
F 3 "" H 1700 4150 50  0001 C CNN
	1    1700 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	1700 4150 1700 4100
$Comp
L Device:D D?
U 1 1 63A644BB
P 1700 3650
F 0 "D?" V 1654 3730 50  0000 L CNN
F 1 "D" V 1745 3730 50  0000 L CNN
F 2 "" H 1700 3650 50  0001 C CNN
F 3 "~" H 1700 3650 50  0001 C CNN
	1    1700 3650
	0    1    1    0   
$EndComp
$Comp
L power:VCC #PWR?
U 1 1 63A644C5
P 1700 3450
F 0 "#PWR?" H 1700 3300 50  0001 C CNN
F 1 "VCC" H 1715 3623 50  0000 C CNN
F 2 "" H 1700 3450 50  0001 C CNN
F 3 "" H 1700 3450 50  0001 C CNN
	1    1700 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1700 3450 1700 3500
Connection ~ 1700 3800
$Comp
L power:GND #PWR?
U 1 1 63A699F8
P 1300 4000
F 0 "#PWR?" H 1300 3750 50  0001 C CNN
F 1 "GND" H 1305 3827 50  0000 C CNN
F 2 "" H 1300 4000 50  0001 C CNN
F 3 "" H 1300 4000 50  0001 C CNN
	1    1300 4000
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x02_Male J?
U 1 1 63A53CD2
P 800 900
F 0 "J?" H 908 1081 50  0000 C CNN
F 1 "Conn_01x02_Male" H 908 990 50  0000 C CNN
F 2 "" H 800 900 50  0001 C CNN
F 3 "~" H 800 900 50  0001 C CNN
	1    800  900 
	1    0    0    -1  
$EndComp
$Comp
L Converter_DCDC:IH4824DH PS?
U 1 1 63A55EE5
P 2100 1000
F 0 "PS?" H 2100 1367 50  0000 C CNN
F 1 "IH4824DH" H 2100 1276 50  0000 C CNN
F 2 "Converter_DCDC:Converter_DCDC_XP_POWER-IHxxxxDH_THT" H 1050 750 50  0001 L CNN
F 3 "https://www.xppower.com/pdfs/SF_IH.pdf" H 3150 700 50  0001 L CNN
	1    2100 1000
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 63A57D18
P 1350 1200
F 0 "#PWR?" H 1350 950 50  0001 C CNN
F 1 "GND" H 1355 1027 50  0000 C CNN
F 2 "" H 1350 1200 50  0001 C CNN
F 3 "" H 1350 1200 50  0001 C CNN
	1    1350 1200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1350 1200 1350 1100
Wire Wire Line
	1350 1100 1700 1100
Wire Wire Line
	1350 1100 1000 1100
Wire Wire Line
	1000 1100 1000 1000
Connection ~ 1350 1100
Wire Wire Line
	1700 900  1000 900 
$Comp
L power:VSS #PWR?
U 1 1 63A5EADE
P 2700 1100
F 0 "#PWR?" H 2700 950 50  0001 C CNN
F 1 "VSS" H 2715 1273 50  0000 C CNN
F 2 "" H 2700 1100 50  0001 C CNN
F 3 "" H 2700 1100 50  0001 C CNN
	1    2700 1100
	-1   0    0    1   
$EndComp
Wire Wire Line
	2500 900  2700 900 
$Comp
L power:GND #PWR?
U 1 1 63A5FE5B
P 2700 1000
F 0 "#PWR?" H 2700 750 50  0001 C CNN
F 1 "GND" V 2705 872 50  0000 R CNN
F 2 "" H 2700 1000 50  0001 C CNN
F 3 "" H 2700 1000 50  0001 C CNN
	1    2700 1000
	0    -1   -1   0   
$EndComp
Wire Wire Line
	2500 1000 2700 1000
$Comp
L power:VDD #PWR?
U 1 1 63A6132B
P 2700 900
F 0 "#PWR?" H 2700 750 50  0001 C CNN
F 1 "VDD" H 2715 1073 50  0000 C CNN
F 2 "" H 2700 900 50  0001 C CNN
F 3 "" H 2700 900 50  0001 C CNN
	1    2700 900 
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 1100 2500 1100
Wire Wire Line
	3850 2300 4550 2300
Text Label 4550 2300 0    50   ~ 0
Lock
Wire Wire Line
	1700 3800 2400 3800
Text Label 2400 3800 0    50   ~ 0
PD
$Comp
L Analog_DAC:MCP4822 U?
U 1 1 63AA73F9
P 7700 2000
F 0 "U?" H 7700 2581 50  0000 C CNN
F 1 "MCP4822" H 7700 2490 50  0000 C CNN
F 2 "" H 8500 1700 50  0001 C CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20002249B.pdf" H 8500 1700 50  0001 C CNN
	1    7700 2000
	1    0    0    -1  
$EndComp
Text Label 7000 1900 2    50   ~ 0
SCLK
Wire Wire Line
	7000 1900 7200 1900
Text Label 7000 2000 2    50   ~ 0
LDAC
Text Label 7000 2100 2    50   ~ 0
SDI
Text Label 7000 2200 2    50   ~ 0
CS
Wire Wire Line
	7000 2200 7200 2200
Wire Wire Line
	7200 2100 7000 2100
Wire Wire Line
	7000 2000 7200 2000
$Comp
L power:VDD #PWR?
U 1 1 63AB3A39
P 7700 1150
F 0 "#PWR?" H 7700 1000 50  0001 C CNN
F 1 "VDD" H 7715 1323 50  0000 C CNN
F 2 "" H 7700 1150 50  0001 C CNN
F 3 "" H 7700 1150 50  0001 C CNN
	1    7700 1150
	1    0    0    -1  
$EndComp
$Comp
L power:VDD #PWR?
U 1 1 63AC4614
P 3850 1950
F 0 "#PWR?" H 3850 1800 50  0001 C CNN
F 1 "VDD" H 3865 2123 50  0000 C CNN
F 2 "" H 3850 1950 50  0001 C CNN
F 3 "" H 3850 1950 50  0001 C CNN
	1    3850 1950
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 63ACA76C
P 7700 2550
F 0 "#PWR?" H 7700 2300 50  0001 C CNN
F 1 "GND" H 7705 2377 50  0000 C CNN
F 2 "" H 7700 2550 50  0001 C CNN
F 3 "" H 7700 2550 50  0001 C CNN
	1    7700 2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	7700 2550 7700 2500
Wire Wire Line
	3450 2300 3850 2300
$Comp
L Amplifier_Operational:OP07 U?
U 1 1 63AE9778
P 2850 2300
F 0 "U?" H 3194 2254 50  0000 L CNN
F 1 "OP07" H 3194 2345 50  0000 L CNN
F 2 "" H 2900 2350 50  0001 C CNN
F 3 "https://www.analog.com/media/en/technical-documentation/data-sheets/OP07.pdf" H 2900 2450 50  0001 C CNN
F 4 "MCP6241T-E/SNCT-ND" H 2850 2300 50  0001 C CNN "Digikey Part Number"
	1    2850 2300
	1    0    0    1   
$EndComp
$Comp
L power:VDD #PWR?
U 1 1 63AEB4A0
P 2750 2650
F 0 "#PWR?" H 2750 2500 50  0001 C CNN
F 1 "VDD" H 2765 2823 50  0000 C CNN
F 2 "" H 2750 2650 50  0001 C CNN
F 3 "" H 2750 2650 50  0001 C CNN
	1    2750 2650
	-1   0    0    1   
$EndComp
Wire Wire Line
	2750 2650 2750 2600
Wire Wire Line
	2750 1950 2750 2000
NoConn ~ 2850 2000
NoConn ~ 2950 2000
Wire Wire Line
	2200 2200 2350 2200
$Comp
L Device:R R?
U 1 1 63AEDF3F
P 2850 1650
F 0 "R?" V 2643 1650 50  0000 C CNN
F 1 "10k" V 2734 1650 50  0000 C CNN
F 2 "" V 2780 1650 50  0001 C CNN
F 3 "~" H 2850 1650 50  0001 C CNN
	1    2850 1650
	0    1    1    0   
$EndComp
Wire Wire Line
	2700 1650 2350 1650
Wire Wire Line
	2350 1650 2350 2200
Connection ~ 2350 2200
Wire Wire Line
	2350 2200 2550 2200
Wire Wire Line
	3000 1650 3450 1650
Wire Wire Line
	3450 1650 3450 2300
Wire Wire Line
	3450 2300 3150 2300
$Comp
L Device:R R?
U 1 1 63AEF661
P 2300 2550
F 0 "R?" V 2093 2550 50  0000 C CNN
F 1 "10k" V 2184 2550 50  0000 C CNN
F 2 "" V 2230 2550 50  0001 C CNN
F 3 "~" H 2300 2550 50  0001 C CNN
	1    2300 2550
	-1   0    0    1   
$EndComp
$Comp
L Device:R R?
U 1 1 63AF04CE
P 1950 2400
F 0 "R?" V 1743 2400 50  0000 C CNN
F 1 "56k" V 1834 2400 50  0000 C CNN
F 2 "" V 1880 2400 50  0001 C CNN
F 3 "~" H 1950 2400 50  0001 C CNN
	1    1950 2400
	0    -1   -1   0   
$EndComp
Wire Wire Line
	2100 2400 2300 2400
Wire Wire Line
	2300 2400 2550 2400
Connection ~ 2300 2400
$Comp
L power:GND #PWR?
U 1 1 63AF2487
P 2300 2750
F 0 "#PWR?" H 2300 2500 50  0001 C CNN
F 1 "GND" H 2305 2577 50  0000 C CNN
F 2 "" H 2300 2750 50  0001 C CNN
F 3 "" H 2300 2750 50  0001 C CNN
	1    2300 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 2750 2300 2700
$Comp
L power:VDD #PWR?
U 1 1 63AF371D
P 1800 2400
F 0 "#PWR?" H 1800 2250 50  0001 C CNN
F 1 "VDD" V 1815 2527 50  0000 L CNN
F 2 "" H 1800 2400 50  0001 C CNN
F 3 "" H 1800 2400 50  0001 C CNN
	1    1800 2400
	0    -1   -1   0   
$EndComp
Connection ~ 3450 2300
Wire Wire Line
	1500 3800 1700 3800
$Comp
L power:GND #PWR?
U 1 1 63B0E1C8
P 2750 1950
F 0 "#PWR?" H 2750 1700 50  0001 C CNN
F 1 "GND" H 2755 1777 50  0000 C CNN
F 2 "" H 2750 1950 50  0001 C CNN
F 3 "" H 2750 1950 50  0001 C CNN
	1    2750 1950
	-1   0    0    1   
$EndComp
$Comp
L Device:Opamp_Dual_Generic U?
U 1 1 63B16594
P 9050 1800
F 0 "U?" H 9050 1433 50  0000 C CNN
F 1 "Opamp_Dual_Generic" H 9050 1524 50  0000 C CNN
F 2 "" H 9050 1800 50  0001 C CNN
F 3 "~" H 9050 1800 50  0001 C CNN
	1    9050 1800
	1    0    0    1   
$EndComp
$Comp
L Device:Opamp_Dual_Generic U?
U 2 1 63B16EF0
P 9050 2300
F 0 "U?" H 9050 2667 50  0000 C CNN
F 1 "Opamp_Dual_Generic" H 9050 2576 50  0000 C CNN
F 2 "" H 9050 2300 50  0001 C CNN
F 3 "~" H 9050 2300 50  0001 C CNN
	2    9050 2300
	1    0    0    -1  
$EndComp
$Comp
L Device:Opamp_Dual_Generic U?
U 3 1 63B17A6F
P 9350 3450
F 0 "U?" H 9308 3496 50  0000 L CNN
F 1 "Opamp_Dual_Generic" H 9308 3405 50  0000 L CNN
F 2 "" H 9350 3450 50  0001 C CNN
F 3 "~" H 9350 3450 50  0001 C CNN
	3    9350 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	8200 1900 8750 1900
$Comp
L Device:R R?
U 1 1 63B23260
P 8450 1250
F 0 "R?" V 8243 1250 50  0000 C CNN
F 1 "10k" V 8334 1250 50  0000 C CNN
F 2 "" V 8380 1250 50  0001 C CNN
F 3 "~" H 8450 1250 50  0001 C CNN
	1    8450 1250
	0    1    1    0   
$EndComp
Wire Wire Line
	8750 1700 8750 1250
Wire Wire Line
	8750 1250 8600 1250
$Comp
L Device:R R?
U 1 1 63B23EBC
P 9000 1250
F 0 "R?" V 8793 1250 50  0000 C CNN
F 1 "17k" V 8884 1250 50  0000 C CNN
F 2 "" V 8930 1250 50  0001 C CNN
F 3 "~" H 9000 1250 50  0001 C CNN
	1    9000 1250
	0    1    1    0   
$EndComp
Wire Wire Line
	8750 1250 8850 1250
Connection ~ 8750 1250
Wire Wire Line
	9150 1250 9550 1250
Wire Wire Line
	9550 1250 9550 1800
Wire Wire Line
	9550 1800 9350 1800
Wire Wire Line
	7700 1150 7700 1250
Wire Wire Line
	8300 1250 7700 1250
Connection ~ 7700 1250
Wire Wire Line
	7700 1250 7700 1600
Wire Wire Line
	8200 2200 8750 2200
$Comp
L Device:R R?
U 1 1 63B2CA87
P 8950 2750
F 0 "R?" V 8743 2750 50  0000 C CNN
F 1 "17k" V 8834 2750 50  0000 C CNN
F 2 "" V 8880 2750 50  0001 C CNN
F 3 "~" H 8950 2750 50  0001 C CNN
	1    8950 2750
	0    1    1    0   
$EndComp
$Comp
L Device:R R?
U 1 1 63B2CF89
P 8550 2750
F 0 "R?" V 8343 2750 50  0000 C CNN
F 1 "10k" V 8434 2750 50  0000 C CNN
F 2 "" V 8480 2750 50  0001 C CNN
F 3 "~" H 8550 2750 50  0001 C CNN
	1    8550 2750
	0    1    1    0   
$EndComp
Wire Wire Line
	8750 2400 8750 2750
Wire Wire Line
	8750 2750 8700 2750
Wire Wire Line
	8750 2750 8800 2750
Connection ~ 8750 2750
$Comp
L power:VDD #PWR?
U 1 1 63B2EA64
P 8350 2750
F 0 "#PWR?" H 8350 2600 50  0001 C CNN
F 1 "VDD" V 8365 2877 50  0000 L CNN
F 2 "" H 8350 2750 50  0001 C CNN
F 3 "" H 8350 2750 50  0001 C CNN
	1    8350 2750
	0    -1   -1   0   
$EndComp
Wire Wire Line
	8350 2750 8400 2750
Wire Wire Line
	9100 2750 9550 2750
Wire Wire Line
	9550 2750 9550 2300
Wire Wire Line
	9550 2300 9350 2300
$Comp
L power:VSS #PWR?
U 1 1 63B338CF
P 9250 3750
F 0 "#PWR?" H 9250 3600 50  0001 C CNN
F 1 "VSS" H 9265 3923 50  0000 C CNN
F 2 "" H 9250 3750 50  0001 C CNN
F 3 "" H 9250 3750 50  0001 C CNN
	1    9250 3750
	-1   0    0    1   
$EndComp
$Comp
L power:VDD #PWR?
U 1 1 63B34292
P 9250 3150
F 0 "#PWR?" H 9250 3000 50  0001 C CNN
F 1 "VDD" H 9265 3323 50  0000 C CNN
F 2 "" H 9250 3150 50  0001 C CNN
F 3 "" H 9250 3150 50  0001 C CNN
	1    9250 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	9550 1800 10000 1800
Connection ~ 9550 1800
Wire Wire Line
	9550 2300 10000 2300
Connection ~ 9550 2300
Text Label 10000 1800 0    50   ~ 0
PZT
Text Label 10000 2300 0    50   ~ 0
Current
$EndSCHEMATC
