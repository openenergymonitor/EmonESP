 /*
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <pixel.h>
#include <NeoPixelBus.h>

// ESP Bitbang method using GPIO4

// void pixel_setup(uint16_t PixelCount, uint16_t PixelPin)
// {
  // NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
// }

NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

void pixel_begin()
{
  // this resets all the neopixels to an off state
  strip.Begin();
  strip.Show();
}

void pixel_rgb_demo(){
  strip.SetPixelColor(0, red);
  strip.SetPixelColor(1, green);
  strip.SetPixelColor(2, blue);
  strip.SetPixelColor(3, white);
  strip.Show();
}

void pixel_off(){
  strip.SetPixelColor(0, black);
  strip.SetPixelColor(1, black);
  strip.SetPixelColor(2, black);
  strip.SetPixelColor(3, black);
  strip.Show();
}