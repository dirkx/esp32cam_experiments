
int img_buf_len = 19200; //hardcoded but it's FRAMESIZE_QQVGA (160x120) grayscale pixels with 8 bits

camera_fb_t * resultFrame = new camera_fb_t();
camera_fb_t * diff(camera_fb_t * fb)
{
  static uint8_t * img_buf_bg = NULL; 

  if (!img_buf_bg) {
    //Serial.println("returning!");
    //create a new buffer based on this one to be our background
    img_buf_bg = new uint8_t[img_buf_len];
    memcpy(img_buf_bg, fb->buf, img_buf_len);
    return NULL;
  }

  //compare bufs
  int bitsPerPixel = 8; //8 bit greyscale
  int numPixels = (8 * fb->len) / bitsPerPixel; //stolen from Richard, cancles out!

  //how big a change is significant
  //if it's an 8 bit frame, we make the delta 32
  //i.e. a value of 32 in pixel brightness
  //our pixel brightness goes from 0-255
  //I just tried a bunch of numbers here

  uint16_t delta = bitsPerPixel * 4;
  uint16_t x = 0, y = 0;

  //just make a copy to use for our results, we replace all the pixels anyhoo
  memcpy(&resultFrame, &fb, sizeof fb);

  //loop over all the pixels in the frame
  //could also  do it by x and y

  for ( uint32_t i = 0; i < numPixels; i += 1 ) {
    x ++;
    if ( x >= fb->width ) {
      y++;
      x = 0;
    }

    //compare the same pixel in 2 frames

    uint16_t pb = fb->buf[ i ];
    uint16_t pf = img_buf_bg[ i ];
    //Serial.print("pf - pb");
    //Serial.println(pf - pb);

    //if the change is greter than the delta in either direction

    if ( ( pf > pb && pf - pb > delta ) || ( pf < pb && pb - pf > delta )) {
      // different from background
      //resultFrame->setPixel( i, pf ); //fixme
      //Serial.println("DIFFERENT");
      //Serial.println(pf - pb);
      resultFrame->buf[ i ] = (uint8_t) 255;
    } else {
      // same-ish as background
      //Serial.println("SAME");
      //Serial.println(pf - pb);
      resultFrame->buf[ i ] = (uint8_t) 0;
    }

    //background is not the previous frame, instead it's a slowly changing average background
    //i.e. we don't compare the previous frame with this one but the bg frame with this one
    //and make the background a little bit more like this pixel, to adjust to slow changes in lighting

    if ( pf > pb ) {
      img_buf_bg[i] = pb + 1;
    }
    if ( pf < pb ) {
      img_buf_bg[i] = pb - 1;
    }

  }
  return resultFrame;
}

// Return value is the percentage of the pixels that have changed
//
//  0..2   no real target/COG - ignore returnX,Y
//  > 2    returnX,Y usable - higher numers mean more confidence
//
int calculate_cog(camera_fb_t * resultFrame, float *returnX, float *returnY)
{
  int bitsPerPixel = 8; //8 bit greyscale
  int numPixels = (8 * resultFrame->len) / bitsPerPixel; //stolen from Richard, cancles out!
  
  //laying groundwork for where the centre of gravity' - the average position of all the diferent bits
  uint32_t sumX = 0;
  uint32_t sumY = 0;
  uint32_t sumN = 0;
  for ( uint32_t i = 0, x = 0, y = 0; i < numPixels; i += 1 ) {
    x ++;
    if ( x >= resultFrame->width ) {
      y++;
      x = 0;
    }
    if (resultFrame->buf[ i ]) {
      sumX += x;
      sumY += y;
      sumN ++;
    };
  };

  ///centre of gravity is "cog"
  uint32_t cogX = 0;
  uint32_t cogY = 0;

  uint32_t percentage = (sumN * 100) / numPixels;

  cogX = sumX / sumN;
  cogY = sumY / sumN;

  //where we're going to look
  //cogx is scaled to width of frame, so we get range 0-1
  *returnX = ((float)cogX  / resultFrame->width);
  *returnY = 1 - ((float)cogY  / resultFrame->width); // use the larger dimension so x & y get the same scaling

  return percentage;
}
