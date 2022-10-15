// As a side effect - we modify the camera image itself;
// to save 20-30k of memory.
//
camera_fb_t * diff(camera_fb_t * current_frame)
{
  // declare the previous frame as a static - so we keep
  // it from call to call.
  //
  static uint8_t * previous_frame = NULL;
  if (previous_frame == NULL) {
    previous_frame = (uint8_t *)malloc(current_frame->len);
    if (previous_frame == NULL) {
      Log.println("Error; could not malloc image for diff()ing");
      return NULL;
    }
    // use the current frame as the starting pont for our diff.
    //
    memcpy(previous_frame, current_frame, current_frame->len);
  }

  const int bitsPerPixel = 8;                   // 8 bit greyscale
  const uint16_t delta = (1<<bitsPerPixel) / 8; // cutoff at 1/8th of the full range different.

  for (size_t i = 0; i < current_frame->len; i++) {
    //compare the same pixel in 2 frames -- this works as we are
    // a 8 bit grayscale.
    //
    uint16_t pb = current_frame->buf[ i ];
    uint16_t pf = previous_frame[ i ];

    //if the change is greter than the delta in either direction
    //
    if (abs( pf - pb ) > delta) {
      // different from background
      current_frame->buf[ i ] = (uint8_t) 255;
    } else {
      // same-ish as background
      current_frame->buf[ i ] = (uint8_t) 0;
    }

    //background is not the previous frame, instead it's a slowly changing average background
    //i.e. we don't compare the previous frame with this one but the bg frame with this one
    //and make the background a little bit more like this pixel, to adjust to slow changes in lighting
    //
    if ( pf > pb ) {
      previous_frame[i] = pb + 1;
    }
    if ( pf < pb ) {
      previous_frame[i] = pb - 1;
    }
  };
  return current_frame;
}

// Return value is the percentage of the pixels that have changed
//
//  0..2   no real target/COG - ignore returnX,Y
//  > 2    returnX,Y usable - higher numers mean more confidence
//
int calculate_cog(camera_fb_t * resultFrame, float * returnX, float * returnY)
{
  int bitsPerPixel = 8; //8 bit greyscale
  int numPixels = (8 * resultFrame->len) / bitsPerPixel; //stolen from Richard, cancles out!

  assert(numPixels == resultFrame->len);
  assert(resultFrame->width ==  160);
  assert(resultFrame->height ==  120);

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

  // there is not much sensible stuff that we can do - so bail out and
  // do not change returnX/returnY.
  //
  if (sumN == 0)
    return 0;

  ///centre of gravity is "cog"
  uint32_t cogX = 0;
  uint32_t cogY = 0;

  uint32_t percentage = (sumN * 100) / numPixels;
  cogX = sumX / sumN;
  cogY = sumY / sumN;

  //where we're going to look
  //cogx is scaled to width of frame, so we get range 0-1
  *returnX = ((float)cogX  / resultFrame->width);
  *returnY = 1. - ((float)cogY  / resultFrame->width); // use the larger dimension so x & y get the same scaling

  return percentage;
}
