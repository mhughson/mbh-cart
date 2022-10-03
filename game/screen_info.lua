-----------------------
-- Name: NTSC Safe Area
-- Author: Sour
-----------------------
-- Displays an overlay based on tepples' "Safe Area on 5.37Mhz NTSC VDPs" diagram
-- Source: https://wiki.nesdev.com/w/index.php/File:Safe_areas.png
--
-- Red: "Danger Zone" (Top and bottom 8 pixels)
--      Most TVs hide all of this.
--
-- Gray: Outside the "Title Safe Area".
--       Some TVs may hide portions of this.
--       Text, etc. should not appear here.
-----------------------

function drawSafeArea()
  local red = 0x40FF0000
  local gray = 0x60505050
  local yellow = 0x60FFFF00
  local blue = 0x600000FF

  emu.drawRectangle(0, 0, 256, 8, red, true)
  emu.drawRectangle(0, 232, 256, 8, red, true)

  emu.drawRectangle(0, 8, 8, 224, yellow, true)
  emu.drawRectangle(248, 8, 16, 224, yellow, true)
  emu.drawRectangle(8, 8, 240, 8, yellow, true)
  emu.drawRectangle(8, 228, 240, 4, yellow, true)

  emu.drawRectangle(8, 16, 8, 212, blue, true)
  emu.drawRectangle(240, 16, 8, 212, blue, true)
  emu.drawRectangle(16, 16, 224, 8, blue, true)
  emu.drawRectangle(16, 216, 224, 12, blue, true)
  
  emu.drawLine(128, 0, 128, 240, gray, true)
  emu.drawLine(0, 120, 256, 120, gray, true)
end

emu.addEventCallback(drawSafeArea, emu.eventType.endFrame)
