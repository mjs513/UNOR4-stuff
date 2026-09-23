void writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap, const uint16_t *palette) {
  tft.startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      tft.writePixel(x + i, y, palette[*bitmap++]);
    }
  }
  tft.endWrite();
}


void drawPNG(const char *filename, int x, int y) {
  struct image_info *image_info = (struct image_info *)filename;

  uint8_t image_bottom = (image_info->image_bottom) ? image_info->image_bottom : 63;
  if (image_info->image_top) {
    tft.fillRect(x, y, 64, image_info->image_top, COLOR_BLACK);
  }
  if (image_bottom < 63) {
    tft.fillRect(x, y + image_bottom, 64, 63 - image_bottom, COLOR_BLACK);
  }

  if (image_info->palette == nullptr) {
    tft.drawRGBBitmap(x, y + image_info->image_top, (const uint16_t *)image_info->image, 64, (image_bottom - image_info->image_top) + 1);

  } else {
    writeRect8BPP(x, y + image_info->image_top, 64, (image_bottom - image_info->image_top) + 1, (const uint8_t *)image_info->image, image_info->palette);
    //tft.writeRect8BPP(x, y, 64, 64, (const uint8_t *)image_info->image, image_info->palette);
  }
};

void drawKeyboard(const char *filename, int x, int y) {
  struct image_info *image_info = (struct image_info *)filename;

  if (image_info->palette == nullptr) {
    tft.drawRGBBitmap(x, y, (const uint16_t *)image_info->image, 40, 14);
  } else {
    writeRect8BPP(x, y, 40, 14, (const uint8_t *)image_info->image, image_info->palette);
  }
}
