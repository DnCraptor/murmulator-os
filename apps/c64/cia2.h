/*
	Copyright Frank Bosing, 2017	

	This file is part of Teensy64.

    Teensy64 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Teensy64 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Teensy64.  If not, see <http://www.gnu.org/licenses/>.

    Diese Datei ist Teil von Teensy64.

    Teensy64 ist Freie Software: Sie konnen es unter den Bedingungen
    der GNU General Public License, wie von der Free Software Foundation,
    Version 3 der Lizenz oder (nach Ihrer Wahl) jeder spateren
    veroffentlichten Version, weiterverbreiten und/oder modifizieren.

    Teensy64 wird in der Hoffnung, dass es nutzlich sein wird, aber
    OHNE JEDE GEWAHRLEISTUNG, bereitgestellt; sogar ohne die implizite
    Gewahrleistung der MARKTFAHIGKEIT oder EIGNUNG FUR EINEN BESTIMMTEN ZWECK.
    Siehe die GNU General Public License fur weitere Details.

    Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
    Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
		
*/

#ifndef Teensy64_cia2_h_
#define Teensy64_cia2_h_


void cia2_clock(int clk) __attribute__ ((hot));
void cia2_checkRTCAlarm() __attribute__ ((hot));
void cia2_write(uint32_t address, uint8_t value) __attribute__ ((hot));
uint8_t cia2_read(uint32_t address) __attribute__ ((hot));

void resetCia2(void);

#endif
