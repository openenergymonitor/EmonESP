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
#include <emonesp.h>

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
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
        {
                switch(pixel%4)
                {
                case 0:
                        strip.SetPixelColor(pixel,red);
                        break;
                case 1:
                        strip.SetPixelColor(pixel,green);
                        break;
                case 2:
                        strip.SetPixelColor(pixel,blue);
                        break;
                case 3:
                        strip.SetPixelColor(pixel,white);
                        break;
                }
                strip.Show();
                delay(50);
        }

}

void light_to_pixel(float val){
    int number_full = (int)val / (255);
    float f_last_pixel = val - (number_full * 255);
    uint8_t last_pixel = (uint8_t) f_last_pixel;
    DEBUG.println("set: " + String(number_full) + " to full and last pixel to:" + last_pixel + " float val is: "  + f_last_pixel);
    for (uint16_t pixel = 0; pixel < number_full; pixel++)
    {
      strip.SetPixelColor(pixel,red);
      strip.Show();
      delay(50);
    }
    strip.SetPixelColor(number_full, RgbColor(last_pixel,0,0));
    strip.Show();

}

void pixel_off(){
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
        {
                strip.SetPixelColor(pixel, black);
        }
        strip.Show();
}
void set_pixel(uint8_t pixel, uint8_t red, uint8_t green, uint8_t blue)
{
  strip.SetPixelColor(pixel, RgbColor(red,green,blue));
  strip.Show();
}
